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
#include <stdio.h>

/* Forward declarations */
static void codegen_expr(codegen_ctx_t *ctx, ast_node_t *node);
static void codegen_stmt(codegen_ctx_t *ctx, ast_node_t *node);
static void codegen_stmts(codegen_ctx_t *ctx, slist_t *stmts);
static void codegen_function_def(codegen_ctx_t *ctx, ast_node_t *node);
static void codegen_lambda(codegen_ctx_t *ctx, ast_node_t *node);

/* Global lambda counter for unique method names */
static int lambda_counter = 0;

/**
 * Update stackmap after an invokedynamic operation.
 * This tracks the stack effects of Python operations.
 */
static void stackmap_track_indy(codegen_ctx_t *ctx, py_indy_op_t op)
{
    if (!ctx->stackmap) {
        return;
    }

    const_pool_t *cp = class_writer_get_cp(ctx->cw);

    switch (op) {
        /* Binary ops: pop 2 $O, push 1 $O */
        case PY_INDY_ADD:
        case PY_INDY_SUB:
        case PY_INDY_MUL:
        case PY_INDY_MATMUL:
        case PY_INDY_TRUEDIV:
        case PY_INDY_FLOORDIV:
        case PY_INDY_MOD:
        case PY_INDY_POW:
        case PY_INDY_LSHIFT:
        case PY_INDY_RSHIFT:
        case PY_INDY_AND:
        case PY_INDY_OR:
        case PY_INDY_XOR:
            stackmap_pop(ctx->stackmap, 2);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Unary ops: pop 1 $O, push 1 $O */
        case PY_INDY_NEG:
        case PY_INDY_POS:
        case PY_INDY_INVERT:
        case PY_INDY_NOT:
            stackmap_pop(ctx->stackmap, 1);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Comparisons: pop 2 $O, push 1 $O (Python bool) */
        case PY_INDY_LT:
        case PY_INDY_LE:
        case PY_INDY_EQ:
        case PY_INDY_NE:
        case PY_INDY_GT:
        case PY_INDY_GE:
        case PY_INDY_IS:
        case PY_INDY_IS_NOT:
        case PY_INDY_CONTAINS:
        case PY_INDY_NOT_CONTAINS:
            stackmap_pop(ctx->stackmap, 2);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* BOOL: pop 1 $O, push 1 int (for use with ifeq/ifne) */
        case PY_INDY_BOOL:
            stackmap_pop(ctx->stackmap, 1);
            stackmap_push_int(ctx->stackmap);
            break;

        /* Getattr: pop 1 $O, push 1 $O */
        case PY_INDY_GETATTR:
            stackmap_pop(ctx->stackmap, 1);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Setattr: pop 2 $O (obj, value), push nothing */
        case PY_INDY_SETATTR:
            stackmap_pop(ctx->stackmap, 2);
            break;

        /* Getitem: pop 2 $O (obj, key), push 1 $O */
        case PY_INDY_GETITEM:
            stackmap_pop(ctx->stackmap, 2);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Setitem: pop 3 $O (obj, key, value), push nothing */
        case PY_INDY_SETITEM:
            stackmap_pop(ctx->stackmap, 3);
            break;

        /* Call: pop 2 ($O callable, $O[] args), push 1 $O */
        case PY_INDY_CALL:
        case PY_INDY_CALL_METHOD:
            stackmap_pop(ctx->stackmap, 2);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Iter: pop 1 $O, push 1 $O (iterator) */
        case PY_INDY_ITER:
            stackmap_pop(ctx->stackmap, 1);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        /* Next: pop 1 $O (iterator), push 1 $O (or null) */
        case PY_INDY_NEXT:
            stackmap_pop(ctx->stackmap, 1);
            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            break;

        default:
            break;
    }
}

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
    ctx->stackmap = stackmap_new();
    ctx->error_msg = NULL;

    /* Closure support */
    ctx->parent_ctx = NULL;
    ctx->captured_vars = NULL;
    ctx->closure_slot = -1;

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
    slist_free(ctx->captured_vars);  /* Names are not owned */
    stackmap_free(ctx->stackmap);
    free(ctx->error_msg);
    /* Note: code_attr is owned by the method, not freed here */
    free(ctx);
}

/**
 * Check if a variable name is in the captured vars list.
 * Returns the index (0-based) or -1 if not found.
 */
static int get_captured_index(codegen_ctx_t *ctx, const char *name)
{
    int idx = 0;
    for (slist_t *s = ctx->captured_vars; s; s = s->next) {
        if (strcmp((const char *)s->data, name) == 0) {
            return idx;
        }
        idx++;
    }
    return -1;
}

/**
 * Check if a name is declared global in the current context.
 */
static bool is_global(codegen_ctx_t *ctx, const char *name)
{
    for (slist_t *s = ctx->global_names; s; s = s->next) {
        if (strcmp((const char *)s->data, name) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Check if a name is declared nonlocal in the current context.
 */
static bool is_nonlocal(codegen_ctx_t *ctx, const char *name)
{
    for (slist_t *s = ctx->nonlocal_names; s; s = s->next) {
        if (strcmp((const char *)s->data, name) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Try to find a variable in parent contexts (for closures).
 * Returns the parent context and slot where the variable is found,
 * or NULL if not found.
 */
static codegen_ctx_t *find_in_parent(codegen_ctx_t *ctx, const char *name, int *out_slot)
{
    for (codegen_ctx_t *p = ctx->parent_ctx; p; p = p->parent_ctx) {
        int slot = codegen_get_local(p, name);
        if (slot >= 0) {
            if (out_slot) *out_slot = slot;
            return p;
        }
    }
    return NULL;
}

/**
 * Helper to check if a name is a parameter or will be locally defined.
 */
static bool is_param_or_local_def(ast_node_t *args_node, slist_t *body, const char *name)
{
    /* Check parameters */
    if (args_node && args_node->type == AST_ARGUMENTS) {
        for (slist_t *s = args_node->data.arguments.posonlyargs; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG && strcmp(arg->data.arg.arg, name) == 0) {
                return true;
            }
        }
        for (slist_t *s = args_node->data.arguments.args; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG && strcmp(arg->data.arg.arg, name) == 0) {
                return true;
            }
        }
        if (args_node->data.arguments.vararg) {
            ast_node_t *arg = args_node->data.arguments.vararg;
            if (arg && arg->type == AST_ARG && strcmp(arg->data.arg.arg, name) == 0) {
                return true;
            }
        }
        for (slist_t *s = args_node->data.arguments.kwonlyargs; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG && strcmp(arg->data.arg.arg, name) == 0) {
                return true;
            }
        }
        if (args_node->data.arguments.kwarg) {
            ast_node_t *arg = args_node->data.arguments.kwarg;
            if (arg && arg->type == AST_ARG && strcmp(arg->data.arg.arg, name) == 0) {
                return true;
            }
        }
    }

    /* Check for assignment to name in body (simple check for top-level assigns) */
    for (slist_t *s = body; s; s = s->next) {
        ast_node_t *stmt = s->data;
        if (stmt && stmt->type == AST_ASSIGN) {
            for (slist_t *t = stmt->data.assign.targets; t; t = t->next) {
                ast_node_t *target = t->data;
                if (target && target->type == AST_NAME &&
                    strcmp(target->data.name.id, name) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

/**
 * Recursively collect name references from an AST node.
 */
static void collect_names(ast_node_t *node, slist_t **names)
{
    if (!node) return;

    switch (node->type) {
        case AST_NAME:
            /* Add name if not already in list */
            for (slist_t *s = *names; s; s = s->next) {
                if (strcmp((const char *)s->data, node->data.name.id) == 0) {
                    return;  /* Already in list */
                }
            }
            *names = slist_append(*names, (void *)node->data.name.id);
            break;

        case AST_BIN_OP:
            collect_names(node->data.bin_op.left, names);
            collect_names(node->data.bin_op.right, names);
            break;

        case AST_UNARY_OP:
            collect_names(node->data.unary_op.operand, names);
            break;

        case AST_COMPARE:
            collect_names(node->data.compare.left, names);
            for (slist_t *s = node->data.compare.comparators; s; s = s->next) {
                collect_names(s->data, names);
            }
            break;

        case AST_CALL:
            collect_names(node->data.call.func, names);
            for (slist_t *s = node->data.call.args; s; s = s->next) {
                collect_names(s->data, names);
            }
            break;

        case AST_ATTRIBUTE:
            collect_names(node->data.attribute.value, names);
            break;

        case AST_SUBSCRIPT:
            collect_names(node->data.subscript.value, names);
            collect_names(node->data.subscript.slice, names);
            break;

        case AST_IF_EXP:
            collect_names(node->data.if_exp.test, names);
            collect_names(node->data.if_exp.body, names);
            collect_names(node->data.if_exp.orelse, names);
            break;

        case AST_LIST:
        case AST_TUPLE:
        case AST_SET:
            for (slist_t *s = node->data.collection.elts; s; s = s->next) {
                collect_names(s->data, names);
            }
            break;

        case AST_DICT:
            for (slist_t *s = node->data.dict.keys; s; s = s->next) {
                collect_names(s->data, names);
            }
            for (slist_t *s = node->data.dict.values; s; s = s->next) {
                collect_names(s->data, names);
            }
            break;

        default:
            break;
    }
}

/**
 * Collect name references from statements.
 */
static void collect_names_from_stmts(slist_t *stmts, slist_t **names)
{
    for (slist_t *s = stmts; s; s = s->next) {
        ast_node_t *stmt = s->data;
        if (!stmt) continue;

        switch (stmt->type) {
            case AST_EXPR_STMT:
                collect_names(stmt->data.expr_stmt.value, names);
                break;

            case AST_ASSIGN:
                collect_names(stmt->data.assign.value, names);
                break;

            case AST_AUG_ASSIGN:
                collect_names(stmt->data.aug_assign.target, names);
                collect_names(stmt->data.aug_assign.value, names);
                break;

            case AST_RETURN:
                collect_names(stmt->data.return_stmt.value, names);
                break;

            case AST_IF:
                collect_names(stmt->data.if_stmt.test, names);
                collect_names_from_stmts(stmt->data.if_stmt.body, names);
                collect_names_from_stmts(stmt->data.if_stmt.orelse, names);
                break;

            case AST_WHILE:
                collect_names(stmt->data.while_stmt.test, names);
                collect_names_from_stmts(stmt->data.while_stmt.body, names);
                break;

            case AST_FOR:
                collect_names(stmt->data.for_stmt.iter, names);
                collect_names_from_stmts(stmt->data.for_stmt.body, names);
                break;

            default:
                break;
        }
    }
}

/**
 * Collect free variables for a function.
 * Returns a list of variable names that are referenced but not defined locally.
 */
static slist_t *collect_free_vars(codegen_ctx_t *parent_ctx, ast_node_t *args_node, slist_t *body)
{
    /* Collect all name references in the function body */
    slist_t *all_names = NULL;
    collect_names_from_stmts(body, &all_names);

    /* Filter to only those that are free (not params/locals but in parent) */
    slist_t *free_vars = NULL;
    for (slist_t *s = all_names; s; s = s->next) {
        const char *name = (const char *)s->data;

        /* Skip if it's a parameter or locally assigned */
        if (is_param_or_local_def(args_node, body, name)) {
            continue;
        }

        /* Check if it exists in a parent context */
        int slot;
        if (find_in_parent(parent_ctx, name, &slot)) {
            /* It's a free variable - add to list if not already there */
            bool found = false;
            for (slist_t *f = free_vars; f; f = f->next) {
                if (strcmp((const char *)f->data, name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                free_vars = slist_append(free_vars, (void *)name);
            }
        }
    }

    slist_free(all_names);
    return free_vars;
}

/**
 * Collect free variables for a lambda (expression body).
 * Returns a list of variable names that are referenced but not defined as parameters.
 */
static slist_t *collect_free_vars_from_expr(codegen_ctx_t *parent_ctx, ast_node_t *args_node, ast_node_t *body_expr)
{
    /* Collect all name references in the expression */
    slist_t *all_names = NULL;
    collect_names(body_expr, &all_names);

    /* Get parameter names for filtering */
    slist_t *param_names = NULL;
    if (args_node && args_node->type == AST_ARGUMENTS) {
        for (slist_t *s = args_node->data.arguments.args; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG) {
                param_names = slist_append(param_names, (void *)arg->data.arg.arg);
            }
        }
    }

    /* Filter to only those that are free (not params but in parent) */
    slist_t *free_vars = NULL;
    for (slist_t *s = all_names; s; s = s->next) {
        const char *name = (const char *)s->data;

        /* Skip if it's a parameter */
        bool is_param = false;
        for (slist_t *p = param_names; p; p = p->next) {
            if (strcmp((const char *)p->data, name) == 0) {
                is_param = true;
                break;
            }
        }
        if (is_param) {
            continue;
        }

        /* Check if it exists in a parent context */
        int slot;
        if (find_in_parent(parent_ctx, name, &slot)) {
            /* It's a free variable - add to list if not already there */
            bool found = false;
            for (slist_t *f = free_vars; f; f = f->next) {
                if (strcmp((const char *)f->data, name) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                free_vars = slist_append(free_vars, (void *)name);
            }
        }
    }

    slist_free(all_names);
    slist_free(param_names);
    return free_vars;
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

    /* Track local type in stackmap - all Python locals are $O (PyObject) */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_set_local_object(ctx->stackmap, slot, cp, LRT_OBJECT);
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
        /* Record a StackMapTable frame at this branch target */
        if (ctx->stackmap) {
            stackmap_record_frame(ctx->stackmap, ctx->code->len);
        }
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
    if (ctx->stackmap) {
        stackmap_push_int(ctx->stackmap);
    }
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
    if (ctx->stackmap) {
        stackmap_push_long(ctx->stackmap);
    }
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
    if (ctx->stackmap) {
        stackmap_push_double(ctx->stackmap);
    }
}

void emit_aconst_null(codegen_ctx_t *ctx)
{
    emit_u8(ctx, OP_ACONST_NULL);
    stack_push(ctx, 1);
    if (ctx->stackmap) {
        stackmap_push_null(ctx->stackmap);
    }
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
    if (ctx->stackmap) {
        stackmap_push_object(ctx->stackmap, cp, "java/lang/String");
    }
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
    /* Push object type (PyObject) to stackmap */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
    }
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
    /* Pop from stackmap and set local type */
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_set_local_object(ctx->stackmap, slot, cp, LRT_OBJECT);
    }
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
    /* Push object type to stackmap - in loretta, fields are Python objects */
    if (ctx->stackmap) {
        stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
    }
}

void emit_putstatic(codegen_ctx_t *ctx, const char *class_name,
                     const char *field_name, const char *descriptor)
{
    const_pool_t *cp = class_writer_get_cp(ctx->cw);
    uint16_t idx = cp_add_fieldref(cp, class_name, field_name, descriptor);
    emit_u8(ctx, OP_PUTSTATIC);
    emit_u16(ctx, idx);
    stack_pop(ctx, 1);
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
    }
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
    uint16_t new_offset = ctx->code->len;  /* Save offset for uninitialized tracking */
    emit_u8(ctx, OP_NEW);
    emit_u16(ctx, idx);
    stack_push(ctx, 1);
    if (ctx->stackmap) {
        stackmap_push_uninitialized(ctx->stackmap, new_offset);
    }
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
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);  /* Pop int count */
        /* Build array descriptor like "[L$O;" */
        char arr_desc[256];
        snprintf(arr_desc, sizeof(arr_desc), "[L%s;", class_name);
        stackmap_push_object(ctx->stackmap, cp, arr_desc);
    }
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
    /* Update stackmap: pop long (already tracked), push PyInt */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_pop(ctx->stackmap, 2);  /* long takes 2 stackmap slots (long + top) */
        stackmap_push_object(ctx->stackmap, cp, LRT_INT);
    }
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
    /* Update stackmap: pop double (already tracked), push PyFloat */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_pop(ctx->stackmap, 2);  /* double takes 2 stackmap slots */
        stackmap_push_object(ctx->stackmap, cp, LRT_FLOAT);
    }
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
    /* Stack unchanged in count: String -> PyStr */
    /* Update stackmap: pop String (already tracked), push PyStr */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(ctx->cw);
        stackmap_pop(ctx->stackmap, 1);
        stackmap_push_object(ctx->stackmap, cp, LRT_STR);
    }
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

/* Forward declarations for comprehensions */
static void codegen_expr(codegen_ctx_t *ctx, ast_node_t *node);

/* Comprehension types */
typedef enum {
    COMP_LIST,
    COMP_SET
} comp_type_t;

/**
 * Generate code for a single comprehension generator.
 * Recursively handles nested generators.
 */
static void codegen_comprehension_loop(codegen_ctx_t *ctx, slist_t *generators,
                                        ast_node_t *elt, int result_slot,
                                        comp_type_t comp_type)
{
    if (!generators) {
        /* Base case: evaluate element and add to result */
        emit_aload(ctx, result_slot);

        codegen_expr(ctx, elt);

        /* Add to collection */
        if (comp_type == COMP_LIST) {
            emit_invokevirtual(ctx, LRT_LIST, "append", "(" DESC_OBJECT ")V");
        } else {
            emit_invokevirtual(ctx, LRT_SET, "add", "(" DESC_OBJECT ")V");
        }
        stack_pop(ctx, 2);
        return;
    }

    ast_node_t *gen = generators->data;
    ast_node_t *target = gen->data.comprehension.target;
    ast_node_t *iter_expr = gen->data.comprehension.iter;
    slist_t *ifs = gen->data.comprehension.ifs;

    /* Get iterator and store */
    codegen_expr(ctx, iter_expr);
    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_ITER, NULL, 0);
    stackmap_track_indy(ctx, PY_INDY_ITER);
    int iter_slot = codegen_alloc_local(ctx, "$comp_iter");
    emit_astore(ctx, iter_slot);

    /* Loop labels */
    label_t *loop_start = codegen_new_label(ctx);
    label_t *loop_end = codegen_new_label(ctx);

    codegen_mark_label(ctx, loop_start);

    /* Get next item (returns null on StopIteration via safeNext) */
    emit_aload(ctx, iter_slot);
    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_NEXT, NULL, 0);
    stackmap_track_indy(ctx, PY_INDY_NEXT);

    /* Check for null (end of iteration) */
    emit_u8(ctx, OP_DUP);
    stack_push(ctx, 1);
    codegen_emit_jump(ctx, OP_IFNULL, loop_end);
    stack_pop(ctx, 1);
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
    }

    /* Bind target variable */
    if (target->type == AST_NAME) {
        int target_slot = codegen_get_local(ctx, target->data.name.id);
        if (target_slot < 0) {
            target_slot = codegen_alloc_local(ctx, target->data.name.id);
        }
        emit_astore(ctx, target_slot);
    } else {
        /* TODO: Handle tuple unpacking in comprehensions */
        emit_u8(ctx, OP_POP);
        stack_pop(ctx, 1);
    }

    /* Evaluate filter conditions */
    label_t *skip_label = NULL;
    if (ifs) {
        skip_label = codegen_new_label(ctx);
        for (slist_t *cond = ifs; cond; cond = cond->next) {
            codegen_expr(ctx, cond->data);
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);
            stackmap_track_indy(ctx, PY_INDY_BOOL);
            codegen_emit_jump(ctx, OP_IFEQ, skip_label);
            stack_pop(ctx, 1);
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }
        }
    }

    /* Process nested generators or evaluate element */
    codegen_comprehension_loop(ctx, generators->next, elt, result_slot, comp_type);

    /* Skip label for failed conditions */
    if (skip_label) {
        codegen_mark_label(ctx, skip_label);
    }

    /* Loop back */
    codegen_emit_jump(ctx, OP_GOTO, loop_start);

    /* End of loop - pop the null */
    codegen_mark_label(ctx, loop_end);
    emit_u8(ctx, OP_POP);
    stack_pop(ctx, 1);
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
    }
}

/**
 * Generate code for dict comprehension loop.
 */
static void codegen_dict_comprehension_loop(codegen_ctx_t *ctx, slist_t *generators,
                                             ast_node_t *key_expr, ast_node_t *value_expr,
                                             int result_slot)
{
    if (!generators) {
        /* Base case: evaluate key/value and add to result */
        emit_aload(ctx, result_slot);
        codegen_expr(ctx, key_expr);
        codegen_expr(ctx, value_expr);

        /* Call __setitem__ via invokedynamic */
        indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_SETITEM, NULL, 0);
        stackmap_track_indy(ctx, PY_INDY_SETITEM);
        stack_pop(ctx, 3);
        return;
    }

    ast_node_t *gen = generators->data;
    ast_node_t *target = gen->data.comprehension.target;
    ast_node_t *iter_expr = gen->data.comprehension.iter;
    slist_t *ifs = gen->data.comprehension.ifs;

    /* Get iterator and store */
    codegen_expr(ctx, iter_expr);
    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_ITER, NULL, 0);
    stackmap_track_indy(ctx, PY_INDY_ITER);
    int iter_slot = codegen_alloc_local(ctx, "$comp_iter");
    emit_astore(ctx, iter_slot);

    /* Loop labels */
    label_t *loop_start = codegen_new_label(ctx);
    label_t *loop_end = codegen_new_label(ctx);

    codegen_mark_label(ctx, loop_start);

    /* Get next item */
    emit_aload(ctx, iter_slot);
    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_NEXT, NULL, 0);
    stackmap_track_indy(ctx, PY_INDY_NEXT);

    /* Check for null */
    emit_u8(ctx, OP_DUP);
    stack_push(ctx, 1);
    codegen_emit_jump(ctx, OP_IFNULL, loop_end);
    stack_pop(ctx, 1);
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
    }

    /* Bind target variable */
    if (target->type == AST_NAME) {
        int target_slot = codegen_get_local(ctx, target->data.name.id);
        if (target_slot < 0) {
            target_slot = codegen_alloc_local(ctx, target->data.name.id);
        }
        emit_astore(ctx, target_slot);
    } else {
        emit_u8(ctx, OP_POP);
        stack_pop(ctx, 1);
    }

    /* Evaluate filter conditions */
    label_t *skip_label = NULL;
    if (ifs) {
        skip_label = codegen_new_label(ctx);
        for (slist_t *cond = ifs; cond; cond = cond->next) {
            codegen_expr(ctx, cond->data);
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);
            stackmap_track_indy(ctx, PY_INDY_BOOL);
            codegen_emit_jump(ctx, OP_IFEQ, skip_label);
            stack_pop(ctx, 1);
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }
        }
    }

    /* Process nested generators or evaluate key/value */
    codegen_dict_comprehension_loop(ctx, generators->next, key_expr, value_expr, result_slot);

    /* Skip label for failed conditions */
    if (skip_label) {
        codegen_mark_label(ctx, skip_label);
    }

    /* Loop back */
    codegen_emit_jump(ctx, OP_GOTO, loop_start);

    /* End of loop - pop the null */
    codegen_mark_label(ctx, loop_end);
    emit_u8(ctx, OP_POP);
    stack_pop(ctx, 1);
    if (ctx->stackmap) {
        stackmap_pop(ctx->stackmap, 1);
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

            /* Check if declared global - always use global lookup */
            if (is_global(ctx, name)) {
                emit_ldc_string(ctx, name);
                emit_invokestatic(ctx, "$G", "builtin",
                                  "(Ljava/lang/String;)" DESC_OBJECT);
                break;
            }

            /* Check if declared nonlocal - look in closure */
            if (is_nonlocal(ctx, name) && ctx->closure_slot >= 0) {
                int captured_idx = get_captured_index(ctx, name);
                if (captured_idx >= 0) {
                    emit_aload(ctx, ctx->closure_slot);
                    emit_iconst(ctx, captured_idx);
                    emit_u8(ctx, OP_AALOAD);
                    stack_push(ctx, 1);
                    stack_pop(ctx, 1);
                    break;
                }
            }

            /* At module level, names are globals unless they're function params */
            if (ctx->is_module_level) {
                emit_ldc_string(ctx, name);
                emit_invokestatic(ctx, "$G", "builtin",
                                  "(Ljava/lang/String;)" DESC_OBJECT);
                break;
            }

            int slot = codegen_get_local(ctx, name);

            if (slot >= 0) {
                /* Local variable */
                emit_aload(ctx, slot);
            } else if (ctx->closure_slot >= 0) {
                /* Check if it's a captured variable */
                int captured_idx = get_captured_index(ctx, name);
                if (captured_idx >= 0) {
                    /* Load from closure array: closure[captured_idx] */
                    emit_aload(ctx, ctx->closure_slot);
                    emit_iconst(ctx, captured_idx);
                    emit_u8(ctx, OP_AALOAD);
                    stack_push(ctx, 1);
                    /* AALOAD: pops array + index, pushes element */
                    /* We pushed 2 (aload + iconst) then AALOAD effectively gives us 1 */
                    stack_pop(ctx, 1);
                } else {
                    /* Not captured, try global/built-in */
                    emit_ldc_string(ctx, name);
                    emit_invokestatic(ctx, "$G", "builtin",
                                      "(Ljava/lang/String;)" DESC_OBJECT);
                }
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
            stackmap_track_indy(ctx, op);
            stack_pop(ctx, 2);
            stack_push(ctx, 1);
            break;
        }

        /* Unary operations */
        case AST_UNARY_OP: {
            codegen_expr(ctx, node->data.unary_op.operand);

            py_indy_op_t op = unaryop_to_indy(node->data.unary_op.op);
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, op, NULL, 0);
            stackmap_track_indy(ctx, op);
            /* Stack: operand -> result */
            break;
        }

        /* Comparison operations (with chain support: a < b < c) */
        case AST_COMPARE: {
            slist_t *ops = node->data.compare.ops;
            slist_t *comps = node->data.compare.comparators;

            if (!ops || !comps) break;

            /* Count comparisons */
            int num_ops = 0;
            for (slist_t *o = ops; o; o = o->next) num_ops++;

            if (num_ops == 1) {
                /* Single comparison: simple case */
                cmp_op_t *op = ops->data;
                ast_node_t *right = comps->data;

                codegen_expr(ctx, node->data.compare.left);
                codegen_expr(ctx, right);

                /* For 'in' and 'not in', swap operands */
                if (*op == CMPOP_IN || *op == CMPOP_NOTIN) {
                    emit_u8(ctx, OP_SWAP);
                }

                py_indy_op_t indy_op = cmpop_to_indy(*op);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, indy_op, NULL, 0);
                stackmap_track_indy(ctx, indy_op);
                stack_pop(ctx, 2);
                stack_push(ctx, 1);
            } else {
                /* Comparison chain: a < b < c means (a < b) and (b < c)
                 * with b evaluated only once and short-circuit on false */
                label_t *end_label = codegen_new_label(ctx);

                /* Evaluate left operand */
                codegen_expr(ctx, node->data.compare.left);

                slist_t *cur_op = ops;
                slist_t *cur_comp = comps;

                while (cur_op && cur_comp) {
                    cmp_op_t *op = cur_op->data;
                    ast_node_t *right = cur_comp->data;
                    bool is_last = (cur_op->next == NULL);

                    /* Evaluate right operand */
                    codegen_expr(ctx, right);

                    if (!is_last) {
                        /* Not last comparison: need to save right for next comparison */
                        /* Stack: [left, right] -> [right, left, right] */
                        emit_u8(ctx, OP_DUP_X1);
                        stack_push(ctx, 1);
                        if (ctx->stackmap) {
                            const_pool_t *cp = class_writer_get_cp(ctx->cw);
                            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
                        }
                    }

                    /* For 'in' and 'not in', swap operands */
                    if (*op == CMPOP_IN || *op == CMPOP_NOTIN) {
                        emit_u8(ctx, OP_SWAP);
                    }

                    /* Perform comparison */
                    py_indy_op_t indy_op = cmpop_to_indy(*op);
                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, indy_op, NULL, 0);
                    stackmap_track_indy(ctx, indy_op);
                    stack_pop(ctx, 2);
                    stack_push(ctx, 1);
                    /* Stack after compare: [saved_right..., result] */

                    if (!is_last) {
                        /* Check result and short-circuit if false */
                        /* Stack: [saved_right, result] */
                        emit_u8(ctx, OP_DUP);
                        stack_push(ctx, 1);
                        if (ctx->stackmap) {
                            const_pool_t *cp = class_writer_get_cp(ctx->cw);
                            stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
                        }

                        /* Convert to int for branching */
                        indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                            PY_INDY_BOOL, NULL, 0);
                        stackmap_track_indy(ctx, PY_INDY_BOOL);

                        /* If false (0), short-circuit: swap and pop saved_right, jump */
                        label_t *continue_label = codegen_new_label(ctx);
                        codegen_emit_jump(ctx, OP_IFNE, continue_label);
                        stack_pop(ctx, 1);
                        if (ctx->stackmap) {
                            stackmap_pop(ctx->stackmap, 1);
                        }

                        /* False path: stack has [saved_right, false_result]
                         * Need to drop saved_right and keep false_result */
                        emit_u8(ctx, OP_SWAP);
                        emit_u8(ctx, OP_POP);
                        stack_pop(ctx, 1);
                        if (ctx->stackmap) {
                            stackmap_pop(ctx->stackmap, 1);
                        }
                        codegen_emit_jump(ctx, OP_GOTO, end_label);

                        /* True path continues: drop result, keep saved_right for next cmp */
                        codegen_mark_label(ctx, continue_label);
                        emit_u8(ctx, OP_POP);
                        stack_pop(ctx, 1);
                        if (ctx->stackmap) {
                            stackmap_pop(ctx->stackmap, 1);
                        }
                        /* Stack: [saved_right] which becomes left for next comparison */
                    }

                    cur_op = cur_op->next;
                    cur_comp = cur_comp->next;
                }

                codegen_mark_label(ctx, end_label);
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
                if (ctx->stackmap) {
                    const_pool_t *cp = class_writer_get_cp(ctx->cw);
                    stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
                }

                /* Convert to boolean for branching */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_BOOL, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_BOOL);

                /* For 'or': if true, short-circuit (keep value, jump to end)
                   For 'and': if false, short-circuit */
                if (is_or) {
                    codegen_emit_jump(ctx, OP_IFNE, end_label);
                } else {
                    codegen_emit_jump(ctx, OP_IFEQ, end_label);
                }
                stack_pop(ctx, 1);
                /* ifeq/ifne consumes the int from the stack */
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }

                /* Pop the duplicated value (we'll use next one) */
                emit_u8(ctx, OP_POP);
                stack_pop(ctx, 1);
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }

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
            stackmap_track_indy(ctx, PY_INDY_BOOL);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, else_label);
            stack_pop(ctx, 1);
            /* ifeq consumes the int */
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

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
            stackmap_track_indy(ctx, PY_INDY_GETATTR);
            /* Stack: obj -> attr_value */
            break;
        }

        /* Subscript: obj[key] */
        case AST_SUBSCRIPT: {
            codegen_expr(ctx, node->data.subscript.value);
            codegen_expr(ctx, node->data.subscript.slice);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_GETITEM, NULL, 0);
            stackmap_track_indy(ctx, PY_INDY_GETITEM);
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
            stackmap_track_indy(ctx, PY_INDY_CALL);
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

        /* Set literal */
        case AST_SET: {
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

            /* Call $ST.of(Object[]) */
            emit_invokestatic(ctx, LRT_SET, "of", "(" DESC_OBJECT_ARR ")" DESC_SET);
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

        /* Slice: lower:upper:step -> $SL(start, stop, step) */
        case AST_SLICE: {
            /* new $SL */
            emit_new(ctx, LRT_SLICE);
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);  /* DUP adds one to stack */

            /* Push start (or None if absent) */
            if (node->data.slice.lower) {
                codegen_expr(ctx, node->data.slice.lower);
            } else {
                emit_py_none(ctx);
            }

            /* Push stop (or None if absent) */
            if (node->data.slice.upper) {
                codegen_expr(ctx, node->data.slice.upper);
            } else {
                emit_py_none(ctx);
            }

            /* Push step (or None if absent) */
            if (node->data.slice.step) {
                codegen_expr(ctx, node->data.slice.step);
            } else {
                emit_py_none(ctx);
            }

            /* Call constructor: $SL.<init>($O, $O, $O)V */
            emit_invokespecial(ctx, LRT_SLICE, "<init>",
                               "(" DESC_OBJECT DESC_OBJECT DESC_OBJECT ")V");
            stack_pop(ctx, 4);  /* Pop dup + 3 args, constructor consumes them */
            /* Stack: $SL object ref (from new) */
            break;
        }

        /* List comprehension: [expr for target in iter if cond] */
        case AST_LIST_COMP: {
            ast_node_t *elt = node->data.comprehension_expr.elt;
            slist_t *generators = node->data.comprehension_expr.generators;

            /* Create empty list and store in temp local */
            emit_new(ctx, LRT_LIST);
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            emit_invokespecial(ctx, LRT_LIST, "<init>", "()V");
            stack_pop(ctx, 1);
            int result_slot = codegen_alloc_local(ctx, "$comp_result");
            emit_astore(ctx, result_slot);

            /* Process each generator (nested loops) */
            codegen_comprehension_loop(ctx, generators, elt, result_slot, COMP_LIST);

            /* Load result */
            emit_aload(ctx, result_slot);
            break;
        }

        /* Set comprehension: {expr for target in iter if cond} */
        case AST_SET_COMP: {
            ast_node_t *elt = node->data.comprehension_expr.elt;
            slist_t *generators = node->data.comprehension_expr.generators;

            /* Create empty set and store in temp local */
            emit_new(ctx, LRT_SET);
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            emit_invokespecial(ctx, LRT_SET, "<init>", "()V");
            stack_pop(ctx, 1);
            int result_slot = codegen_alloc_local(ctx, "$comp_result");
            emit_astore(ctx, result_slot);

            /* Process each generator */
            codegen_comprehension_loop(ctx, generators, elt, result_slot, COMP_SET);

            /* Load result */
            emit_aload(ctx, result_slot);
            break;
        }

        /* Dict comprehension: {key: value for target in iter if cond} */
        case AST_DICT_COMP: {
            ast_node_t *key_expr = node->data.dict_comp.key;
            ast_node_t *value_expr = node->data.dict_comp.value;
            slist_t *generators = node->data.dict_comp.generators;

            /* Create empty dict and store in temp local */
            emit_new(ctx, LRT_DICT);
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            emit_invokespecial(ctx, LRT_DICT, "<init>", "()V");
            stack_pop(ctx, 1);
            int result_slot = codegen_alloc_local(ctx, "$comp_result");
            emit_astore(ctx, result_slot);

            /* Process generators - for dict, we need to handle key/value pair */
            codegen_dict_comprehension_loop(ctx, generators, key_expr, value_expr, result_slot);

            /* Load result */
            emit_aload(ctx, result_slot);
            break;
        }

        /* Lambda expression: lambda args: body */
        case AST_LAMBDA:
            codegen_lambda(ctx, node);
            break;

        /* Generator expression: (expr for target in iter if cond) */
        case AST_GENERATOR_EXP: {
            ast_node_t *elt = node->data.comprehension_expr.elt;
            slist_t *generators = node->data.comprehension_expr.generators;

            if (!generators) {
                emit_aconst_null(ctx);
                break;
            }

            /* Get the first (and for now, only supported) generator */
            ast_node_t *gen = generators->data;
            ast_node_t *target = gen->data.comprehension.target;
            ast_node_t *iter_expr = gen->data.comprehension.iter;
            slist_t *ifs = gen->data.comprehension.ifs;

            /* For nested generators, we'd need to flatten - for now, handle single */
            if (generators->next) {
                /* Nested generators - fall back to eager evaluation into a list,
                 * then wrap in an iterator. Not ideal but works. */
                emit_new(ctx, LRT_LIST);
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_invokespecial(ctx, LRT_LIST, "<init>", "()V");
                stack_pop(ctx, 1);
                int result_slot = codegen_alloc_local(ctx, "$genexp_result");
                emit_astore(ctx, result_slot);
                codegen_comprehension_loop(ctx, generators, elt, result_slot, COMP_LIST);
                emit_aload(ctx, result_slot);
                /* Get iterator from list */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_ITER, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_ITER);
                break;
            }

            /* Single generator - create lazy $GE */

            /* 1. Evaluate the source iterable */
            codegen_expr(ctx, iter_expr);

            /* 2. Create lambda for element expression: lambda target: elt */
            /* Build a synthetic lambda AST node */
            ast_node_t *mapper_arg = ast_new(AST_ARG, node->line, node->column);
            if (target->type == AST_NAME) {
                mapper_arg->data.arg.arg = target->data.name.id;
            } else {
                /* For tuple unpacking targets, use a placeholder */
                mapper_arg->data.arg.arg = "$item";
            }
            mapper_arg->data.arg.annotation = NULL;

            ast_node_t *mapper_args = ast_new(AST_ARGUMENTS, node->line, node->column);
            mapper_args->data.arguments.posonlyargs = NULL;
            mapper_args->data.arguments.args = slist_append(NULL, mapper_arg);
            mapper_args->data.arguments.vararg = NULL;
            mapper_args->data.arguments.kwonlyargs = NULL;
            mapper_args->data.arguments.kw_defaults = NULL;
            mapper_args->data.arguments.kwarg = NULL;
            mapper_args->data.arguments.defaults = NULL;

            ast_node_t *mapper_lambda = ast_new(AST_LAMBDA, node->line, node->column);
            mapper_lambda->data.lambda.args = mapper_args;
            mapper_lambda->data.lambda.body = elt;

            codegen_lambda(ctx, mapper_lambda);
            /* Stack: [source, mapper] */

            /* 3. Create filter lambdas if any */
            int num_filters = slist_length(ifs);

            if (num_filters == 0) {
                /* No filters - call $GE.of(source, mapper) */
                emit_invokestatic(ctx, LRT_GENEXP, "of",
                                  "(L$O;L$MH;)L$GE;");
                stack_pop(ctx, 2);
                stack_push(ctx, 1);
            } else if (num_filters == 1) {
                /* Single filter - create filter lambda */
                ast_node_t *filter_lambda = ast_new(AST_LAMBDA, node->line, node->column);
                filter_lambda->data.lambda.args = mapper_args;  /* Same args */
                filter_lambda->data.lambda.body = ifs->data;

                codegen_lambda(ctx, filter_lambda);
                /* Stack: [source, mapper, filter] */

                emit_invokestatic(ctx, LRT_GENEXP, "of",
                                  "(L$O;L$MH;L$MH;)L$GE;");
                stack_pop(ctx, 3);
                stack_push(ctx, 1);
            } else {
                /* Multiple filters - create array of filter lambdas */
                emit_iconst(ctx, num_filters);
                emit_anewarray(ctx, "$MH");

                int i = 0;
                for (slist_t *cond = ifs; cond; cond = cond->next) {
                    emit_u8(ctx, OP_DUP);
                    stack_push(ctx, 1);
                    emit_iconst(ctx, i);

                    ast_node_t *filter_lambda = ast_new(AST_LAMBDA, node->line, node->column);
                    filter_lambda->data.lambda.args = mapper_args;
                    filter_lambda->data.lambda.body = cond->data;
                    codegen_lambda(ctx, filter_lambda);

                    emit_u8(ctx, OP_AASTORE);
                    stack_pop(ctx, 3);
                    i++;
                }
                /* Stack: [source, mapper, filters[]] */

                emit_invokestatic(ctx, LRT_GENEXP, "of",
                                  "(L$O;L$MH;[L$MH;)L$GE;");
                stack_pop(ctx, 3);
                stack_push(ctx, 1);
            }

            /* Note: We're not freeing the synthetic AST nodes - they'll be
             * cleaned up when the whole AST is freed. This is acceptable
             * for a compiler. */
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

                    /* Check if declared global or at module level */
                    if (is_global(ctx, name) || ctx->is_module_level) {
                        /* Store via $G.setGlobal(name, value) */
                        /* Stack: value */
                        emit_ldc_string(ctx, name);  /* Stack: value, name */
                        emit_u8(ctx, OP_SWAP);       /* Stack: name, value */
                        emit_invokestatic(ctx, "$G", "setGlobal",
                                          "(Ljava/lang/String;L$O;)V");
                        stack_pop(ctx, 2);  /* setGlobal consumes both */

                        /* Also keep a local copy for faster access at module level */
                        if (ctx->is_module_level && !is_global(ctx, name)) {
                            /* Re-evaluate the value for local copy */
                            /* Actually, value was already consumed - need to reload */
                            /* For simplicity, just use globals at module level */
                        }
                    } else {
                        int slot = codegen_get_local(ctx, name);
                        if (slot < 0) {
                            /* Allocate new local */
                            slot = codegen_alloc_local(ctx, name);
                        }
                        emit_astore(ctx, slot);
                    }
                } else if (target->type == AST_SUBSCRIPT) {
                    /* obj[key] = value */
                    /* Stack: value */
                    /* Need: obj, key, value on stack */
                    codegen_expr(ctx, target->data.subscript.value);  /* obj */
                    emit_u8(ctx, OP_SWAP);
                    codegen_expr(ctx, target->data.subscript.slice);  /* key */
                    emit_u8(ctx, OP_SWAP);

                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_SETITEM, NULL, 0);
                    stackmap_track_indy(ctx, PY_INDY_SETITEM);
                    stack_pop(ctx, 3);
                } else if (target->type == AST_ATTRIBUTE) {
                    /* obj.attr = value */
                    codegen_expr(ctx, target->data.attribute.value);  /* obj */
                    emit_u8(ctx, OP_SWAP);

                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_SETATTR,
                                        target->data.attribute.attr, 0);
                    stackmap_track_indy(ctx, PY_INDY_SETATTR);
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
                stackmap_track_indy(ctx, op);
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
            stackmap_track_indy(ctx, PY_INDY_BOOL);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, else_label);
            stack_pop(ctx, 1);
            /* ifeq consumes the int */
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

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
            stackmap_track_indy(ctx, PY_INDY_BOOL);

            /* Branch if false */
            codegen_emit_jump(ctx, OP_IFEQ, end_label);
            stack_pop(ctx, 1);
            /* ifeq consumes the int */
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

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
            stackmap_track_indy(ctx, PY_INDY_ITER);

            /* Store iterator in a temporary */
            int iter_slot = codegen_alloc_local(ctx, "$iter");
            emit_astore(ctx, iter_slot);

            /* Loop start */
            codegen_mark_label(ctx, start_label);

            /* Get next item */
            emit_aload(ctx, iter_slot);

            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache, PY_INDY_NEXT, NULL, 0);
            stackmap_track_indy(ctx, PY_INDY_NEXT);

            /* Check for StopIteration (null return) */
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            if (ctx->stackmap) {
                const_pool_t *cp = class_writer_get_cp(ctx->cw);
                stackmap_push_object(ctx->stackmap, cp, LRT_OBJECT);
            }
            codegen_emit_jump(ctx, OP_IFNULL, end_label);
            stack_pop(ctx, 1);
            /* ifnull consumes one reference from stack */
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

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
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }
            }

            /* Body */
            codegen_stmts(ctx, node->data.for_stmt.body);

            /* Loop back */
            codegen_emit_jump(ctx, OP_GOTO, start_label);

            /* End */
            codegen_mark_label(ctx, end_label);
            emit_u8(ctx, OP_POP);  /* Pop the null from iterator */
            stack_pop(ctx, 1);
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

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

        /* With statement (context manager) */
        case AST_WITH: {
            slist_t *items = node->data.with_stmt.items;
            slist_t *body = node->data.with_stmt.body;

            /* For each with item */
            for (slist_t *item = items; item; item = item->next) {
                ast_node_t *with_item = item->data;
                ast_node_t *context_expr = with_item->data.with_item.context_expr;
                ast_node_t *optional_vars = with_item->data.with_item.optional_vars;

                /* Evaluate context manager */
                codegen_expr(ctx, context_expr);

                /* Store context manager in temp local */
                int mgr_slot = codegen_alloc_local(ctx, "$ctx_mgr");
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_astore(ctx, mgr_slot);

                /* Call __enter__() */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_GETATTR, "__enter__", 0);
                stackmap_track_indy(ctx, PY_INDY_GETATTR);

                /* Call the bound method with no args */
                emit_iconst(ctx, 0);
                emit_anewarray(ctx, LRT_OBJECT);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_CALL, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_CALL);
                stack_pop(ctx, 1);

                /* Bind result to optional_vars if present */
                if (optional_vars && optional_vars->type == AST_NAME) {
                    const char *var_name = optional_vars->data.name.id;
                    int var_slot = codegen_get_local(ctx, var_name);
                    if (var_slot < 0) {
                        var_slot = codegen_alloc_local(ctx, var_name);
                    }
                    emit_astore(ctx, var_slot);
                } else {
                    /* No binding, discard __enter__ result */
                    emit_u8(ctx, OP_POP);
                    stack_pop(ctx, 1);
                }

                /* Record try start */
                uint16_t try_start = (uint16_t)ctx->code->len;

                /* Generate body (only for last item to avoid nesting complexity) */
                if (!item->next) {
                    codegen_stmts(ctx, body);
                }

                /* Record try end */
                uint16_t try_end = (uint16_t)ctx->code->len;

                /* Jump past handler */
                label_t *after_finally = codegen_new_label(ctx);
                codegen_emit_jump(ctx, OP_GOTO, after_finally);

                /* Exception handler - call __exit__ and re-raise */
                label_t *handler_label = codegen_new_label(ctx);
                codegen_mark_label(ctx, handler_label);
                uint16_t handler_pc = (uint16_t)ctx->code->len;

                /* Exception is on stack */
                stack_push(ctx, 1);
                if (ctx->stackmap) {
                    const_pool_t *cp = class_writer_get_cp(ctx->cw);
                    stackmap_push_object(ctx->stackmap, cp, "$X");
                }

                /* Store exception */
                int exc_slot = codegen_alloc_local(ctx, "$exc");
                emit_astore(ctx, exc_slot);

                /* Call __exit__(exc_type, exc_val, exc_tb) - simplified to (None, exc, None) */
                emit_aload(ctx, mgr_slot);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_GETATTR, "__exit__", 0);
                stackmap_track_indy(ctx, PY_INDY_GETATTR);

                /* Create args array [None, exc, None] */
                emit_iconst(ctx, 3);
                emit_anewarray(ctx, LRT_OBJECT);
                /* args[0] = None */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 0);
                emit_py_none(ctx);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                /* args[1] = exc */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 1);
                emit_aload(ctx, exc_slot);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                /* args[2] = None */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 2);
                emit_py_none(ctx);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);

                /* Call __exit__ */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_CALL, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_CALL);
                stack_pop(ctx, 1);

                /* Check if __exit__ returned True (suppress exception) */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_BOOL, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_BOOL);
                label_t *suppress = codegen_new_label(ctx);
                codegen_emit_jump(ctx, OP_IFNE, suppress);
                stack_pop(ctx, 1);
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }

                /* Re-raise exception */
                emit_aload(ctx, exc_slot);
                emit_u8(ctx, OP_ATHROW);
                stack_pop(ctx, 1);

                /* Exception suppressed - continue */
                codegen_mark_label(ctx, suppress);

                /* Normal exit - call __exit__(None, None, None) */
                codegen_mark_label(ctx, after_finally);

                emit_aload(ctx, mgr_slot);
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_GETATTR, "__exit__", 0);
                stackmap_track_indy(ctx, PY_INDY_GETATTR);

                /* Create args array [None, None, None] */
                emit_iconst(ctx, 3);
                emit_anewarray(ctx, LRT_OBJECT);
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 0);
                emit_py_none(ctx);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 1);
                emit_py_none(ctx);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_iconst(ctx, 2);
                emit_py_none(ctx);
                emit_u8(ctx, OP_AASTORE);
                stack_pop(ctx, 3);

                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_CALL, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_CALL);
                stack_pop(ctx, 1);

                /* Discard __exit__ return value */
                emit_u8(ctx, OP_POP);
                stack_pop(ctx, 1);

                /* Add exception table entry */
                const_pool_t *cp = class_writer_get_cp(ctx->cw);
                uint16_t catch_type = cp_add_class(cp, "$X");
                code_attr_add_exception(ctx->code_attr, try_start, try_end,
                                        handler_pc, catch_type);
            }
            break;
        }

        /* Import statement: import foo, import foo.bar, import foo as bar */
        case AST_IMPORT: {
            for (slist_t *s = node->data.import_stmt.names; s; s = s->next) {
                ast_node_t *alias = s->data;
                const char *module_name = alias->data.alias.name;
                const char *as_name = alias->data.alias.asname;
                if (!as_name) as_name = module_name;

                /* For dotted imports like "import foo.bar.baz", the as_name
                   should be just "foo" (the top-level module) unless explicitly aliased */
                if (!alias->data.alias.asname) {
                    /* Find first component before dot */
                    const char *dot = strchr(module_name, '.');
                    if (dot) {
                        /* We import "foo.bar.baz" but bind just "foo" */
                        size_t len = dot - module_name;
                        char *top_name = alloca(len + 1);
                        strncpy(top_name, module_name, len);
                        top_name[len] = '\0';
                        as_name = top_name;
                    }
                }

                /* Call $G.importModule(module_name) */
                emit_ldc_string(ctx, module_name);
                emit_invokestatic(ctx, "$G", "importModule",
                                  "(Ljava/lang/String;)L" LRT_MODULE ";");
                stack_push(ctx, 1);

                /* Store as global: $G.setGlobal(as_name, module) */
                emit_ldc_string(ctx, as_name);
                emit_u8(ctx, OP_SWAP);
                emit_invokestatic(ctx, "$G", "setGlobal",
                                  "(Ljava/lang/String;L$O;)V");
                stack_pop(ctx, 2);
            }
            break;
        }

        /* From import statement: from foo import bar, from foo import * */
        case AST_IMPORT_FROM: {
            const char *module_name = node->data.import_from.module;
            slist_t *names = node->data.import_from.names;

            if (!module_name) {
                /* Relative import without module name - not supported yet */
                fprintf(stderr, "Error: relative imports not fully supported\n");
                break;
            }

            /* Import the module first */
            emit_ldc_string(ctx, module_name);
            emit_invokestatic(ctx, "$G", "importModule",
                              "(Ljava/lang/String;)L" LRT_MODULE ";");
            stack_push(ctx, 1);

            /* Now get each name from the module */
            for (slist_t *s = names; s; s = s->next) {
                ast_node_t *alias = s->data;
                const char *name = alias->data.alias.name;
                const char *as_name = alias->data.alias.asname;
                if (!as_name) as_name = name;

                if (strcmp(name, "*") == 0) {
                    /* from foo import * - get all public names */
                    /* For now, just skip (complex implementation) */
                    fprintf(stderr, "Warning: 'from %s import *' not fully implemented\n",
                            module_name);
                    continue;
                }

                /* DUP module reference */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);

                /* Call module.getAttr(name) */
                emit_ldc_string(ctx, name);
                emit_invokevirtual(ctx, LRT_MODULE, "getAttr",
                                   "(Ljava/lang/String;)L$O;");
                /* Stack: module, value */

                /* Store as global: $G.setGlobal(as_name, value) */
                emit_ldc_string(ctx, as_name);
                emit_u8(ctx, OP_SWAP);
                emit_invokestatic(ctx, "$G", "setGlobal",
                                  "(Ljava/lang/String;L$O;)V");
                stack_pop(ctx, 2);
            }

            /* Pop the module reference */
            emit_u8(ctx, OP_POP);
            stack_pop(ctx, 1);
            break;
        }

        /* Global declaration */
        case AST_GLOBAL: {
            for (slist_t *s = node->data.global_stmt.names; s; s = s->next) {
                const char *name = (const char *)s->data;
                /* Check if already in list */
                bool found = false;
                for (slist_t *g = ctx->global_names; g; g = g->next) {
                    if (strcmp((const char *)g->data, name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ctx->global_names = slist_append(ctx->global_names, (void *)name);
                }
            }
            break;
        }

        /* Nonlocal declaration */
        case AST_NONLOCAL: {
            for (slist_t *s = node->data.global_stmt.names; s; s = s->next) {
                const char *name = (const char *)s->data;
                /* Check if already in list */
                bool found = false;
                for (slist_t *n = ctx->nonlocal_names; n; n = n->next) {
                    if (strcmp((const char *)n->data, name) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    ctx->nonlocal_names = slist_append(ctx->nonlocal_names, (void *)name);
                }
            }
            break;
        }

        /* Function definition */
        case AST_FUNCTION_DEF:
            codegen_function_def(ctx, node);
            break;

        /* Try/except/finally statement */
        case AST_TRY: {
            slist_t *body = node->data.try_stmt.body;
            slist_t *handlers = node->data.try_stmt.handlers;
            slist_t *orelse = node->data.try_stmt.orelse;
            slist_t *finalbody = node->data.try_stmt.finalbody;

            label_t *after_handlers = codegen_new_label(ctx);

            /* Record start of try block */
            uint16_t try_start_pc = (uint16_t)ctx->code->len;

            /* Generate try body */
            codegen_stmts(ctx, body);

            /* Record end of try block */
            uint16_t try_end_pc = (uint16_t)ctx->code->len;

            /* If no exception, jump past handlers */
            codegen_emit_jump(ctx, OP_GOTO, after_handlers);

            /* Generate exception handlers */
            for (slist_t *h = handlers; h; h = h->next) {
                ast_node_t *handler = h->data;
                ast_node_t *exc_type = handler->data.except_handler.type;
                const char *exc_name = handler->data.except_handler.name;
                slist_t *handler_body = handler->data.except_handler.body;

                /* Mark handler start */
                label_t *handler_label = codegen_new_label(ctx);
                codegen_mark_label(ctx, handler_label);
                uint16_t handler_pc = (uint16_t)ctx->code->len;

                /* Exception is on stack - need to handle it */
                stack_push(ctx, 1);  /* Exception pushed by JVM */
                if (ctx->stackmap) {
                    const_pool_t *cp = class_writer_get_cp(ctx->cw);
                    stackmap_push_object(ctx->stackmap, cp, "$X");
                }

                if (exc_type) {
                    /* Typed handler: except SomeError as e: */
                    /* TODO: Check if exception matches type */
                    /* For now, catch all $X and check type at runtime */

                    if (exc_name) {
                        /* Bind exception to name */
                        int exc_slot = codegen_get_local(ctx, exc_name);
                        if (exc_slot < 0) {
                            exc_slot = codegen_alloc_local(ctx, exc_name);
                        }
                        emit_astore(ctx, exc_slot);
                    } else {
                        /* No binding, discard exception */
                        emit_u8(ctx, OP_POP);
                        stack_pop(ctx, 1);
                        if (ctx->stackmap) {
                            stackmap_pop(ctx->stackmap, 1);
                        }
                    }
                } else {
                    /* Bare except: - catch all */
                    if (exc_name) {
                        int exc_slot = codegen_get_local(ctx, exc_name);
                        if (exc_slot < 0) {
                            exc_slot = codegen_alloc_local(ctx, exc_name);
                        }
                        emit_astore(ctx, exc_slot);
                    } else {
                        emit_u8(ctx, OP_POP);
                        stack_pop(ctx, 1);
                        if (ctx->stackmap) {
                            stackmap_pop(ctx->stackmap, 1);
                        }
                    }
                }

                /* Generate handler body */
                codegen_stmts(ctx, handler_body);

                /* Jump to after all handlers */
                codegen_emit_jump(ctx, OP_GOTO, after_handlers);

                /* Add exception table entry */
                /* catch_type 0 = catch all, otherwise need class index */
                const_pool_t *cp = class_writer_get_cp(ctx->cw);
                uint16_t catch_type = cp_add_class(cp, "$X");
                code_attr_add_exception(ctx->code_attr, try_start_pc, try_end_pc,
                                        handler_pc, catch_type);
            }

            /* Mark after handlers */
            codegen_mark_label(ctx, after_handlers);

            /* Generate else block if present (runs if no exception) */
            if (orelse) {
                codegen_stmts(ctx, orelse);
            }

            /* Generate finally block if present */
            if (finalbody) {
                /* For now, just generate the finally code inline
                 * A proper implementation would also run finally on exception */
                codegen_stmts(ctx, finalbody);
            }

            break;
        }

        /* Raise statement: raise [exc [from cause]] */
        case AST_RAISE: {
            ast_node_t *exc = node->data.raise_stmt.exc;
            ast_node_t *cause = node->data.raise_stmt.cause;

            if (exc) {
                /* raise exc - evaluate and throw */
                codegen_expr(ctx, exc);

                /* If it's a class (type), instantiate it */
                /* For now, assume it's already an exception instance or $X */

                /* Check if it's a $X, if not wrap it */
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_instanceof(ctx, "$X");
                label_t *is_exception = codegen_new_label(ctx);
                codegen_emit_jump(ctx, OP_IFNE, is_exception);
                stack_pop(ctx, 1);
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }

                /* Not a $X - convert to string and wrap */
                /* Stack: [obj] */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_STR, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_STR);

                /* Create $X("Exception", str) */
                emit_new(ctx, "$X");
                emit_u8(ctx, OP_DUP_X1);
                stack_push(ctx, 1);
                emit_u8(ctx, OP_SWAP);
                emit_ldc_string(ctx, "Exception");
                emit_u8(ctx, OP_SWAP);
                /* Stack: [$X, $X, "Exception", str] */
                emit_invokespecial(ctx, "$X", "<init>",
                                   "(Ljava/lang/String;Ljava/lang/String;)V");
                stack_pop(ctx, 3);
                /* Stack: [$X] */
                label_t *do_throw = codegen_new_label(ctx);
                codegen_emit_jump(ctx, OP_GOTO, do_throw);

                codegen_mark_label(ctx, is_exception);
                /* Already a $X, just use it */
                /* Stack: [$X, int_result] - pop the instanceof result */
                emit_u8(ctx, OP_POP);
                stack_pop(ctx, 1);
                if (ctx->stackmap) {
                    stackmap_pop(ctx->stackmap, 1);
                }

                codegen_mark_label(ctx, do_throw);

                /* Handle 'from cause' if present */
                if (cause) {
                    /* TODO: Set __cause__ on the exception */
                    /* For now, just evaluate and discard */
                    codegen_expr(ctx, cause);
                    emit_u8(ctx, OP_POP);
                    stack_pop(ctx, 1);
                }

                /* Throw */
                emit_checkcast(ctx, "$X");
                emit_u8(ctx, OP_ATHROW);
                stack_pop(ctx, 1);
            } else {
                /* Bare raise - re-raise current exception */
                /* This requires exception context which we don't track yet */
                /* For now, raise a generic error */
                emit_new(ctx, "$X");
                emit_u8(ctx, OP_DUP);
                stack_push(ctx, 1);
                emit_ldc_string(ctx, "RuntimeError");
                emit_ldc_string(ctx, "No active exception to re-raise");
                emit_invokespecial(ctx, "$X", "<init>",
                                   "(Ljava/lang/String;Ljava/lang/String;)V");
                stack_pop(ctx, 2);
                emit_u8(ctx, OP_ATHROW);
                stack_pop(ctx, 1);
            }
            break;
        }

        /* Assert statement: assert test [, msg] */
        case AST_ASSERT: {
            ast_node_t *test = node->data.assert_stmt.test;
            ast_node_t *msg = node->data.assert_stmt.msg;

            /* Evaluate test condition */
            codegen_expr(ctx, test);

            /* Convert to bool (int) */
            indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                PY_INDY_BOOL, NULL, 0);
            stackmap_track_indy(ctx, PY_INDY_BOOL);

            /* If true (non-zero), skip the error */
            label_t *skip_label = codegen_new_label(ctx);
            codegen_emit_jump(ctx, OP_IFNE, skip_label);
            stack_pop(ctx, 1);
            if (ctx->stackmap) {
                stackmap_pop(ctx->stackmap, 1);
            }

            /* Assertion failed - throw AssertionError */
            emit_new(ctx, "$X");
            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);

            /* Push type and message */
            emit_ldc_string(ctx, "AssertionError");
            if (msg) {
                codegen_expr(ctx, msg);
                /* Convert message to string */
                indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                    PY_INDY_STR, NULL, 0);
                stackmap_track_indy(ctx, PY_INDY_STR);
            } else {
                emit_ldc_string(ctx, "assertion failed");
            }

            /* Call $X.<init>(String, String) */
            emit_invokespecial(ctx, "$X", "<init>",
                               "(Ljava/lang/String;Ljava/lang/String;)V");
            stack_pop(ctx, 3);

            /* Throw the exception */
            emit_u8(ctx, OP_ATHROW);

            /* Mark skip label */
            codegen_mark_label(ctx, skip_label);
            break;
        }

        /* Delete statement: del x, del d[key], del obj.attr */
        case AST_DELETE: {
            for (slist_t *t = node->data.delete_stmt.targets; t; t = t->next) {
                ast_node_t *target = t->data;

                if (target->type == AST_NAME) {
                    /* del x - set local to null (Python semantics: unbind name) */
                    const char *name = target->data.name.id;
                    int slot = codegen_get_local(ctx, name);
                    if (slot >= 0) {
                        emit_aconst_null(ctx);
                        emit_astore(ctx, slot);
                    }
                    /* If not a local, it would be a global - not yet supported */
                } else if (target->type == AST_SUBSCRIPT) {
                    /* del d[key] - call __delitem__ */
                    codegen_expr(ctx, target->data.subscript.value);  /* container */
                    codegen_expr(ctx, target->data.subscript.slice);  /* key */
                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                        PY_INDY_DELITEM, NULL, 0);
                    stackmap_track_indy(ctx, PY_INDY_DELITEM);
                    stack_pop(ctx, 2);
                } else if (target->type == AST_ATTRIBUTE) {
                    /* del obj.attr - call __delattr__ */
                    codegen_expr(ctx, target->data.attribute.value);  /* object */
                    const char *attr = target->data.attribute.attr;
                    indy_emit_operation(ctx->cw, ctx->code, ctx->indy_cache,
                                        PY_INDY_DELATTR, attr, 0);
                    stackmap_track_indy(ctx, PY_INDY_DELATTR);
                    stack_pop(ctx, 1);
                }
            }
            break;
        }

        /* Class definition */
        case AST_CLASS_DEF: {
            const char *class_name = node->data.class_def.name;
            slist_t *bases = node->data.class_def.bases;
            slist_t *body = node->data.class_def.body;

            /* Set class context for method name uniqueness */
            const char *prev_class_name = ctx->current_class_name;
            ctx->current_class_name = class_name;

            /* Count base classes */
            int num_bases = slist_length(bases);

            if (num_bases == 0) {
                /* No bases - call $Cls.of(String) */
                emit_ldc_string(ctx, class_name);
                emit_invokestatic(ctx, LRT_CLASS, "of",
                                  "(Ljava/lang/String;)L" LRT_CLASS ";");
                stack_push(ctx, 1);
            } else {
                /* With bases - need to create array */
                emit_ldc_string(ctx, class_name);

                /* Create bases array */
                emit_iconst(ctx, num_bases);
                emit_anewarray(ctx, LRT_OBJECT);

                int i = 0;
                for (slist_t *b = bases; b; b = b->next) {
                    emit_u8(ctx, OP_DUP);
                    stack_push(ctx, 1);
                    emit_iconst(ctx, i);
                    codegen_expr(ctx, b->data);
                    emit_u8(ctx, OP_AASTORE);
                    stack_pop(ctx, 3);
                    i++;
                }

                /* Call $Cls.of(String, $O[]) */
                emit_invokestatic(ctx, LRT_CLASS, "of",
                                  "(Ljava/lang/String;[L$O;)L" LRT_CLASS ";");
                stack_pop(ctx, 2);
                stack_push(ctx, 1);
            }

            /* Stack: $Cls */

            /* Store class in a temp local while we populate it */
            int class_slot = codegen_alloc_local(ctx, "$class_temp");
            emit_astore(ctx, class_slot);

            /* Process class body - methods become class attributes */
            for (slist_t *s = body; s; s = s->next) {
                ast_node_t *stmt = s->data;

                if (stmt->type == AST_FUNCTION_DEF) {
                    /* Compile the method */
                    const char *method_name = stmt->data.func_def.name;

                    /* Generate the function (this stores it in a local) */
                    codegen_function_def(ctx, stmt);

                    /* Load class, load method, call setAttr */
                    emit_aload(ctx, class_slot);
                    int method_slot = codegen_get_local(ctx, method_name);
                    emit_aload(ctx, method_slot);

                    /* Call $Cls.setAttr(String, $O) */
                    emit_ldc_string(ctx, method_name);
                    emit_u8(ctx, OP_SWAP);  /* setAttr expects (name, value) */
                    emit_invokevirtual(ctx, LRT_CLASS, "setAttr",
                                       "(Ljava/lang/String;L$O;)V");
                    stack_pop(ctx, 2);
                } else if (stmt->type == AST_ASSIGN) {
                    /* Class variable assignment */
                    for (slist_t *t = stmt->data.assign.targets; t; t = t->next) {
                        ast_node_t *target = t->data;
                        if (target->type == AST_NAME) {
                            const char *attr_name = target->data.name.id;

                            emit_aload(ctx, class_slot);
                            emit_ldc_string(ctx, attr_name);
                            codegen_expr(ctx, stmt->data.assign.value);
                            emit_invokevirtual(ctx, LRT_CLASS, "setAttr",
                                               "(Ljava/lang/String;L$O;)V");
                            stack_pop(ctx, 2);
                        }
                    }
                } else if (stmt->type == AST_PASS) {
                    /* Ignore pass statements */
                } else if (stmt->type == AST_EXPR_STMT) {
                    /* Docstrings or other expressions - evaluate and discard */
                    codegen_expr(ctx, stmt->data.expr_stmt.value);
                    emit_u8(ctx, OP_POP);
                    stack_pop(ctx, 1);
                }
                /* Other statement types in class body not yet supported */
            }

            /* Load the class back and store */
            emit_aload(ctx, class_slot);

            /* At module level, store to globals */
            if (ctx->is_module_level) {
                emit_ldc_string(ctx, class_name);
                emit_u8(ctx, OP_SWAP);
                emit_invokestatic(ctx, "$G", "setGlobal",
                                  "(Ljava/lang/String;L$O;)V");
                stack_pop(ctx, 2);
            } else {
                int final_slot = codegen_get_local(ctx, class_name);
                if (final_slot < 0) {
                    final_slot = codegen_alloc_local(ctx, class_name);
                }
                emit_astore(ctx, final_slot);
            }

            /* Restore previous class context */
            ctx->current_class_name = prev_class_name;
            break;
        }

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
 * Function code generation
 * ======================================================================== */

/**
 * Build a JVM method descriptor for a Python function.
 * All parameters are $O and return is $O.
 * @param num_params Number of parameters (excluding closure)
 * @param has_closure If true, first param is $O[] for closure
 * @return Allocated descriptor string (caller must free)
 */
static char *build_method_descriptor_ex(int num_params, bool has_closure)
{
    /* Format: ([L$O;L$O;L$O;...)L$O; or (L$O;L$O;...)L$O; */
    /* Closure array takes 5 chars "[L$O;" */
    /* Each regular param takes 4 chars "L$O;" plus parens and return */
    size_t closure_size = has_closure ? 5 : 0;
    size_t len = 2 + closure_size + (num_params * 4) + 4 + 1;
    char *desc = malloc(len);
    if (!desc) {
        return NULL;
    }

    char *p = desc;
    *p++ = '(';
    
    /* Add closure array parameter if present */
    if (has_closure) {
        *p++ = '[';
        *p++ = 'L';
        *p++ = '$';
        *p++ = 'O';
        *p++ = ';';
    }
    
    /* Add regular parameters */
    for (int i = 0; i < num_params; i++) {
        *p++ = 'L';
        *p++ = '$';
        *p++ = 'O';
        *p++ = ';';
    }
    
    /* Return type */
    *p++ = ')';
    *p++ = 'L';
    *p++ = '$';
    *p++ = 'O';
    *p++ = ';';
    *p = '\0';

    return desc;
}

/**
 * Build a JVM method descriptor for a Python function.
 * All parameters are $O and return is $O.
 * @param num_params Number of parameters
 * @return Allocated descriptor string (caller must free)
 */
static char *build_method_descriptor(int num_params)
{
    /* Format: (L$O;L$O;...)L$O; */
    /* Each param takes 4 chars "L$O;" plus parens and return = 5 + 4*n */
    size_t len = 2 + (num_params * 4) + 4 + 1;  /* () + params + return + null */
    char *desc = malloc(len);
    if (!desc) {
        return NULL;
    }

    char *p = desc;
    *p++ = '(';
    for (int i = 0; i < num_params; i++) {
        *p++ = 'L';
        *p++ = '$';
        *p++ = 'O';
        *p++ = ';';
    }
    *p++ = ')';
    *p++ = 'L';
    *p++ = '$';
    *p++ = 'O';
    *p++ = ';';
    *p = '\0';

    return desc;
}

/**
 * Count the number of parameters in an AST_ARGUMENTS node.
 * This counts regular parameters; *args and **kwargs are handled separately.
 */
static int count_parameters(ast_node_t *args_node)
{
    if (!args_node || args_node->type != AST_ARGUMENTS) {
        return 0;
    }

    int count = 0;

    /* Count posonlyargs */
    for (slist_t *s = args_node->data.arguments.posonlyargs; s; s = s->next) {
        count++;
    }

    /* Count args */
    for (slist_t *s = args_node->data.arguments.args; s; s = s->next) {
        count++;
    }

    /* Count *args as one parameter (it receives a tuple) */
    if (args_node->data.arguments.vararg) {
        count++;
    }

    /* Count kwonlyargs */
    for (slist_t *s = args_node->data.arguments.kwonlyargs; s; s = s->next) {
        count++;
    }

    /* Count **kwargs as one parameter (it receives a dict) */
    if (args_node->data.arguments.kwarg) {
        count++;
    }

    return count;
}

/**
 * Generate code for a function definition.
 * Creates a static method for the function body and stores a callable
 * in the function name variable.
 */
static void codegen_function_def(codegen_ctx_t *ctx, ast_node_t *node)
{
    const char *func_name = node->data.func_def.name;
    ast_node_t *args_node = node->data.func_def.args;
    slist_t *body = node->data.func_def.body;

    /* Collect free variables (closures) */
    codegen_ctx_t temp_ctx = {0};
    temp_ctx.parent_ctx = ctx;
    slist_t *free_vars = collect_free_vars(&temp_ctx, args_node, body);
    int num_free_vars = slist_length(free_vars);
    bool has_closure = (num_free_vars > 0);

    /* Count parameters */
    int num_params = count_parameters(args_node);

    /* Build method name: prefix with $py_ to avoid conflicts
     * When inside a class, include the class name for uniqueness */
    char method_name[256];
    if (ctx->current_class_name) {
        snprintf(method_name, sizeof(method_name), "$py_%s$%s",
                 ctx->current_class_name, func_name);
    } else {
        snprintf(method_name, sizeof(method_name), "$py_%s", func_name);
    }

    /* Build method descriptor (with closure array if needed) */
    char *descriptor;
    if (has_closure) {
        descriptor = build_method_descriptor_ex(num_params, true);
    } else {
        descriptor = build_method_descriptor(num_params);
    }
    if (!descriptor) {
        slist_free(free_vars);
        return;
    }

    /* Add the method to the class */
    method_info_t *method = class_writer_add_method(ctx->cw, method_name,
                                                     descriptor,
                                                     ACC_PUBLIC | ACC_STATIC);

    /* Create a new codegen context for the function body */
    codegen_ctx_t *func_ctx = codegen_ctx_new(ctx->cw, method, ctx->indy_cache,
                                               ctx->scope, ctx->source);
    if (!func_ctx) {
        free(descriptor);
        slist_free(free_vars);
        return;
    }

    /* Set parent context and captured variables for closure support */
    func_ctx->parent_ctx = ctx;
    func_ctx->captured_vars = free_vars;

    /* Set up parameters as local variables */
    int slot = 0;
    const_pool_t *cp = class_writer_get_cp(ctx->cw);

    /* If we have a closure, slot 0 is the closure array */
    if (has_closure) {
        func_ctx->closure_slot = slot;
        if (func_ctx->stackmap) {
            stackmap_set_local_object(func_ctx->stackmap, slot, cp, "[L$O;");
        }
        slot++;
    }

    /* Process posonlyargs */
    if (args_node && args_node->type == AST_ARGUMENTS) {
        for (slist_t *s = args_node->data.arguments.posonlyargs; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG) {
                const char *param_name = arg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(func_ctx->locals, param_name, var);
                }
                /* Track in stackmap */
                if (func_ctx->stackmap) {
                    stackmap_set_local_object(func_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }

        /* Process args */
        for (slist_t *s = args_node->data.arguments.args; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG) {
                const char *param_name = arg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(func_ctx->locals, param_name, var);
                }
                /* Track in stackmap */
                if (func_ctx->stackmap) {
                    stackmap_set_local_object(func_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }

        /* Process *args (vararg) */
        if (args_node->data.arguments.vararg) {
            ast_node_t *vararg = args_node->data.arguments.vararg;
            if (vararg->type == AST_ARG) {
                const char *param_name = vararg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(func_ctx->locals, param_name, var);
                }
                if (func_ctx->stackmap) {
                    stackmap_set_local_object(func_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }

        /* Process kwonlyargs */
        for (slist_t *s = args_node->data.arguments.kwonlyargs; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG) {
                const char *param_name = arg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(func_ctx->locals, param_name, var);
                }
                /* Track in stackmap */
                if (func_ctx->stackmap) {
                    stackmap_set_local_object(func_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }

        /* Process **kwargs (kwarg) */
        if (args_node->data.arguments.kwarg) {
            ast_node_t *kwarg = args_node->data.arguments.kwarg;
            if (kwarg->type == AST_ARG) {
                const char *param_name = kwarg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(func_ctx->locals, param_name, var);
                }
                if (func_ctx->stackmap) {
                    stackmap_set_local_object(func_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }
    }

    func_ctx->next_local = slot;
    func_ctx->max_locals = slot;

    /* Generate null checks and default substitutions for parameters with defaults.
     * Defaults are aligned to the END of the args list (not the beginning).
     * E.g., def f(a, b, c=1, d=2) has defaults for c and d.
     * defaults list length <= args list length.
     */
    if (args_node && args_node->type == AST_ARGUMENTS) {
        slist_t *args = args_node->data.arguments.args;
        slist_t *defaults = args_node->data.arguments.defaults;

        int num_args = slist_length(args);
        int num_defaults = slist_length(defaults);
        int first_default_idx = num_args - num_defaults;

        /* Also need to account for posonlyargs */
        int posonly_count = slist_length(args_node->data.arguments.posonlyargs);

        /* Iterate through args that have defaults */
        slist_t *default_iter = defaults;
        int arg_idx = 0;
        for (slist_t *s = args; s; s = s->next, arg_idx++) {
            if (arg_idx >= first_default_idx && default_iter) {
                ast_node_t *default_val = default_iter->data;
                ast_node_t *arg = s->data;

                if (arg && arg->type == AST_ARG && default_val) {
                    int param_slot = posonly_count + arg_idx;

                    /* Generate: if (param == null) param = default_value; */
                    label_t *skip_label = codegen_new_label(func_ctx);

                    /* Load param and check if null */
                    emit_aload(func_ctx, param_slot);
                    codegen_emit_jump(func_ctx, OP_IFNONNULL, skip_label);
                    stack_pop(func_ctx, 1);
                    if (func_ctx->stackmap) {
                        stackmap_pop(func_ctx->stackmap, 1);
                    }

                    /* Param is null, evaluate default and store */
                    codegen_expr(func_ctx, default_val);
                    emit_astore(func_ctx, param_slot);

                    /* Skip label */
                    codegen_mark_label(func_ctx, skip_label);
                }

                default_iter = default_iter->next;
            }
        }

        /* Handle kw_defaults for kwonlyargs */
        slist_t *kw_args = args_node->data.arguments.kwonlyargs;
        slist_t *kw_defaults = args_node->data.arguments.kw_defaults;
        int kw_idx = 0;
        int kw_base_slot = posonly_count + num_args;

        slist_t *kwd_iter = kw_defaults;
        for (slist_t *s = kw_args; s && kwd_iter; s = s->next, kwd_iter = kwd_iter->next, kw_idx++) {
            ast_node_t *default_val = kwd_iter->data;
            ast_node_t *arg = s->data;

            if (arg && arg->type == AST_ARG && default_val) {
                int param_slot = kw_base_slot + kw_idx;

                /* Generate: if (param == null) param = default_value; */
                label_t *skip_label = codegen_new_label(func_ctx);

                emit_aload(func_ctx, param_slot);
                codegen_emit_jump(func_ctx, OP_IFNONNULL, skip_label);
                stack_pop(func_ctx, 1);
                if (func_ctx->stackmap) {
                    stackmap_pop(func_ctx->stackmap, 1);
                }

                codegen_expr(func_ctx, default_val);
                emit_astore(func_ctx, param_slot);

                codegen_mark_label(func_ctx, skip_label);
            }
        }
    }

    /* Generate code for function body */
    codegen_stmts(func_ctx, body);

    /* If the function doesn't end with a return, add implicit return None */
    /* Check if last instruction was ARETURN */
    bool needs_return = true;
    if (func_ctx->code->len > 0) {
        uint8_t last_op = func_ctx->code->data[func_ctx->code->len - 1];
        if (last_op == OP_ARETURN || last_op == OP_RETURN) {
            needs_return = false;
        }
    }

    if (needs_return) {
        emit_py_none(func_ctx);
        emit_u8(func_ctx, OP_ARETURN);
        stack_pop(func_ctx, 1);
    }

    /* Finalize function code attribute */
    codegen_resolve_labels(func_ctx);
    func_ctx->code_attr->data.code.max_stack = func_ctx->max_stack > 0 ? func_ctx->max_stack : 1;
    func_ctx->code_attr->data.code.max_locals = func_ctx->max_locals > 0 ? func_ctx->max_locals : 1;

    /* Serialize and attach StackMapTable */
    if (func_ctx->stackmap && func_ctx->stackmap->num_entries > 0) {
        uint32_t smt_length;
        uint8_t *smt_data = stackmap_serialize(func_ctx->stackmap, cp, &smt_length);
        if (smt_data) {
            code_attr_set_stack_map_table(func_ctx->code_attr, cp, smt_data, smt_length);
        }
    }

    /* Add code attribute to method */
    method->attributes = slist_append(method->attributes, func_ctx->code_attr);

    /* Save captured vars info before freeing func_ctx */
    slist_t *captured_vars = func_ctx->captured_vars;
    func_ctx->captured_vars = NULL;  /* Transfer ownership */

    /* Clean up function context (but don't free code_attr, it's owned by method) */
    func_ctx->code_attr = NULL;
    codegen_ctx_free(func_ctx);

    /* Now in the main code, we need to create a callable and store it in the
     * function name variable. For closures, we bind captured values.
     */

    /* Get the class name for the method reference */
    const char *class_name = cp->entries[cp->entries[ctx->cw->this_class].data.class_index].data.utf8;

    /* Create a methodref for the function (must use descriptor before freeing!) */
    uint16_t method_ref = cp_add_methodref(cp, class_name, method_name, descriptor);

    /* Now we can free the descriptor */
    free(descriptor);

    /* Create method handle entry pointing to the methodref */
    uint16_t mh_index = cp_add_method_handle(cp, REF_invokeStatic, method_ref);

    /* Load the method handle */
    emit_u8(ctx, OP_LDC_W);
    emit_u16(ctx, mh_index);
    stack_push(ctx, 1);
    if (ctx->stackmap) {
        stackmap_push_object(ctx->stackmap, cp, "java/lang/invoke/MethodHandle");
    }

    /* If we have captured variables, create closure array and wrap */
    if (captured_vars) {
        int num_captured = slist_length(captured_vars);

        /* Create array for captured values */
        emit_iconst(ctx, num_captured);
        emit_anewarray(ctx, LRT_OBJECT);

        /* Fill with captured values from parent context */
        int i = 0;
        for (slist_t *s = captured_vars; s; s = s->next) {
            const char *var_name = (const char *)s->data;

            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            emit_iconst(ctx, i);

            /* Load the captured variable from current (parent) context */
            int var_slot = codegen_get_local(ctx, var_name);
            if (var_slot >= 0) {
                emit_aload(ctx, var_slot);
            } else {
                /* Should not happen if free var detection is correct */
                emit_aconst_null(ctx);
            }

            emit_u8(ctx, OP_AASTORE);
            stack_pop(ctx, 3);
            i++;
        }

        /* Call $MH.withClosure(MethodHandle, $O[]) -> $MH */
        emit_invokestatic(ctx, "$MH", "withClosure",
                          "(Ljava/lang/invoke/MethodHandle;[L$O;)L$MH;");
        stack_pop(ctx, 2);
        stack_push(ctx, 1);

        slist_free(captured_vars);
    } else {
        /* No closure - wrap in $MH directly */
        emit_invokestatic(ctx, "$MH", "of",
                          "(Ljava/lang/invoke/MethodHandle;)L$MH;");
        /* Stack: MH -> $MH (no change in count) */
    }

    /* Store in local variable with function name */
    /* At module level (not inside a class), also store to globals */
    if (ctx->is_module_level && ctx->current_class_name == NULL) {
        /* Stack: $MH */
        emit_ldc_string(ctx, func_name);  /* Stack: $MH, name */
        emit_u8(ctx, OP_SWAP);             /* Stack: name, $MH */
        emit_invokestatic(ctx, "$G", "setGlobal",
                          "(Ljava/lang/String;L$O;)V");
        stack_pop(ctx, 2);
    } else {
        int func_slot = codegen_get_local(ctx, func_name);
        if (func_slot < 0) {
            func_slot = codegen_alloc_local(ctx, func_name);
        }
        emit_astore(ctx, func_slot);
    }
}

/**
 * Generate code for a lambda expression.
 * Lambda creates an anonymous function and leaves the callable on the stack.
 */
static void codegen_lambda(codegen_ctx_t *ctx, ast_node_t *node)
{
    ast_node_t *args_node = node->data.lambda.args;
    ast_node_t *body = node->data.lambda.body;

    /* Generate unique method name */
    char method_name[64];
    snprintf(method_name, sizeof(method_name), "$lambda$%d", lambda_counter++);

    /* Collect free variables for closure support */
    codegen_ctx_t temp_ctx = {0};
    temp_ctx.parent_ctx = ctx;
    slist_t *free_vars = collect_free_vars_from_expr(&temp_ctx, args_node, body);

    int num_free_vars = slist_length(free_vars);
    bool has_closure = (num_free_vars > 0);

    /* Count parameters */
    int num_params = count_parameters(args_node);

    /* Build method descriptor */
    char *descriptor;
    if (has_closure) {
        descriptor = build_method_descriptor_ex(num_params, true);
    } else {
        descriptor = build_method_descriptor(num_params);
    }
    if (!descriptor) {
        slist_free(free_vars);
        return;
    }

    /* Add the method to the class */
    method_info_t *method = class_writer_add_method(ctx->cw, method_name,
                                                     descriptor,
                                                     ACC_PUBLIC | ACC_STATIC);

    /* Create a new codegen context for the lambda body */
    codegen_ctx_t *lambda_ctx = codegen_ctx_new(ctx->cw, method, ctx->indy_cache,
                                                 ctx->scope, ctx->source);
    if (!lambda_ctx) {
        free(descriptor);
        slist_free(free_vars);
        return;
    }

    /* Set parent context and captured variables */
    lambda_ctx->parent_ctx = ctx;
    lambda_ctx->captured_vars = free_vars;

    /* Set up parameters as local variables */
    int slot = 0;
    const_pool_t *cp = class_writer_get_cp(ctx->cw);

    /* If we have a closure, slot 0 is the closure array */
    if (has_closure) {
        lambda_ctx->closure_slot = slot;
        if (lambda_ctx->stackmap) {
            stackmap_set_local_object(lambda_ctx->stackmap, slot, cp, "[L$O;");
        }
        slot++;
    }

    /* Process lambda arguments (simpler than function - just args, no kw) */
    if (args_node && args_node->type == AST_ARGUMENTS) {
        for (slist_t *s = args_node->data.arguments.args; s; s = s->next) {
            ast_node_t *arg = s->data;
            if (arg && arg->type == AST_ARG) {
                const char *param_name = arg->data.arg.arg;
                local_var_t *var = malloc(sizeof(local_var_t));
                if (var) {
                    var->name = str_dup(param_name);
                    var->slot = slot;
                    var->start_pc = 0;
                    var->end_pc = -1;
                    hashtable_insert(lambda_ctx->locals, param_name, var);
                }
                if (lambda_ctx->stackmap) {
                    stackmap_set_local_object(lambda_ctx->stackmap, slot, cp, LRT_OBJECT);
                }
                slot++;
            }
        }

        /* Handle defaults for lambda parameters */
        slist_t *args = args_node->data.arguments.args;
        slist_t *defaults = args_node->data.arguments.defaults;

        int num_args = slist_length(args);
        int num_defaults = slist_length(defaults);
        int first_default_idx = num_args - num_defaults;

        int closure_offset = has_closure ? 1 : 0;
        slist_t *default_iter = defaults;
        int arg_idx = 0;
        for (slist_t *s = args; s; s = s->next, arg_idx++) {
            if (arg_idx >= first_default_idx && default_iter) {
                ast_node_t *default_val = default_iter->data;
                ast_node_t *arg = s->data;

                if (arg && arg->type == AST_ARG && default_val) {
                    int param_slot = closure_offset + arg_idx;

                    /* Generate: if (param == null) param = default_value; */
                    label_t *skip_label = codegen_new_label(lambda_ctx);

                    emit_aload(lambda_ctx, param_slot);
                    codegen_emit_jump(lambda_ctx, OP_IFNONNULL, skip_label);
                    stack_pop(lambda_ctx, 1);
                    if (lambda_ctx->stackmap) {
                        stackmap_pop(lambda_ctx->stackmap, 1);
                    }

                    codegen_expr(lambda_ctx, default_val);
                    emit_astore(lambda_ctx, param_slot);

                    codegen_mark_label(lambda_ctx, skip_label);
                }

                default_iter = default_iter->next;
            }
        }
    }

    lambda_ctx->next_local = slot;
    lambda_ctx->max_locals = slot;

    /* Generate code for lambda body (a single expression) and return it */
    codegen_expr(lambda_ctx, body);
    emit_u8(lambda_ctx, OP_ARETURN);
    stack_pop(lambda_ctx, 1);

    /* Finalize lambda code attribute */
    codegen_resolve_labels(lambda_ctx);
    lambda_ctx->code_attr->data.code.max_stack = lambda_ctx->max_stack > 0 ? lambda_ctx->max_stack : 1;
    lambda_ctx->code_attr->data.code.max_locals = lambda_ctx->max_locals > 0 ? lambda_ctx->max_locals : 1;

    /* Serialize and attach StackMapTable */
    if (lambda_ctx->stackmap && lambda_ctx->stackmap->num_entries > 0) {
        uint32_t smt_length;
        uint8_t *smt_data = stackmap_serialize(lambda_ctx->stackmap, cp, &smt_length);
        if (smt_data) {
            code_attr_set_stack_map_table(lambda_ctx->code_attr, cp, smt_data, smt_length);
        }
    }

    /* Add code attribute to method */
    method->attributes = slist_append(method->attributes, lambda_ctx->code_attr);

    /* Save captured vars before cleanup */
    slist_t *captured_vars = lambda_ctx->captured_vars;
    lambda_ctx->captured_vars = NULL;

    /* Clean up context (don't free code_attr, it's owned by method) */
    lambda_ctx->code_attr = NULL;
    codegen_ctx_free(lambda_ctx);

    /* Now generate code in the OUTER context to create the callable */

    /* Get the class name */
    const char *class_name = cp->entries[cp->entries[ctx->cw->this_class].data.class_index].data.utf8;

    /* Create method reference and handle */
    uint16_t method_ref = cp_add_methodref(cp, class_name, method_name, descriptor);
    free(descriptor);

    uint16_t mh_index = cp_add_method_handle(cp, REF_invokeStatic, method_ref);

    /* Load the method handle */
    emit_u8(ctx, OP_LDC_W);
    emit_u16(ctx, mh_index);
    stack_push(ctx, 1);
    if (ctx->stackmap) {
        stackmap_push_object(ctx->stackmap, cp, "java/lang/invoke/MethodHandle");
    }

    /* If closure, create array and wrap with $MH.withClosure */
    if (captured_vars) {
        int num_captured = slist_length(captured_vars);

        emit_iconst(ctx, num_captured);
        emit_anewarray(ctx, LRT_OBJECT);

        int i = 0;
        for (slist_t *s = captured_vars; s; s = s->next) {
            const char *var_name = (const char *)s->data;

            emit_u8(ctx, OP_DUP);
            stack_push(ctx, 1);
            emit_iconst(ctx, i);

            int var_slot = codegen_get_local(ctx, var_name);
            if (var_slot >= 0) {
                emit_aload(ctx, var_slot);
            } else {
                emit_aconst_null(ctx);
            }

            emit_u8(ctx, OP_AASTORE);
            stack_pop(ctx, 3);
            i++;
        }

        emit_invokestatic(ctx, "$MH", "withClosure",
                          "(Ljava/lang/invoke/MethodHandle;[L$O;)L$MH;");
        stack_pop(ctx, 2);
        stack_push(ctx, 1);

        slist_free(captured_vars);
    } else {
        /* No closure - wrap in $MH */
        emit_invokestatic(ctx, "$MH", "of",
                          "(Ljava/lang/invoke/MethodHandle;)L$MH;");
    }

    /* Lambda leaves $MH on the stack (it's an expression) */
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
    ctx->is_module_level = true;  /* Module-level code - variables are globals */

    /* Initialize stackmap for the method - slot 0 is String[] args */
    if (ctx->stackmap) {
        const_pool_t *cp = class_writer_get_cp(cw);
        stackmap_set_local_object(ctx->stackmap, 0, cp, "[Ljava/lang/String;");
    }

    /* Generate code for each statement */
    codegen_stmts(ctx, ast->data.module.body);

    /* Return from main */
    emit_u8(ctx, OP_RETURN);

    /* Finalize code attribute */
    codegen_resolve_labels(ctx);
    ctx->code_attr->data.code.max_stack = ctx->max_stack > 0 ? ctx->max_stack : 1;
    ctx->code_attr->data.code.max_locals = ctx->max_locals;

    /* Serialize and attach StackMapTable if there are any frames */
    if (ctx->stackmap && ctx->stackmap->num_entries > 0) {
        const_pool_t *cp = class_writer_get_cp(cw);
        uint32_t smt_length;
        uint8_t *smt_data = stackmap_serialize(ctx->stackmap, cp, &smt_length);
        if (smt_data) {
            code_attr_set_stack_map_table(ctx->code_attr, cp, smt_data, smt_length);
        }
    }

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

