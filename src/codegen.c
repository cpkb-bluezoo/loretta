/*
 * codegen.c
 * Code generation - AST to JVM bytecode
 * Copyright (C) 2026 Chris Burdess <dog@bluezoo.org>
 *
 * This file is part of loretta.
 *
 * loretta is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * loretta is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "codegen.h"
#include <string.h>
#include <stdlib.h>

/* Forward declarations */
static void codegen_expr(codegen_ctx_t *ctx, ast_node_t *node);
static void codegen_stmt(codegen_ctx_t *ctx, ast_node_t *node);
static void codegen_stmts(codegen_ctx_t *ctx, slist_t *stmts);

/* ========================================================================
 * Code generation context
 * ======================================================================== */

codegen_ctx_t *codegen_ctx_new(class_writer_t *cw, method_info_t *method,
                                indy_cache_t *cache, scope_t *scope,
                                source_file_t *source)
{
    codegen_ctx_t *ctx = calloc(1, sizeof(codegen_ctx_t));
    if (!ctx) {
        return NULL;
    }

    ctx->cw = cw;
    ctx->method = method;
    ctx->code_attr = code_attr_new(class_writer_get_cp(cw));
    ctx->code = ctx->code_attr->data.code.code;
    ctx->indy_cache = cache;
    ctx->locals = hashtable_new();
    ctx->next_local = 0;
    ctx->max_locals = 0;
    ctx->stack_depth = 0;
    ctx->max_stack = 0;
    ctx->labels = NULL;
    ctx->loop_stack = NULL;
    ctx->try_stack = NULL;
    ctx->current_line = 1;
    ctx->source = source;
    ctx->scope = scope;
    ctx->error_msg = NULL;

    return ctx;
}

void codegen_ctx_free(codegen_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }

    hashtable_free(ctx->locals);
    slist_free_full(ctx->labels, free);
    slist_free(ctx->loop_stack);
    slist_free(ctx->try_stack);
    free(ctx->error_msg);
    /* Note: code_attr is owned by the method, not freed here */
    free(ctx);
}

/* ========================================================================
 * Local variable management
 * ======================================================================== */

int codegen_alloc_local(codegen_ctx_t *ctx, const char *name)
{
    int slot = ctx->next_local++;
    if (ctx->next_local > ctx->max_locals) {
        ctx->max_locals = ctx->next_local;
    }

    local_var_t *var = malloc(sizeof(local_var_t));
    if (var) {
        var->name = str_dup(name);
        var->slot = slot;
        var->start_pc = ctx->code->len;
        var->end_pc = -1;
        hashtable_insert(ctx->locals, name, var);
    }

    return slot;
}

int codegen_get_local(codegen_ctx_t *ctx, const char *name)
{
    local_var_t *var = hashtable_lookup(ctx->locals, name);
    return var ? var->slot : -1;
}

/* ========================================================================
 * Label management
 * ======================================================================== */

label_t *codegen_new_label(codegen_ctx_t *ctx)
{
    label_t *label = calloc(1, sizeof(label_t));
    if (label) {
        label->offset = -1;  /* Unresolved */
        label->references = NULL;
        ctx->labels = slist_append(ctx->labels, label);
    }
    return label;
}

void codegen_mark_label(codegen_ctx_t *ctx, label_t *label)
{
    if (label) {
        label->offset = ctx->code->len;
    }
}

void codegen_emit_jump(codegen_ctx_t *ctx, uint8_t opcode, label_t *label)
{
    emit_u8(ctx, opcode);

    if (label->offset >= 0) {
        /* Backward jump - offset is known */
        int16_t offset = label->offset - (ctx->code->len - 1);
        emit_i16(ctx, offset);
    } else {
        /* Forward jump - record reference for later resolution */
        size_t *ref = malloc(sizeof(size_t));
        if (ref) {
            *ref = ctx->code->len;
            label->references = slist_append(label->references, ref);
        }
        emit_i16(ctx, 0);  /* Placeholder */
    }
}

void codegen_resolve_labels(codegen_ctx_t *ctx)
{
    for (slist_t *l = ctx->labels; l; l = l->next) {
        label_t *label = l->data;
        for (slist_t *r = label->references; r; r = r->next) {
            size_t ref_offset = *(size_t *)r->data;
            int16_t offset = label->offset - (ref_offset - 1);
            bytebuf_patch_u16(ctx->code, ref_offset, (uint16_t)offset);
        }
        slist_free_full(label->references, free);
        label->references = NULL;
    }
}

/* ========================================================================
 * Stack tracking
 * ======================================================================== */

void stack_push(codegen_ctx_t *ctx, int count)
{
    ctx->stack_depth += count;
    if (ctx->stack_depth > ctx->max_stack) {
        ctx->max_stack = ctx->stack_depth;
    }
}

void stack_pop(codegen_ctx_t *ctx, int count)
{
    ctx->stack_depth -= count;
}

/* ========================================================================
 * Bytecode emission helpers
 * ======================================================================== */

void emit_u8(codegen_ctx_t *ctx, uint8_t val)
{
    bytebuf_write_u8(ctx->code, val);
}

void emit_u16(codegen_ctx_t *ctx, uint16_t val)
{
    bytebuf_write_u16(ctx->code, val);
}

void emit_i16(codegen_ctx_t *ctx, int16_t val)
{
    bytebuf_write_i16(ctx->code, val);
}

void emit_iconst(codegen_ctx_t *ctx, int32_t value)
{
    if (value >= -1 && value <= 5) {
        emit_u8(ctx, OP_ICONST_0 + value);
    } else if (value >= -128 && value <= 127) {
        emit_u8(ctx, OP_BIPUSH);
        emit_u8(ctx, (uint8_t)value);
    } else if (value >= -32768 && value <= 32767) {
        emit_u8(ctx, OP_SIPUSH);
        emit_i16(ctx, (int16_t)value);
    } else {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        uint16_t idx = cp_add_integer(cp, value);
        if (idx <= 255) {
            emit_u8(ctx, OP_LDC);
            emit_u8(ctx, idx);
        } else {
            emit_u8(ctx, OP_LDC_W);
            emit_u16(ctx, idx);
        }
    }
    stack_push(ctx, 1);
}

void emit_lconst(codegen_ctx_t *ctx, int64_t value)
{
    if (value == 0) {
        emit_u8(ctx, OP_LCONST_0);
    } else if (value == 1) {
        emit_u8(ctx, OP_LCONST_1);
    } else {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        uint16_t idx = cp_add_long(cp, value);
        emit_u8(ctx, OP_LDC2_W);
        emit_u16(ctx, idx);
    }
    stack_push(ctx, 2);
}

void emit_fconst(codegen_ctx_t *ctx, float value)
{
    if (value == 0.0f) {
        emit_u8(ctx, OP_FCONST_0);
    } else if (value == 1.0f) {
        emit_u8(ctx, OP_FCONST_1);
    } else if (value == 2.0f) {
        emit_u8(ctx, OP_FCONST_2);
    } else {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        uint16_t idx = cp_add_float(cp, value);
        if (idx <= 255) {
            emit_u8(ctx, OP_LDC);
            emit_u8(ctx, idx);
        } else {
            emit_u8(ctx, OP_LDC_W);
            emit_u16(ctx, idx);
        }
    }
    stack_push(ctx, 1);
}

void emit_dconst(codegen_ctx_t *ctx, double value)
{
    if (value == 0.0) {
        emit_u8(ctx, OP_DCONST_0);
    } else if (value == 1.0) {
        emit_u8(ctx, OP_DCONST_1);
    } else {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        uint16_t idx = cp_add_double(cp, value);
        emit_u8(ctx, OP_LDC2_W);
        emit_u16(ctx, idx);
    }
    stack_push(ctx, 2);
}

void emit_aconst_null(codegen_ctx_t *ctx)
{
    emit_u8(ctx, OP_ACONST_NULL);
    stack_push(ctx, 1);
}

void emit_ldc_string(codegen_ctx_t *ctx, const char *str)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_string(cp, str);
    if (idx <= 255) {
        emit_u8(ctx, OP_LDC);
        emit_u8(ctx, idx);
    } else {
        emit_u8(ctx, OP_LDC_W);
        emit_u16(ctx, idx);
    }
    stack_push(ctx, 1);
}

void emit_aload(codegen_ctx_t *ctx, int slot)
{
    if (slot >= 0 && slot <= 3) {
        emit_u8(ctx, OP_ALOAD_0 + slot);
    } else if (slot <= 255) {
        emit_u8(ctx, OP_ALOAD);
        emit_u8(ctx, slot);
    } else {
        emit_u8(ctx, OP_WIDE);
        emit_u8(ctx, OP_ALOAD);
        emit_u16(ctx, slot);
    }
    stack_push(ctx, 1);
}

void emit_astore(codegen_ctx_t *ctx, int slot)
{
    if (slot >= 0 && slot <= 3) {
        emit_u8(ctx, OP_ASTORE_0 + slot);
    } else if (slot <= 255) {
        emit_u8(ctx, OP_ASTORE);
        emit_u8(ctx, slot);
    } else {
        emit_u8(ctx, OP_WIDE);
        emit_u8(ctx, OP_ASTORE);
        emit_u16(ctx, slot);
    }
    stack_pop(ctx, 1);
}

void emit_invokestatic(codegen_ctx_t *ctx, const char *class_name,
                        const char *method_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_methodref(cp, class_name, method_name, descriptor);
    emit_u8(ctx, OP_INVOKESTATIC);
    emit_u16(ctx, idx);
    /* TODO: Adjust stack based on descriptor */
}

void emit_invokevirtual(codegen_ctx_t *ctx, const char *class_name,
                         const char *method_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_methodref(cp, class_name, method_name, descriptor);
    emit_u8(ctx, OP_INVOKEVIRTUAL);
    emit_u16(ctx, idx);
}

void emit_invokespecial(codegen_ctx_t *ctx, const char *class_name,
                         const char *method_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_methodref(cp, class_name, method_name, descriptor);
    emit_u8(ctx, OP_INVOKESPECIAL);
    emit_u16(ctx, idx);
}

void emit_invokeinterface(codegen_ctx_t *ctx, const char *class_name,
                           const char *method_name, const char *descriptor,
                           uint8_t count)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_interface_methodref(cp, class_name, method_name, descriptor);
    emit_u8(ctx, OP_INVOKEINTERFACE);
    emit_u16(ctx, idx);
    emit_u8(ctx, count);
    emit_u8(ctx, 0);
}

void emit_invokedynamic(codegen_ctx_t *ctx, uint16_t indy_index)
{
    emit_u8(ctx, OP_INVOKEDYNAMIC);
    emit_u16(ctx, indy_index);
    emit_u8(ctx, 0);
    emit_u8(ctx, 0);
}

void emit_getstatic(codegen_ctx_t *ctx, const char *class_name,
                     const char *field_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_fieldref(cp, class_name, field_name, descriptor);
    emit_u8(ctx, OP_GETSTATIC);
    emit_u16(ctx, idx);
    stack_push(ctx, 1);
}

void emit_putstatic(codegen_ctx_t *ctx, const char *class_name,
                     const char *field_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_fieldref(cp, class_name, field_name, descriptor);
    emit_u8(ctx, OP_PUTSTATIC);
    emit_u16(ctx, idx);
    stack_pop(ctx, 1);
}

void emit_getfield(codegen_ctx_t *ctx, const char *class_name,
                    const char *field_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_fieldref(cp, class_name, field_name, descriptor);
    emit_u8(ctx, OP_GETFIELD);
    emit_u16(ctx, idx);
    /* Stack: objectref -> value */
}

void emit_putfield(codegen_ctx_t *ctx, const char *class_name,
                    const char *field_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_fieldref(cp, class_name, field_name, descriptor);
    emit_u8(ctx, OP_PUTFIELD);
    emit_u16(ctx, idx);
    stack_pop(ctx, 2);
}

void emit_new(codegen_ctx_t *ctx, const char *class_name)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_class(cp, class_name);
    emit_u8(ctx, OP_NEW);
    emit_u16(ctx, idx);
    stack_push(ctx, 1);
}

void emit_newarray(codegen_ctx_t *ctx, uint8_t atype)
{
    emit_u8(ctx, OP_NEWARRAY);
    emit_u8(ctx, atype);
    /* Stack: count -> arrayref */
}

void emit_anewarray(codegen_ctx_t *ctx, const char *class_name)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_class(cp, class_name);
    emit_u8(ctx, OP_ANEWARRAY);
    emit_u16(ctx, idx);
    /* Stack: count -> arrayref */
}

void emit_checkcast(codegen_ctx_t *ctx, const char *class_name)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_class(cp, class_name);
    emit_u8(ctx, OP_CHECKCAST);
    emit_u16(ctx, idx);
}

void emit_instanceof(codegen_ctx_t *ctx, const char *class_name)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_class(cp, class_name);
    emit_u8(ctx, OP_INSTANCEOF);
    emit_u16(ctx, idx);
    /* Stack: objectref -> int */
}

/* ========================================================================
 * Expression code generation
 * ======================================================================== */

/**
 * Emit code to create a Python int from a Java long.
 */
static void emit_py_int(codegen_ctx_t *ctx, int64_t value)
{
    /* Push the long value */
    emit_lconst(ctx, value);

    /* Call $I.of(long) to create PyInt */
    emit_invokestatic(ctx, LRT_INT, "of", "(J)" DESC_INT);
    stack_pop(ctx, 2);  /* long is 2 slots */
    stack_push(ctx, 1); /* PyInt is 1 slot */
}

/**
 * Emit code to create a Python float from a Java double.
 */
static void emit_py_float(codegen_ctx_t *ctx, double value)
{
    /* Push the double value */
    emit_dconst(ctx, value);

    /* Call $F.of(double) to create PyFloat */
    emit_invokestatic(ctx, LRT_FLOAT, "of", "(D)L" LRT_FLOAT ";");
    stack_pop(ctx, 2);  /* double is 2 slots */
    stack_push(ctx, 1);
}

/**
 * Emit code to create a Python string from a Java String.
 */
static void emit_py_str(codegen_ctx_t *ctx, const char *value)
{
    /* Push Java string constant */
    emit_ldc_string(ctx, value);

    /* Call $S.of(String) to create PyStr */
    emit_invokestatic(ctx, LRT_STR, "of", "(Ljava/lang/String;)" DESC_STR);
    /* Stack unchanged: String -> PyStr */
}

/**
 * Emit code for Python None.
 */
static void emit_py_none(codegen_ctx_t *ctx)
{
    /* Get $N.INSTANCE singleton */
    emit_getstatic(ctx, LRT_NONE, "INSTANCE", DESC_NONE);
}

/**
 * Emit code for Python True or False.
 */
static void emit_py_bool(codegen_ctx_t *ctx, bool value)
{
    /* Get $B.TRUE or $B.FALSE singleton */
    const char *field = value ? "TRUE" : "FALSE";
    emit_getstatic(ctx, LRT_BOOL, field, DESC_BOOL);
}

/**
 * Convert Python operator enum to invokedynamic operation.
 */
static py_indy_op_t binop_to_indy(bin_op_t op)
{
    switch (op) {
        case BINOP_ADD:      return PY_INDY_ADD;
        case BINOP_SUB:      return PY_INDY_SUB;
        case BINOP_MULT:     return PY_INDY_MUL;
        case BINOP_MATMULT:  return PY_INDY_MATMUL;
        case BINOP_DIV:      return PY_INDY_TRUEDIV;
        case BINOP_FLOORDIV: return PY_INDY_FLOORDIV;
        case BINOP_MOD:      return PY_INDY_MOD;
        case BINOP_POW:      return PY_INDY_POW;
        case BINOP_LSHIFT:   return PY_INDY_LSHIFT;
        case BINOP_RSHIFT:   return PY_INDY_RSHIFT;
        case BINOP_BITOR:    return PY_INDY_OR;
        case BINOP_BITXOR:   return PY_INDY_XOR;
        case BINOP_BITAND:   return PY_INDY_AND;
        default:             return PY_INDY_ADD;  /* Shouldn't happen */
    }
}

/**
 * Convert Python comparison operator to invokedynamic operation.
 */
static py_indy_op_t cmpop_to_indy(cmp_op_t op)
{
    switch (op) {
        case CMPOP_EQ:    return PY_INDY_EQ;
        case CMPOP_NOTEQ: return PY_INDY_NE;
        case CMPOP_LT:    return PY_INDY_LT;
        case CMPOP_LTE:   return PY_INDY_LE;
        case CMPOP_GT:    return PY_INDY_GT;
        case CMPOP_GTE:   return PY_INDY_GE;
        case CMPOP_IS:    return PY_INDY_IS;
        case CMPOP_ISNOT: return PY_INDY_IS_NOT;
        case CMPOP_IN:    return PY_INDY_CONTAINS;
        case CMPOP_NOTIN: return PY_INDY_NOT_CONTAINS;
        default:          return PY_INDY_EQ;
    }
}

/**
 * Convert Python unary operator to invokedynamic operation.
 */
static py_indy_op_t unaryop_to_indy(unary_op_t op)
{
    switch (op) {
        case UNARYOP_INVERT: return PY_INDY_INVERT;
        case UNARYOP_NOT:    return PY_INDY_NOT;
        case UNARYOP_UADD:   return PY_INDY_POS;
        case UNARYOP_USUB:   return PY_INDY_NEG;
        default:             return PY_INDY_NEG;
    }
}

/**
 * Generate code for an expression node.
 * Leaves result on the stack.
 */
static void codegen_expr(codegen_ctx_t *ctx, ast_node_t *node)
{
    if (!node || ctx->error_msg) {
        return;
    }

    switch (node->type) {
        /* Constants (literals) */
        case AST_CONSTANT: {
            switch (node->data.constant.kind) {
                case TOK_INTEGER:
                    emit_py_int(ctx, node->data.constant.value.int_val);
                    break;
                case TOK_FLOAT:
                    emit_py_float(ctx, node->data.constant.value.float_val);
                    break;
                case TOK_STRING:
                    emit_py_str(ctx, node->data.constant.value.str_val);
                    break;
                case TOK_BYTES:
                    /* For now, treat bytes like strings - runtime will handle */
                    emit_py_str(ctx, node->data.constant.value.str_val);
                    break;
                case TOK_TRUE:
                    emit_py_bool(ctx, true);
                    break;
                case TOK_FALSE:
                    emit_py_bool(ctx, false);
                    break;
                case TOK_NONE:
                    emit_py_none(ctx);
                    break;
                default:
                    emit_aconst_null(ctx);  /* Fallback */
                    break;
            }
            break;
        }

        /* Name lookup */
        case AST_NAME: {
            const char *name = node->data.name.id;
            int slot = codegen_get_local(ctx, name);

            if (slot >= 0) {
                /* Local variable */
                emit_aload(ctx, slot);
            } else {
                /* Global/built-in lookup via $G.builtin(name) */
                emit_ldc_string(ctx, name);
                emit_invokestatic(ctx, "$G", "builtin",
                                  "(Ljava/lang/String;)" DESC_OBJECT);
                /* Stack: String -> $O */
            }
            break;
        }

        /* Binary operations */
        case AST_BIN_OP: {
            codegen_expr(ctx, node->data.bin_op.left);
            codegen_expr(ctx, node->data.bin_op.right);

            py_indy_op_t op = binop_to_indy(node->data.bin_op.op);
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, op, NULL, 0);
            stack_pop(ctx, 2);
            stack_push(ctx, 1);
            break;
        }

        /* Unary operations */
        case AST_UNARY_OP: {
            codegen_expr(ctx, node->data.unary_op.operand);

            py_indy_op_t op = unaryop_to_indy(node->data.unary_op.op);
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, op, NULL, 0);
            /* Stack: operand -> result */
            break;
        }

        /* Comparison operations */
        case AST_COMPARE: {
            /* For now, handle single comparison only */
            /* TODO: Handle comparison chains properly */
            codegen_expr(ctx, node->data.compare.left);

            slist_t *ops = node->data.compare.ops;
            slist_t *comps = node->data.compare.comparators;

            if (ops && comps) {
                cmp_op_t *op = ops->data;
                ast_node_t *right = comps->data;

                codegen_expr(ctx, right);

                py_indy_op_t indy_op = cmpop_to_indy(*op);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, indy_op, NULL, 0);
                stack_pop(ctx, 2);
                stack_push(ctx, 1);
            }
            break;
        }

        /* Boolean operations (and, or) */
        case AST_BOOL_OP: {
            slist_t *values = node->data.bool_op.values;
            if (!values) break;

            bool is_or = (node->data.bool_op.op == BOOLOP_OR);

            /* Evaluate first value */
            codegen_expr(ctx, values->data);
            values = values->next;

            label_t *end_label = codegen_new_label(ctx);

            while (values) {
                /* Duplicate for testing */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);

                /* Convert to boolean for branching */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);

                /* For 'or': if true, short-circuit (keep value, jump to end)
                   For 'and': if false, short-circuit */
                if (is_or) {
                    codegen_emit_jump(ctx, OP_IFNE, end_label);
                } else {
                    codegen_emit_jump(ctx, OP_IFEQ, end_label);
                }
                stack_pop(ctx, 1);

                /* Pop the duplicated value (we'll use next one) */
                emit_u8(ctx, OP_POP);
                stack_pop(ctx, 1);

                /* Evaluate next value */
                codegen_expr(ctx, values->data);
                values = values->next;
            }

            codegen_mark_label(ctx, end_label);
            break;
        }

        /* Conditional expression: x if cond else y */
        case AST_IF_EXP: {
            label_t *else_label = codegen_new_label(ctx);
            label_t *end_label = codegen_new_label(ctx);

            /* Evaluate condition */
            codegen_expr(ctx, node->data.if_exp.test);

            /* Convert to boolean */
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, else_label);
            stack_pop(ctx, 1);

            /* True branch */
            codegen_expr(ctx, node->data.if_exp.body);
            codegen_emit_jump(ctx, OP_GOTO, end_label);

            /* False branch */
            codegen_mark_label(ctx, else_label);
            codegen_expr(ctx, node->data.if_exp.orelse);

            codegen_mark_label(ctx, end_label);
            break;
        }

        /* Attribute access: obj.attr */
        case AST_ATTRIBUTE: {
            codegen_expr(ctx, node->data.attribute.value);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_GETATTR,
                                node->data.attribute.attr, 0);
            /* Stack: obj -> attr_value */
            break;
        }

        /* Subscript: obj[key] */
        case AST_SUBSCRIPT: {
            codegen_expr(ctx, node->data.subscript.value);
            codegen_expr(ctx, node->data.subscript.slice);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_GETITEM, NULL, 0);
            stack_pop(ctx, 2);
            stack_push(ctx, 1);
            break;
        }

        /* Function call */
        case AST_CALL: {
            /* Push the callable */
            codegen_expr(ctx, node->data.call.func);

            /* Count arguments */
            int argc = 0;
            for (slist_t *a = node->data.call.args; a; a = a->next) {
                argc++;
            }

            /* Create array for arguments */
            emit_iconst(ctx, argc);
            emit_anewarray(ctx, LRT_OBJECT);

            /* Fill array with arguments */
            int i = 0;
            for (slist_t *a = node->data.call.args; a; a = a->next) {
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, i);
                codegen_expr(ctx, a->data);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                i++;
            }

            /* Call via invokedynamic */
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_CALL, NULL, argc);
            stack_pop(ctx, 2);  /* callable + args array */
            stack_push(ctx, 1); /* result */
            break;
        }

        /* List literal */
        case AST_LIST: {
            int count = 0;
            for (slist_t *e = node->data.collection.elts; e; e = e->next) {
                count++;
            }

            /* Create array */
            emit_iconst(ctx, count);
            emit_anewarray(ctx, LRT_OBJECT);

            /* Fill array */
            int i = 0;
            for (slist_t *e = node->data.collection.elts; e; e = e->next) {
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, i);
                codegen_expr(ctx, e->data);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                i++;
            }

            /* Call $L.of(Object[]) */
            emit_invokestatic(ctx, LRT_LIST, "of", "(" DESC_OBJECT_ARR ")" DESC_LIST);
            break;
        }

        /* Tuple literal */
        case AST_TUPLE: {
            int count = 0;
            for (slist_t *e = node->data.collection.elts; e; e = e->next) {
                count++;
            }

            emit_iconst(ctx, count);
            emit_anewarray(ctx, LRT_OBJECT);

            int i = 0;
            for (slist_t *e = node->data.collection.elts; e; e = e->next) {
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, i);
                codegen_expr(ctx, e->data);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                i++;
            }

            emit_invokestatic(ctx, LRT_TUPLE, "of", "(" DESC_OBJECT_ARR ")" DESC_TUPLE);
            break;
        }

        /* Dict literal */
        case AST_DICT: {
            int count = 0;
            for (slist_t *k = node->data.dict.keys; k; k = k->next) {
                count++;
            }

            /* Create arrays for keys and values */
            emit_iconst(ctx, count);
            emit_anewarray(ctx, LRT_OBJECT);

            int i = 0;
            for (slist_t *k = node->data.dict.keys; k; k = k->next) {
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, i);
                codegen_expr(ctx, k->data);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                i++;
            }

            emit_iconst(ctx, count);
            emit_anewarray(ctx, LRT_OBJECT);

            i = 0;
            for (slist_t *v = node->data.dict.values; v; v = v->next) {
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, i);
                codegen_expr(ctx, v->data);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                i++;
            }

            emit_invokestatic(ctx, LRT_DICT, "of",
                              "(" DESC_OBJECT_ARR DESC_OBJECT_ARR ")" DESC_DICT);
            stack_pop(ctx, 2);
            stack_push(ctx, 1);
            break;
        }

        default:
            /* TODO: Handle remaining expression types */
            emit_aconst_null(ctx);  /* Placeholder for unimplemented */
            break;
    }
}

/* ========================================================================
 * Statement code generation
 * ======================================================================== */

/**
 * Generate code for a statement node.
 */
static void codegen_stmt(codegen_ctx_t *ctx, ast_node_t *node)
{
    if (!node || ctx->error_msg) {
        return;
    }

    switch (node->type) {
        /* Expression statement */
        case AST_EXPR_STMT: {
            codegen_expr(ctx, node->data.expr_stmt.value);
            /* Pop the result (not used) */
            emit_u8(ctx, OP_POP);
            stack_pop(ctx, 1);
            break;
        }

        /* Assignment: target = value */
        case AST_ASSIGN: {
            /* Evaluate the value first */
            codegen_expr(ctx, node->data.assign.value);

            /* Handle each target (for chained assignment) */
            for (slist_t *t = node->data.assign.targets; t; t = t->next) {
                ast_node_t *target = t->data;

                if (t->next) {
                    /* More targets - duplicate the value */
                    emit_u8(ctx, OP_DUP);
                    stack_push(ctx, 1);
                }

                if (target->type == AST_NAME) {
                    const char *name = target->data.name.id;
                    int slot = codegen_get_local(ctx, name);

                    if (slot < 0) {
                        /* Allocate new local */
                        slot = codegen_alloc_local(ctx, name);
                    }

                    emit_astore(ctx, slot);
                } else if (target->type == AST_SUBSCRIPT) {
                    /* obj[key] = value */
                    /* Stack: value */
                    /* Need: obj, key, value on stack */
                    codegen_expr(ctx, target->data.subscript.value);  /* obj */
                    emit_u8(ctx, OP_SWAP);
                    codegen_expr(ctx, target->data.subscript.slice);  /* key */
                    emit_u8(ctx, OP_SWAP);

                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_SETITEM, NULL, 0);
                    stack_pop(ctx, 3);
                } else if (target->type == AST_ATTRIBUTE) {
                    /* obj.attr = value */
                    codegen_expr(ctx, target->data.attribute.value);  /* obj */
                    emit_u8(ctx, OP_SWAP);

                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_SETATTR,
                                        target->data.attribute.attr, 0);
                    stack_pop(ctx, 2);
                }
            }
            break;
        }

        /* Augmented assignment: target += value */
        case AST_AUG_ASSIGN: {
            ast_node_t *target = node->data.aug_assign.target;

            if (target->type == AST_NAME) {
                const char *name = target->data.name.id;
                int slot = codegen_get_local(ctx, name);

                if (slot >= 0) {
                    emit_aload(ctx, slot);
                } else {
                    emit_aconst_null(ctx);  /* TODO: Global lookup */
                }

                codegen_expr(ctx, node->data.aug_assign.value);

                /* Emit the binary operation */
                py_indy_op_t op = binop_to_indy(node->data.aug_assign.op);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, op, NULL, 0);
                stack_pop(ctx, 2);
                stack_push(ctx, 1);

                if (slot < 0) {
                    slot = codegen_alloc_local(ctx, name);
                }
                emit_astore(ctx, slot);
            }
            break;
        }

        /* If statement */
        case AST_IF: {
            label_t *else_label = codegen_new_label(ctx);
            label_t *end_label = codegen_new_label(ctx);

            /* Evaluate condition */
            codegen_expr(ctx, node->data.if_stmt.test);

            /* Convert to boolean */
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, else_label);
            stack_pop(ctx, 1);

            /* True branch */
            codegen_stmts(ctx, node->data.if_stmt.body);
            codegen_emit_jump(ctx, OP_GOTO, end_label);

            /* Else branch */
            codegen_mark_label(ctx, else_label);
            if (node->data.if_stmt.orelse) {
                codegen_stmts(ctx, node->data.if_stmt.orelse);
            }

            codegen_mark_label(ctx, end_label);
            break;
        }

        /* While loop */
        case AST_WHILE: {
            label_t *start_label = codegen_new_label(ctx);
            label_t *end_label = codegen_new_label(ctx);

            /* Push loop context for break/continue */
            loop_ctx_t loop_ctx = { end_label, start_label };
            ctx->loop_stack = slist_prepend(ctx->loop_stack, &loop_ctx);

            /* Loop start */
            codegen_mark_label(ctx, start_label);

            /* Evaluate condition */
            codegen_expr(ctx, node->data.while_stmt.test);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, end_label);
            stack_pop(ctx, 1);

            /* Body */
            codegen_stmts(ctx, node->data.while_stmt.body);

            /* Loop back */
            codegen_emit_jump(ctx, OP_GOTO, start_label);

            /* End (also handles else clause) */
            codegen_mark_label(ctx, end_label);
            if (node->data.while_stmt.orelse) {
                codegen_stmts(ctx, node->data.while_stmt.orelse);
            }

            /* Pop loop context */
            ctx->loop_stack = ctx->loop_stack->next;
            break;
        }

        /* For loop */
        case AST_FOR: {
            label_t *start_label = codegen_new_label(ctx);
            label_t *end_label = codegen_new_label(ctx);

            /* Push loop context */
            loop_ctx_t loop_ctx = { end_label, start_label };
            ctx->loop_stack = slist_prepend(ctx->loop_stack, &loop_ctx);

            /* Get iterator */
            codegen_expr(ctx, node->data.for_stmt.iter);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_ITER, NULL, 0);

            /* Store iterator in a temporary */
            int iter_slot = codegen_alloc_local(ctx, "$iter");
            emit_astore(ctx, iter_slot);

            /* Loop start */
            codegen_mark_label(ctx, start_label);

            /* Get next item */
            emit_aload(ctx, iter_slot);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_NEXT, NULL, 0);

            /* Check for StopIteration (null return) */
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            codegen_emit_jump(ctx, OP_IFNULL, end_label);
            stack_pop(ctx, 1);

            /* Store loop variable */
            ast_node_t *target = node->data.for_stmt.target;
            if (target->type == AST_NAME) {
                const char *name = target->data.name.id;
                int slot = codegen_get_local(ctx, name);
                if (slot < 0) {
                    slot = codegen_alloc_local(ctx, name);
                }
                emit_astore(ctx, slot);
            } else {
                emit_u8(ctx, OP_POP);  /* TODO: Handle tuple unpacking */
                stack_pop(ctx, 1);
            }

            /* Body */
            codegen_stmts(ctx, node->data.for_stmt.body);

            /* Loop back */
            codegen_emit_jump(ctx, OP_GOTO, start_label);

            /* End */
            codegen_mark_label(ctx, end_label);
            emit_u8(ctx, OP_POP);  /* Pop the null from iterator */
            stack_pop(ctx, 1);

            if (node->data.for_stmt.orelse) {
                codegen_stmts(ctx, node->data.for_stmt.orelse);
            }

            /* Pop loop context */
            ctx->loop_stack = ctx->loop_stack->next;
            break;
        }

        /* Break statement */
        case AST_BREAK: {
            if (ctx->loop_stack) {
                loop_ctx_t *loop = ctx->loop_stack->data;
                codegen_emit_jump(ctx, OP_GOTO, loop->break_label);
            }
            break;
        }

        /* Continue statement */
        case AST_CONTINUE: {
            if (ctx->loop_stack) {
                loop_ctx_t *loop = ctx->loop_stack->data;
                codegen_emit_jump(ctx, OP_GOTO, loop->continue_label);
            }
            break;
        }

        /* Return statement */
        case AST_RETURN: {
            if (node->data.return_stmt.value) {
                codegen_expr(ctx, node->data.return_stmt.value);
                emit_u8(ctx, OP_ARETURN);
                stack_pop(ctx, 1);
            } else {
                emit_py_none(ctx);
                emit_u8(ctx, OP_ARETURN);
                stack_pop(ctx, 1);
            }
            break;
        }

        /* Pass statement (no-op) */
        case AST_PASS:
            break;

        default:
            /* TODO: Handle remaining statement types */
            break;
    }
}

/**
 * Generate code for a list of statements.
 */
static void codegen_stmts(codegen_ctx_t *ctx, slist_t *stmts)
{
    for (slist_t *s = stmts; s; s = s->next) {
        codegen_stmt(ctx, s->data);
    }
}

/* ========================================================================
 * Module code generation
 * ======================================================================== */

int codegen_module(ast_node_t *ast, analyzer_t *analyzer,
                    source_file_t *source, compiler_options_t *opts)
{
    if (!ast || ast->type != AST_MODULE || !source) {
        return 1;
    }

    /* Derive class name from source file name */
    const char *filename = source->filename;
    const char *basename = strrchr(filename, '/');
    basename = basename ? basename + 1 : filename;

    char *class_name = str_dup(basename);
    char *dot = strrchr(class_name, '.');
    if (dot) {
        *dot = '\0';
    }

    /* Create class writer */
    class_writer_t *cw = class_writer_new(class_name, NULL, ACC_PUBLIC);
    if (!cw) {
        free(class_name);
        return 1;
    }

    class_writer_set_source_file(cw, basename);

    /* Initialize indy cache */
    indy_cache_t indy_cache;
    indy_cache_init(&indy_cache);

    /* Generate main method for module-level code */
    method_info_t *main_method = class_writer_add_method(cw, "main",
                                                          "([Ljava/lang/String;)V",
                                                          ACC_PUBLIC | ACC_STATIC);

    codegen_ctx_t *ctx = codegen_ctx_new(cw, main_method, &indy_cache,
                                          analyzer->global_scope, source);
    ctx->next_local = 1;  /* args takes slot 0 */
    ctx->max_locals = 1;

    /* Generate code for each statement */
    codegen_stmts(ctx, ast->data.module.body);

    /* Return from main */
    emit_u8(ctx, OP_RETURN);

    /* Finalize code attribute */
    codegen_resolve_labels(ctx);
    ctx->code_attr->data.code.max_stack = ctx->max_stack > 0 ? ctx->max_stack : 1;
    ctx->code_attr->data.code.max_locals = ctx->max_locals;

    /* Add code attribute to method */
    main_method->attributes = slist_append(main_method->attributes, ctx->code_attr);

    /* Write class file */
    const char *output_dir = opts->output_dir ? opts->output_dir : ".";
    bool success = class_writer_write_file(cw, output_dir);

    codegen_ctx_free(ctx);
    class_writer_free(cw);
    free(class_name);

    return success ? 0 : 1;
}

