/*
 * parser.c
 * Python 3 parser - recursive descent with feedforward lexer
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

#include "loretta.h"

/* ========================================================================
 * AST node constructors and utilities
 * ======================================================================== */

ast_node_t *ast_new(ast_node_type_t type, int line, int column)
{
    ast_node_t *node = calloc(1, sizeof(ast_node_t));
    if (node) {
        node->type = type;
        node->line = line;
        node->column = column;
    }
    return node;
}

void ast_free(ast_node_t *node)
{
    if (!node) {
        return;
    }

    /* Free type-specific data */
    switch (node->type) {
        case AST_MODULE:
            slist_free_full(node->data.module.body, (void (*)(void *))ast_free);
            break;

        case AST_FUNCTION_DEF:
        case AST_ASYNC_FUNCTION_DEF:
            free(node->data.func_def.name);
            ast_free(node->data.func_def.args);
            slist_free_full(node->data.func_def.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.func_def.decorator_list, (void (*)(void *))ast_free);
            ast_free(node->data.func_def.returns);
            break;

        case AST_CLASS_DEF:
            free(node->data.class_def.name);
            slist_free_full(node->data.class_def.bases, (void (*)(void *))ast_free);
            slist_free_full(node->data.class_def.keywords, (void (*)(void *))ast_free);
            slist_free_full(node->data.class_def.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.class_def.decorator_list, (void (*)(void *))ast_free);
            break;

        case AST_NAME:
            free(node->data.name.id);
            break;

        case AST_CONSTANT:
            if (node->data.constant.kind == TOK_STRING ||
                node->data.constant.kind == TOK_BYTES) {
                free(node->data.constant.value.str_val);
            }
            break;

        case AST_ATTRIBUTE:
            ast_free(node->data.attribute.value);
            free(node->data.attribute.attr);
            break;

        case AST_RETURN:
            ast_free(node->data.return_stmt.value);
            break;

        case AST_DELETE:
            slist_free_full(node->data.delete_stmt.targets, (void (*)(void *))ast_free);
            break;

        case AST_ASSIGN:
            slist_free_full(node->data.assign.targets, (void (*)(void *))ast_free);
            ast_free(node->data.assign.value);
            break;

        case AST_AUG_ASSIGN:
            ast_free(node->data.aug_assign.target);
            ast_free(node->data.aug_assign.value);
            break;

        case AST_ANN_ASSIGN:
            ast_free(node->data.ann_assign.target);
            ast_free(node->data.ann_assign.annotation);
            ast_free(node->data.ann_assign.value);
            break;

        case AST_FOR:
        case AST_ASYNC_FOR:
            ast_free(node->data.for_stmt.target);
            ast_free(node->data.for_stmt.iter);
            slist_free_full(node->data.for_stmt.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.for_stmt.orelse, (void (*)(void *))ast_free);
            break;

        case AST_WHILE:
            ast_free(node->data.while_stmt.test);
            slist_free_full(node->data.while_stmt.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.while_stmt.orelse, (void (*)(void *))ast_free);
            break;

        case AST_IF:
            ast_free(node->data.if_stmt.test);
            slist_free_full(node->data.if_stmt.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.if_stmt.orelse, (void (*)(void *))ast_free);
            break;

        case AST_WITH:
        case AST_ASYNC_WITH:
            slist_free_full(node->data.with_stmt.items, (void (*)(void *))ast_free);
            slist_free_full(node->data.with_stmt.body, (void (*)(void *))ast_free);
            break;

        case AST_RAISE:
            ast_free(node->data.raise_stmt.exc);
            ast_free(node->data.raise_stmt.cause);
            break;

        case AST_TRY:
        case AST_TRY_STAR:
            slist_free_full(node->data.try_stmt.body, (void (*)(void *))ast_free);
            slist_free_full(node->data.try_stmt.handlers, (void (*)(void *))ast_free);
            slist_free_full(node->data.try_stmt.orelse, (void (*)(void *))ast_free);
            slist_free_full(node->data.try_stmt.finalbody, (void (*)(void *))ast_free);
            break;

        case AST_ASSERT:
            ast_free(node->data.assert_stmt.test);
            ast_free(node->data.assert_stmt.msg);
            break;

        case AST_IMPORT:
            slist_free_full(node->data.import_stmt.names, (void (*)(void *))ast_free);
            break;

        case AST_IMPORT_FROM:
            free(node->data.import_from.module);
            slist_free_full(node->data.import_from.names, (void (*)(void *))ast_free);
            break;

        case AST_GLOBAL:
        case AST_NONLOCAL:
            slist_free_full(node->data.global_stmt.names, free);
            break;

        case AST_EXPR_STMT:
            ast_free(node->data.expr_stmt.value);
            break;

        case AST_BOOL_OP:
            slist_free_full(node->data.bool_op.values, (void (*)(void *))ast_free);
            break;

        case AST_NAMED_EXPR:
            ast_free(node->data.named_expr.target);
            ast_free(node->data.named_expr.value);
            break;

        case AST_BIN_OP:
            ast_free(node->data.bin_op.left);
            ast_free(node->data.bin_op.right);
            break;

        case AST_UNARY_OP:
            ast_free(node->data.unary_op.operand);
            break;

        case AST_LAMBDA:
            ast_free(node->data.lambda.args);
            ast_free(node->data.lambda.body);
            break;

        case AST_IF_EXP:
            ast_free(node->data.if_exp.test);
            ast_free(node->data.if_exp.body);
            ast_free(node->data.if_exp.orelse);
            break;

        case AST_DICT:
            slist_free_full(node->data.dict.keys, (void (*)(void *))ast_free);
            slist_free_full(node->data.dict.values, (void (*)(void *))ast_free);
            break;

        case AST_SET:
        case AST_LIST:
        case AST_TUPLE:
            slist_free_full(node->data.collection.elts, (void (*)(void *))ast_free);
            break;

        case AST_LIST_COMP:
        case AST_SET_COMP:
        case AST_GENERATOR_EXP:
            ast_free(node->data.comprehension_expr.elt);
            slist_free_full(node->data.comprehension_expr.generators, (void (*)(void *))ast_free);
            break;

        case AST_DICT_COMP:
            ast_free(node->data.dict_comp.key);
            ast_free(node->data.dict_comp.value);
            slist_free_full(node->data.dict_comp.generators, (void (*)(void *))ast_free);
            break;

        case AST_AWAIT:
        case AST_YIELD:
        case AST_YIELD_FROM:
            ast_free(node->data.await_yield.value);
            break;

        case AST_COMPARE:
            ast_free(node->data.compare.left);
            slist_free_full(node->data.compare.comparators, (void (*)(void *))ast_free);
            break;

        case AST_CALL:
            ast_free(node->data.call.func);
            slist_free_full(node->data.call.args, (void (*)(void *))ast_free);
            slist_free_full(node->data.call.keywords, (void (*)(void *))ast_free);
            break;

        case AST_SUBSCRIPT:
            ast_free(node->data.subscript.value);
            ast_free(node->data.subscript.slice);
            break;

        case AST_STARRED:
            ast_free(node->data.starred.value);
            break;

        case AST_SLICE:
            ast_free(node->data.slice.lower);
            ast_free(node->data.slice.upper);
            ast_free(node->data.slice.step);
            break;

        case AST_COMPREHENSION:
            ast_free(node->data.comprehension.target);
            ast_free(node->data.comprehension.iter);
            slist_free_full(node->data.comprehension.ifs, (void (*)(void *))ast_free);
            break;

        case AST_EXCEPT_HANDLER:
            ast_free(node->data.except_handler.type);
            free(node->data.except_handler.name);
            slist_free_full(node->data.except_handler.body, (void (*)(void *))ast_free);
            break;

        case AST_ARGUMENTS:
            slist_free_full(node->data.arguments.posonlyargs, (void (*)(void *))ast_free);
            slist_free_full(node->data.arguments.args, (void (*)(void *))ast_free);
            ast_free(node->data.arguments.vararg);
            slist_free_full(node->data.arguments.kwonlyargs, (void (*)(void *))ast_free);
            slist_free_full(node->data.arguments.kw_defaults, (void (*)(void *))ast_free);
            ast_free(node->data.arguments.kwarg);
            slist_free_full(node->data.arguments.defaults, (void (*)(void *))ast_free);
            break;

        case AST_ARG:
            free(node->data.arg.arg);
            ast_free(node->data.arg.annotation);
            break;

        case AST_KEYWORD:
            free(node->data.keyword.arg);
            ast_free(node->data.keyword.value);
            break;

        case AST_ALIAS:
            free(node->data.alias.name);
            free(node->data.alias.asname);
            break;

        case AST_WITH_ITEM:
            ast_free(node->data.with_item.context_expr);
            ast_free(node->data.with_item.optional_vars);
            break;

        case AST_MATCH:
            ast_free(node->data.match_stmt.subject);
            slist_free_full(node->data.match_stmt.cases, (void (*)(void *))ast_free);
            break;

        case AST_MATCH_CASE:
            ast_free(node->data.match_case.pattern);
            ast_free(node->data.match_case.guard);
            slist_free_full(node->data.match_case.body, (void (*)(void *))ast_free);
            break;

        case AST_MATCH_VALUE:
        case AST_MATCH_SINGLETON:
        case AST_MATCH_AS:
        case AST_MATCH_OR:
        case AST_MATCH_SEQUENCE:
        case AST_MATCH_CLASS:
        case AST_MATCH_STAR:
            ast_free(node->data.pattern.value);
            free(node->data.pattern.name);
            slist_free_full(node->data.pattern.patterns, (void (*)(void *))ast_free);
            break;

        case AST_MATCH_MAPPING:
            slist_free_full(node->data.mapping_match.keys, (void (*)(void *))ast_free);
            slist_free_full(node->data.mapping_match.patterns, (void (*)(void *))ast_free);
            break;

        default:
            break;
    }

    free(node);
}

const char *ast_type_name(ast_node_type_t type)
{
    switch (type) {
        case AST_MODULE:            return "Module";
        case AST_FUNCTION_DEF:      return "FunctionDef";
        case AST_ASYNC_FUNCTION_DEF: return "AsyncFunctionDef";
        case AST_CLASS_DEF:         return "ClassDef";
        case AST_RETURN:            return "Return";
        case AST_DELETE:            return "Delete";
        case AST_ASSIGN:            return "Assign";
        case AST_AUG_ASSIGN:        return "AugAssign";
        case AST_ANN_ASSIGN:        return "AnnAssign";
        case AST_FOR:               return "For";
        case AST_ASYNC_FOR:         return "AsyncFor";
        case AST_WHILE:             return "While";
        case AST_IF:                return "If";
        case AST_WITH:              return "With";
        case AST_ASYNC_WITH:        return "AsyncWith";
        case AST_MATCH:             return "Match";
        case AST_RAISE:             return "Raise";
        case AST_TRY:               return "Try";
        case AST_TRY_STAR:          return "TryStar";
        case AST_ASSERT:            return "Assert";
        case AST_IMPORT:            return "Import";
        case AST_IMPORT_FROM:       return "ImportFrom";
        case AST_GLOBAL:            return "Global";
        case AST_NONLOCAL:          return "Nonlocal";
        case AST_EXPR_STMT:         return "Expr";
        case AST_PASS:              return "Pass";
        case AST_BREAK:             return "Break";
        case AST_CONTINUE:          return "Continue";
        case AST_BOOL_OP:           return "BoolOp";
        case AST_NAMED_EXPR:        return "NamedExpr";
        case AST_BIN_OP:            return "BinOp";
        case AST_UNARY_OP:          return "UnaryOp";
        case AST_LAMBDA:            return "Lambda";
        case AST_IF_EXP:            return "IfExp";
        case AST_DICT:              return "Dict";
        case AST_SET:               return "Set";
        case AST_LIST_COMP:         return "ListComp";
        case AST_SET_COMP:          return "SetComp";
        case AST_DICT_COMP:         return "DictComp";
        case AST_GENERATOR_EXP:     return "GeneratorExp";
        case AST_AWAIT:             return "Await";
        case AST_YIELD:             return "Yield";
        case AST_YIELD_FROM:        return "YieldFrom";
        case AST_COMPARE:           return "Compare";
        case AST_CALL:              return "Call";
        case AST_FORMATTED_VALUE:   return "FormattedValue";
        case AST_JOINED_STR:        return "JoinedStr";
        case AST_CONSTANT:          return "Constant";
        case AST_ATTRIBUTE:         return "Attribute";
        case AST_SUBSCRIPT:         return "Subscript";
        case AST_STARRED:           return "Starred";
        case AST_NAME:              return "Name";
        case AST_LIST:              return "List";
        case AST_TUPLE:             return "Tuple";
        case AST_SLICE:             return "Slice";
        case AST_COMPREHENSION:     return "comprehension";
        case AST_EXCEPT_HANDLER:    return "ExceptHandler";
        case AST_ARGUMENTS:         return "arguments";
        case AST_ARG:               return "arg";
        case AST_KEYWORD:           return "keyword";
        case AST_ALIAS:             return "alias";
        case AST_WITH_ITEM:         return "withitem";
        case AST_MATCH_VALUE:       return "MatchValue";
        case AST_MATCH_SINGLETON:   return "MatchSingleton";
        case AST_MATCH_SEQUENCE:    return "MatchSequence";
        case AST_MATCH_MAPPING:     return "MatchMapping";
        case AST_MATCH_CLASS:       return "MatchClass";
        case AST_MATCH_STAR:        return "MatchStar";
        case AST_MATCH_AS:          return "MatchAs";
        case AST_MATCH_OR:          return "MatchOr";
        case AST_MATCH_CASE:        return "match_case";
        default:                    return "Unknown";
    }
}

static const char *binop_name(bin_op_t op)
{
    switch (op) {
        case BINOP_ADD:      return "+";
        case BINOP_SUB:      return "-";
        case BINOP_MULT:     return "*";
        case BINOP_DIV:      return "/";
        case BINOP_MOD:      return "%";
        case BINOP_POW:      return "**";
        case BINOP_LSHIFT:   return "<<";
        case BINOP_RSHIFT:   return ">>";
        case BINOP_BITOR:    return "|";
        case BINOP_BITXOR:   return "^";
        case BINOP_BITAND:   return "&";
        case BINOP_FLOORDIV: return "//";
        case BINOP_MATMULT:  return "@";
        default:             return "?";
    }
}

static const char *cmpop_name(cmp_op_t op)
{
    switch (op) {
        case CMPOP_EQ:    return "==";
        case CMPOP_NOTEQ: return "!=";
        case CMPOP_LT:    return "<";
        case CMPOP_LTE:   return "<=";
        case CMPOP_GT:    return ">";
        case CMPOP_GTE:   return ">=";
        case CMPOP_IS:    return "is";
        case CMPOP_ISNOT: return "is not";
        case CMPOP_IN:    return "in";
        case CMPOP_NOTIN: return "not in";
        default:          return "?";
    }
}

static const char *unaryop_name(unary_op_t op)
{
    switch (op) {
        case UNARYOP_INVERT: return "~";
        case UNARYOP_NOT:    return "not";
        case UNARYOP_UADD:   return "+";
        case UNARYOP_USUB:   return "-";
        default:             return "?";
    }
}

void ast_print(ast_node_t *node, int indent)
{
    if (!node) {
        return;
    }

    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
    printf("%s (line %d)\n", ast_type_name(node->type), node->line);

    /* Print children based on node type */
    switch (node->type) {
        case AST_MODULE:
            for (slist_t *s = node->data.module.body; s; s = s->next) {
                ast_print(s->data, indent + 1);
            }
            break;

        case AST_FUNCTION_DEF:
        case AST_ASYNC_FUNCTION_DEF:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("name: %s\n", node->data.func_def.name);
            for (slist_t *s = node->data.func_def.body; s; s = s->next) {
                ast_print(s->data, indent + 1);
            }
            break;

        case AST_NAME:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("id: %s\n", node->data.name.id);
            break;

        case AST_CONSTANT:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            if (node->data.constant.kind == TOK_INTEGER) {
                printf("value: %lld\n", node->data.constant.value.int_val);
            } else if (node->data.constant.kind == TOK_STRING) {
                printf("value: \"%s\"\n", node->data.constant.value.str_val);
            } else if (node->data.constant.kind == TOK_FLOAT) {
                printf("value: %g\n", node->data.constant.value.float_val);
            } else if (node->data.constant.kind == TOK_TRUE) {
                printf("value: True\n");
            } else if (node->data.constant.kind == TOK_FALSE) {
                printf("value: False\n");
            } else if (node->data.constant.kind == TOK_NONE) {
                printf("value: None\n");
            }
            break;

        case AST_BIN_OP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("op: %s\n", binop_name(node->data.bin_op.op));
            ast_print(node->data.bin_op.left, indent + 1);
            ast_print(node->data.bin_op.right, indent + 1);
            break;

        case AST_UNARY_OP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("op: %s\n", unaryop_name(node->data.unary_op.op));
            ast_print(node->data.unary_op.operand, indent + 1);
            break;

        case AST_COMPARE:
            ast_print(node->data.compare.left, indent + 1);
            for (slist_t *op = node->data.compare.ops,
                         *cmp = node->data.compare.comparators;
                 op && cmp;
                 op = op->next, cmp = cmp->next) {
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("op: %s\n", cmpop_name(*(cmp_op_t *)op->data));
                ast_print(cmp->data, indent + 1);
            }
            break;

        case AST_BOOL_OP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("op: %s\n", node->data.bool_op.op == BOOLOP_AND ? "and" : "or");
            for (slist_t *s = node->data.bool_op.values; s; s = s->next) {
                ast_print(s->data, indent + 1);
            }
            break;

        case AST_ASSIGN:
            for (slist_t *s = node->data.assign.targets; s; s = s->next) {
                ast_print(s->data, indent + 1);
            }
            ast_print(node->data.assign.value, indent + 1);
            break;

        case AST_AUG_ASSIGN:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("op: %s=\n", binop_name(node->data.aug_assign.op));
            ast_print(node->data.aug_assign.target, indent + 1);
            ast_print(node->data.aug_assign.value, indent + 1);
            break;

        case AST_EXPR_STMT:
            ast_print(node->data.expr_stmt.value, indent + 1);
            break;

        case AST_IF_EXP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("body:\n");
            ast_print(node->data.if_exp.body, indent + 2);
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("test:\n");
            ast_print(node->data.if_exp.test, indent + 2);
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("orelse:\n");
            ast_print(node->data.if_exp.orelse, indent + 2);
            break;

        case AST_LAMBDA:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("args: (");
            if (node->data.lambda.args) {
                for (slist_t *s = node->data.lambda.args->data.arguments.args; s; s = s->next) {
                    ast_node_t *arg = s->data;
                    printf("%s", arg->data.arg.arg);
                    if (s->next) printf(", ");
                }
            }
            printf(")\n");
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("body:\n");
            ast_print(node->data.lambda.body, indent + 2);
            break;

        case AST_SLICE:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("lower:\n");
            if (node->data.slice.lower) {
                ast_print(node->data.slice.lower, indent + 2);
            } else {
                for (int i = 0; i < indent + 2; i++) printf("  ");
                printf("None\n");
            }
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("upper:\n");
            if (node->data.slice.upper) {
                ast_print(node->data.slice.upper, indent + 2);
            } else {
                for (int i = 0; i < indent + 2; i++) printf("  ");
                printf("None\n");
            }
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("step:\n");
            if (node->data.slice.step) {
                ast_print(node->data.slice.step, indent + 2);
            } else {
                for (int i = 0; i < indent + 2; i++) printf("  ");
                printf("None\n");
            }
            break;

        case AST_SUBSCRIPT:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("value:\n");
            ast_print(node->data.subscript.value, indent + 2);
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("slice:\n");
            ast_print(node->data.subscript.slice, indent + 2);
            break;

        case AST_CALL:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("func:\n");
            ast_print(node->data.call.func, indent + 2);
            if (node->data.call.args) {
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("args:\n");
                for (slist_t *s = node->data.call.args; s; s = s->next) {
                    ast_print(s->data, indent + 2);
                }
            }
            break;

        case AST_LIST_COMP:
        case AST_SET_COMP:
        case AST_GENERATOR_EXP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("elt:\n");
            ast_print(node->data.comprehension_expr.elt, indent + 2);
            for (slist_t *gen = node->data.comprehension_expr.generators; gen; gen = gen->next) {
                ast_node_t *comp = gen->data;
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("for:\n");
                ast_print(comp->data.comprehension.target, indent + 2);
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("in:\n");
                ast_print(comp->data.comprehension.iter, indent + 2);
                for (slist_t *cond = comp->data.comprehension.ifs; cond; cond = cond->next) {
                    for (int i = 0; i < indent + 1; i++) {
                        printf("  ");
                    }
                    printf("if:\n");
                    ast_print(cond->data, indent + 2);
                }
            }
            break;

        case AST_DICT_COMP:
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("key:\n");
            ast_print(node->data.dict_comp.key, indent + 2);
            for (int i = 0; i < indent + 1; i++) {
                printf("  ");
            }
            printf("value:\n");
            ast_print(node->data.dict_comp.value, indent + 2);
            for (slist_t *gen = node->data.dict_comp.generators; gen; gen = gen->next) {
                ast_node_t *comp = gen->data;
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("for:\n");
                ast_print(comp->data.comprehension.target, indent + 2);
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("in:\n");
                ast_print(comp->data.comprehension.iter, indent + 2);
                for (slist_t *cond = comp->data.comprehension.ifs; cond; cond = cond->next) {
                    for (int i = 0; i < indent + 1; i++) {
                        printf("  ");
                    }
                    printf("if:\n");
                    ast_print(cond->data, indent + 2);
                }
            }
            break;

        case AST_LIST:
        case AST_SET:
        case AST_TUPLE:
            for (slist_t *s = node->data.collection.elts; s; s = s->next) {
                ast_print(s->data, indent + 1);
            }
            break;

        case AST_DICT:
            for (slist_t *k = node->data.dict.keys, *v = node->data.dict.values;
                 k && v;
                 k = k->next, v = v->next) {
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("key:\n");
                ast_print(k->data, indent + 2);
                for (int i = 0; i < indent + 1; i++) {
                    printf("  ");
                }
                printf("value:\n");
                ast_print(v->data, indent + 2);
            }
            break;

        default:
            break;
    }
}

/* ========================================================================
 * Parser implementation
 * ======================================================================== */

parser_t *parser_new(lexer_t *lexer, source_file_t *source)
{
    parser_t *parser = malloc(sizeof(parser_t));
    if (!parser) {
        return NULL;
    }

    parser->lexer = lexer;
    parser->source = source;
    parser->error_msg = NULL;
    parser->error_line = 0;
    parser->error_column = 0;

    return parser;
}

void parser_free(parser_t *parser)
{
    if (!parser) {
        return;
    }
    free(parser->error_msg);
    free(parser);
}

/* ========================================================================
 * Parser helper functions
 * ======================================================================== */

static void parser_error(parser_t *parser, const char *fmt, ...)
{
    if (parser->error_msg) {
        return;  /* Keep first error */
    }

    parser->error_line = lexer_line(parser->lexer);
    parser->error_column = lexer_column(parser->lexer);

    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    parser->error_msg = str_dup(buf);
}

static bool parser_check(parser_t *parser, token_type_t type)
{
    return lexer_type(parser->lexer) == type;
}

static bool parser_match(parser_t *parser, token_type_t type)
{
    if (parser_check(parser, type)) {
        lexer_advance(parser->lexer);
        return true;
    }
    return false;
}

static void parser_expect(parser_t *parser, token_type_t type)
{
    if (!parser_match(parser, type)) {
        parser_error(parser, "Expected '%s', got '%s'",
                    token_type_name(type),
                    token_type_name(lexer_type(parser->lexer)));
    }
}

/* Forward declarations for statement parsing */
static ast_node_t *parse_statement(parser_t *parser);
static ast_node_t *parse_simple_stmt(parser_t *parser);
static ast_node_t *parse_compound_stmt(parser_t *parser);

/* ========================================================================
 * Expression parsing - Pratt parser (top-down operator precedence)
 *
 * Binding power levels (higher = tighter binding):
 *   4  - or
 *   6  - and
 *   10 - comparisons (chained): <, <=, >, >=, ==, !=, is, in, not in
 *   12 - |
 *   14 - ^
 *   16 - &
 *   18 - <<, >>
 *   20 - +, -
 *   22 - *, /, //, %, @
 *   26 - ** (right-associative)
 * ======================================================================== */

/* Forward declarations for expression parsing */
static ast_node_t *parse_expr_bp(parser_t *parser, int min_bp);
static ast_node_t *parse_expression(parser_t *parser);
static ast_node_t *parse_atom(parser_t *parser);
static ast_node_t *parse_primary(parser_t *parser);

/*
 * Get the binary operator and binding power for a token.
 * Returns the left binding power, or 0 if not a binary operator.
 * Sets *op to the bin_op_t value for arithmetic operators.
 * Sets *right_bp to the right binding power (for associativity).
 */
static int get_binop_bp(token_type_t type, bin_op_t *op, int *right_bp)
{
    switch (type) {
        /* Power - right associative (right_bp < left_bp) */
        case TOK_DOUBLESTAR:
            *op = BINOP_POW;
            *right_bp = 25;  /* Right assoc: 26-1 */
            return 26;

        /* Multiplicative */
        case TOK_STAR:
            *op = BINOP_MULT;
            *right_bp = 23;
            return 22;
        case TOK_SLASH:
            *op = BINOP_DIV;
            *right_bp = 23;
            return 22;
        case TOK_DOUBLESLASH:
            *op = BINOP_FLOORDIV;
            *right_bp = 23;
            return 22;
        case TOK_PERCENT:
            *op = BINOP_MOD;
            *right_bp = 23;
            return 22;
        case TOK_AT:
            *op = BINOP_MATMULT;
            *right_bp = 23;
            return 22;

        /* Additive */
        case TOK_PLUS:
            *op = BINOP_ADD;
            *right_bp = 21;
            return 20;
        case TOK_MINUS:
            *op = BINOP_SUB;
            *right_bp = 21;
            return 20;

        /* Shift */
        case TOK_LSHIFT:
            *op = BINOP_LSHIFT;
            *right_bp = 19;
            return 18;
        case TOK_RSHIFT:
            *op = BINOP_RSHIFT;
            *right_bp = 19;
            return 18;

        /* Bitwise */
        case TOK_AMPERSAND:
            *op = BINOP_BITAND;
            *right_bp = 17;
            return 16;
        case TOK_CARET:
            *op = BINOP_BITXOR;
            *right_bp = 15;
            return 14;
        case TOK_PIPE:
            *op = BINOP_BITOR;
            *right_bp = 13;
            return 12;

        default:
            return 0;
    }
}

/*
 * Check if the current token starts a comparison operator (without advancing).
 */
static bool is_comparison_op(parser_t *parser)
{
    token_type_t type = lexer_type(parser->lexer);
    switch (type) {
        case TOK_LT:
        case TOK_GT:
        case TOK_LE:
        case TOK_GE:
        case TOK_EQ:
        case TOK_NE:
        case TOK_IS:
        case TOK_IN:
            return true;
        case TOK_NOT:
            /* Could be "not in" - peek ahead */
            {
                lexer_pos_t saved = lexer_save_pos(parser->lexer);
                lexer_advance(parser->lexer);
                bool is_not_in = parser_check(parser, TOK_IN);
                lexer_restore_pos(parser->lexer, saved);
                return is_not_in;
            }
        default:
            return false;
    }
}

/*
 * Parse and consume a comparison operator, returning the cmp_op_t.
 * Assumes is_comparison_op() returned true.
 */
static cmp_op_t parse_cmpop(parser_t *parser)
{
    token_type_t type = lexer_type(parser->lexer);

    switch (type) {
        case TOK_LT:
            lexer_advance(parser->lexer);
            return CMPOP_LT;
        case TOK_GT:
            lexer_advance(parser->lexer);
            return CMPOP_GT;
        case TOK_LE:
            lexer_advance(parser->lexer);
            return CMPOP_LTE;
        case TOK_GE:
            lexer_advance(parser->lexer);
            return CMPOP_GTE;
        case TOK_EQ:
            lexer_advance(parser->lexer);
            return CMPOP_EQ;
        case TOK_NE:
            lexer_advance(parser->lexer);
            return CMPOP_NOTEQ;
        case TOK_IS:
            lexer_advance(parser->lexer);
            if (parser_match(parser, TOK_NOT)) {
                return CMPOP_ISNOT;
            }
            return CMPOP_IS;
        case TOK_IN:
            lexer_advance(parser->lexer);
            return CMPOP_IN;
        case TOK_NOT:
            /* "not in" */
            lexer_advance(parser->lexer);  /* consume 'not' */
            lexer_advance(parser->lexer);  /* consume 'in' */
            return CMPOP_NOTIN;
        default:
            /* Should not reach here if is_comparison_op was checked */
            return CMPOP_EQ;
    }
}

/*
 * Parse a comparison expression with possible chaining.
 * Python allows: a < b < c which means (a < b) and (b < c)
 * This creates an AST_COMPARE node with left, ops list, and comparators list.
 * Assumes is_comparison_op() already returned true.
 */
static ast_node_t *parse_comparison(parser_t *parser, ast_node_t *left)
{
    int line = left->line;
    int column = left->column;

    ast_node_t *node = ast_new(AST_COMPARE, line, column);
    node->data.compare.left = left;
    node->data.compare.ops = NULL;
    node->data.compare.comparators = NULL;

    /* Parse first comparison operator */
    cmp_op_t first_op = parse_cmpop(parser);
    cmp_op_t *op_ptr = malloc(sizeof(cmp_op_t));
    *op_ptr = first_op;
    node->data.compare.ops = slist_append(node->data.compare.ops, op_ptr);

    /* Parse right operand (at binding power 11 to not consume more comparisons) */
    ast_node_t *right = parse_expr_bp(parser, 11);
    node->data.compare.comparators = slist_append(node->data.compare.comparators, right);

    /* Check for chained comparisons */
    while (!parser->error_msg && is_comparison_op(parser)) {
        cmp_op_t next_op = parse_cmpop(parser);
        op_ptr = malloc(sizeof(cmp_op_t));
        *op_ptr = next_op;
        node->data.compare.ops = slist_append(node->data.compare.ops, op_ptr);

        right = parse_expr_bp(parser, 11);
        node->data.compare.comparators = slist_append(node->data.compare.comparators, right);
    }

    return node;
}

/*
 * Parse boolean operators (and, or).
 * These collect multiple operands into a single AST_BOOL_OP node.
 */
static ast_node_t *parse_bool_op(parser_t *parser, ast_node_t *left, bool_op_t op, int right_bp)
{
    int line = left->line;
    int column = left->column;

    ast_node_t *node = ast_new(AST_BOOL_OP, line, column);
    node->data.bool_op.op = op;
    node->data.bool_op.values = slist_append(NULL, left);

    /* Collect all operands with the same operator */
    token_type_t op_tok = (op == BOOLOP_AND) ? TOK_AND : TOK_OR;

    do {
        ast_node_t *right = parse_expr_bp(parser, right_bp);
        node->data.bool_op.values = slist_append(node->data.bool_op.values, right);
    } while (parser_match(parser, op_tok));

    return node;
}

/* Forward declaration for lambda parsing */
static ast_node_t *parse_lambda_args(parser_t *parser);

/*
 * Core Pratt parser: parse expression with minimum binding power.
 */
static ast_node_t *parse_expr_bp(parser_t *parser, int min_bp)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    /* Parse prefix/unary operators and atoms */
    ast_node_t *left;

    if (parser_match(parser, TOK_LAMBDA)) {
        /* Lambda expression: lambda args: body */
        ast_node_t *node = ast_new(AST_LAMBDA, line, column);
        node->data.lambda.args = parse_lambda_args(parser);
        parser_expect(parser, TOK_COLON);
        /* Lambda body is a single expression (not statements) */
        node->data.lambda.body = parse_expr_bp(parser, 0);
        return node;
    } else if (parser_match(parser, TOK_YIELD)) {
        /* Yield expression: yield value or yield from expr */
        if (parser_match(parser, TOK_FROM)) {
            /* yield from expr */
            ast_node_t *node = ast_new(AST_YIELD_FROM, line, column);
            node->data.await_yield.value = parse_expr_bp(parser, 0);
            return node;
        } else {
            /* yield [value] */
            ast_node_t *node = ast_new(AST_YIELD, line, column);
            /* Check if there's a value (not at end of expression context) */
            if (!parser_check(parser, TOK_RPAREN) && !parser_check(parser, TOK_RBRACKET) &&
                !parser_check(parser, TOK_RBRACE) && !parser_check(parser, TOK_COMMA) &&
                !parser_check(parser, TOK_NEWLINE) && !parser_check(parser, TOK_EOF) &&
                !parser_check(parser, TOK_COLON) && !parser_check(parser, TOK_SEMICOLON)) {
                node->data.await_yield.value = parse_expr_bp(parser, 0);
            } else {
                node->data.await_yield.value = NULL;
            }
            return node;
        }
    } else if (parser_match(parser, TOK_AWAIT)) {
        /* Await expression: await expr */
        ast_node_t *node = ast_new(AST_AWAIT, line, column);
        node->data.await_yield.value = parse_expr_bp(parser, 24);  /* High precedence */
        return node;
    } else if (parser_match(parser, TOK_MINUS)) {
        ast_node_t *operand = parse_expr_bp(parser, 24);  /* Unary binds tighter than * */
        left = ast_new(AST_UNARY_OP, line, column);
        left->data.unary_op.op = UNARYOP_USUB;
        left->data.unary_op.operand = operand;
    } else if (parser_match(parser, TOK_PLUS)) {
        ast_node_t *operand = parse_expr_bp(parser, 24);
        left = ast_new(AST_UNARY_OP, line, column);
        left->data.unary_op.op = UNARYOP_UADD;
        left->data.unary_op.operand = operand;
    } else if (parser_match(parser, TOK_TILDE)) {
        ast_node_t *operand = parse_expr_bp(parser, 24);
        left = ast_new(AST_UNARY_OP, line, column);
        left->data.unary_op.op = UNARYOP_INVERT;
        left->data.unary_op.operand = operand;
    } else if (parser_match(parser, TOK_NOT)) {
        /* 'not' has lower precedence than comparisons */
        ast_node_t *operand = parse_expr_bp(parser, 8);
        left = ast_new(AST_UNARY_OP, line, column);
        left->data.unary_op.op = UNARYOP_NOT;
        left->data.unary_op.operand = operand;
    } else {
        left = parse_primary(parser);
    }

    if (!left || parser->error_msg) {
        return left;
    }

    /* Parse infix operators */
    while (!parser->error_msg) {
        token_type_t type = lexer_type(parser->lexer);

        /*
         * Walrus operator (named expression): name := value
         * Has very low precedence (bp 1), right-associative.
         * Only valid when left side is a simple name.
         */
        if (type == TOK_WALRUS && min_bp <= 1) {
            if (left->type != AST_NAME) {
                parser_error(parser, "Walrus operator target must be a name");
                break;
            }
            lexer_advance(parser->lexer);

            ast_node_t *named = ast_new(AST_NAMED_EXPR, left->line, left->column);
            named->data.named_expr.target = left;
            named->data.named_expr.value = parse_expr_bp(parser, 1);
            left = named;
            continue;
        }

        /*
         * Conditional expression: body if test else orelse
         * This has the lowest precedence (below 'or').
         * We only parse this at min_bp <= 2 to allow it in contexts
         * like lambda bodies and assignment values.
         */
        if (type == TOK_IF && min_bp <= 2) {
            lexer_advance(parser->lexer);
            int if_line = lexer_line(parser->lexer);
            int if_column = lexer_column(parser->lexer);

            /* Parse the condition */
            ast_node_t *test = parse_expr_bp(parser, 4);  /* Above 'or' level */
            parser_expect(parser, TOK_ELSE);
            /* Parse the else branch (right-associative: recurse at same level) */
            ast_node_t *orelse = parse_expr_bp(parser, 2);

            ast_node_t *if_exp = ast_new(AST_IF_EXP, if_line, if_column);
            if_exp->data.if_exp.body = left;
            if_exp->data.if_exp.test = test;
            if_exp->data.if_exp.orelse = orelse;
            left = if_exp;
            continue;
        }

        /* Check for 'or' (lowest precedence binary, above conditional) */
        if (type == TOK_OR && min_bp <= 4) {
            lexer_advance(parser->lexer);
            left = parse_bool_op(parser, left, BOOLOP_OR, 5);
            continue;
        }

        /* Check for 'and' */
        if (type == TOK_AND && min_bp <= 6) {
            lexer_advance(parser->lexer);
            left = parse_bool_op(parser, left, BOOLOP_AND, 7);
            continue;
        }

        /* Check for comparison operators */
        if (min_bp <= 10 && is_comparison_op(parser)) {
            left = parse_comparison(parser, left);
            continue;
        }

        /* Check for arithmetic/bitwise binary operators */
        bin_op_t binop;
        int right_bp;
        int left_bp = get_binop_bp(type, &binop, &right_bp);

        if (left_bp == 0 || left_bp < min_bp) {
            break;
        }

        lexer_advance(parser->lexer);
        int op_line = lexer_line(parser->lexer);
        int op_column = lexer_column(parser->lexer);

        ast_node_t *right = parse_expr_bp(parser, right_bp);
        if (!right) {
            break;
        }

        ast_node_t *binop_node = ast_new(AST_BIN_OP, op_line, op_column);
        binop_node->data.bin_op.left = left;
        binop_node->data.bin_op.op = binop;
        binop_node->data.bin_op.right = right;
        left = binop_node;
    }

    return left;
}

/*
 * Parse lambda arguments: simple comma-separated identifiers.
 * For a full implementation, handle defaults, *args, **kwargs, etc.
 */
static ast_node_t *parse_lambda_args(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    ast_node_t *args_node = ast_new(AST_ARGUMENTS, line, column);
    args_node->data.arguments.posonlyargs = NULL;
    args_node->data.arguments.args = NULL;
    args_node->data.arguments.vararg = NULL;
    args_node->data.arguments.kwonlyargs = NULL;
    args_node->data.arguments.kw_defaults = NULL;
    args_node->data.arguments.kwarg = NULL;
    args_node->data.arguments.defaults = NULL;

    /* Parse comma-separated parameter names until ':' */
    while (parser_check(parser, TOK_IDENTIFIER)) {
        ast_node_t *arg = ast_new(AST_ARG, lexer_line(parser->lexer),
                                   lexer_column(parser->lexer));
        arg->data.arg.arg = str_dup(lexer_text(parser->lexer));
        arg->data.arg.annotation = NULL;
        lexer_advance(parser->lexer);

        args_node->data.arguments.args = slist_append(args_node->data.arguments.args, arg);

        if (!parser_match(parser, TOK_COMMA)) {
            break;
        }
    }

    return args_node;
}

/*
 * Parse a comprehension target - simple name or tuple of names.
 * Stops before 'in' keyword.
 */
static ast_node_t *parse_comp_target(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    /* Parse primary (name, tuple, etc.) without operators */
    ast_node_t *target = parse_primary(parser);

    /* Check for tuple unpacking: for x, y in ... */
    if (parser_match(parser, TOK_COMMA)) {
        ast_node_t *tuple = ast_new(AST_TUPLE, line, column);
        tuple->data.collection.elts = slist_new(target);
        tuple->data.collection.ctx = CTX_STORE;

        do {
            ast_node_t *elt = parse_primary(parser);
            tuple->data.collection.elts = slist_append(tuple->data.collection.elts, elt);
        } while (parser_match(parser, TOK_COMMA) && !parser_check(parser, TOK_IN));

        return tuple;
    }

    return target;
}

/*
 * Parse comprehension clauses: "for target in iter [if cond]..."
 * Returns a list of AST_COMPREHENSION nodes.
 */
static slist_t *parse_comprehension_clauses(parser_t *parser)
{
    slist_t *generators = NULL;

    while (parser_match(parser, TOK_FOR)) {
        int line = lexer_line(parser->lexer);
        int column = lexer_column(parser->lexer);

        ast_node_t *comp = ast_new(AST_COMPREHENSION, line, column);

        /* Parse target (the loop variable) - stops before 'in' */
        comp->data.comprehension.target = parse_comp_target(parser);
        if (comp->data.comprehension.target->type == AST_NAME) {
            comp->data.comprehension.target->data.name.ctx = CTX_STORE;
        }

        parser_expect(parser, TOK_IN);

        /*
         * Parse iter - need to allow comparisons but stop before 'if' keyword.
         * We use binding power 3 which is above conditional expressions (2)
         * but allows everything else including comparisons.
         * The 'if' keyword for filter is handled separately below.
         */
        comp->data.comprehension.iter = parse_expr_bp(parser, 3);

        /* Parse optional 'if' filters */
        comp->data.comprehension.ifs = NULL;
        while (parser_match(parser, TOK_IF)) {
            /*
             * Parse the filter condition. Allow all expressions including
             * comparisons, but stop before another 'for' or 'if'.
             * We use binding power 3 to allow boolean ops and comparisons.
             */
            ast_node_t *cond = parse_expr_bp(parser, 3);
            comp->data.comprehension.ifs = slist_append(comp->data.comprehension.ifs, cond);
        }

        comp->data.comprehension.is_async = false;

        generators = slist_append(generators, comp);
    }

    return generators;
}

static ast_node_t *parse_atom(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    token_type_t type = lexer_type(parser->lexer);

    switch (type) {
        case TOK_IDENTIFIER: {
            ast_node_t *node = ast_new(AST_NAME, line, column);
            node->data.name.id = str_dup(lexer_text(parser->lexer));
            node->data.name.ctx = CTX_LOAD;
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_INTEGER: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = TOK_INTEGER;
            node->data.constant.value.int_val = lexer_int_value(parser->lexer);
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_FLOAT: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = TOK_FLOAT;
            node->data.constant.value.float_val = lexer_float_value(parser->lexer);
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_STRING:
        case TOK_BYTES: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = type;
            node->data.constant.value.str_val = str_dup(lexer_text(parser->lexer));
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_TRUE: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = TOK_TRUE;
            node->data.constant.value.bool_val = true;
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_FALSE: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = TOK_FALSE;
            node->data.constant.value.bool_val = false;
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_NONE: {
            ast_node_t *node = ast_new(AST_CONSTANT, line, column);
            node->data.constant.kind = TOK_NONE;
            lexer_advance(parser->lexer);
            return node;
        }

        case TOK_LPAREN: {
            lexer_advance(parser->lexer);
            if (parser_check(parser, TOK_RPAREN)) {
                /* Empty tuple */
                lexer_advance(parser->lexer);
                ast_node_t *node = ast_new(AST_TUPLE, line, column);
                node->data.collection.elts = NULL;
                node->data.collection.ctx = CTX_LOAD;
                return node;
            }

            /* Parse first expression */
            ast_node_t *first = parse_expression(parser);

            /* Check for generator expression: (expr for ...) */
            if (parser_check(parser, TOK_FOR)) {
                ast_node_t *node = ast_new(AST_GENERATOR_EXP, line, column);
                node->data.comprehension_expr.elt = first;
                node->data.comprehension_expr.generators = parse_comprehension_clauses(parser);
                parser_expect(parser, TOK_RPAREN);
                return node;
            }

            /* Check for tuple: (a, b, ...) */
            if (parser_match(parser, TOK_COMMA)) {
                ast_node_t *node = ast_new(AST_TUPLE, line, column);
                node->data.collection.elts = slist_new(first);
                node->data.collection.ctx = CTX_LOAD;

                /* Parse remaining elements */
                if (!parser_check(parser, TOK_RPAREN)) {
                    do {
                        ast_node_t *elt = parse_expression(parser);
                        node->data.collection.elts = slist_append(node->data.collection.elts, elt);
                    } while (parser_match(parser, TOK_COMMA) && !parser_check(parser, TOK_RPAREN));
                }

                parser_expect(parser, TOK_RPAREN);
                return node;
            }

            /* Just a parenthesized expression */
            parser_expect(parser, TOK_RPAREN);
            return first;
        }

        case TOK_LBRACKET: {
            lexer_advance(parser->lexer);

            if (parser_check(parser, TOK_RBRACKET)) {
                /* Empty list */
                lexer_advance(parser->lexer);
                ast_node_t *node = ast_new(AST_LIST, line, column);
                node->data.collection.elts = NULL;
                node->data.collection.ctx = CTX_LOAD;
                return node;
            }

            /*
             * Parse first expression with binding power 1 to stop before 'for'.
             * This way [x for x in items] parses 'x' then sees 'for'.
             */
            ast_node_t *first = parse_expr_bp(parser, 1);

            /* Check for list comprehension: [expr for ...] */
            if (parser_check(parser, TOK_FOR)) {
                ast_node_t *node = ast_new(AST_LIST_COMP, line, column);
                node->data.comprehension_expr.elt = first;
                node->data.comprehension_expr.generators = parse_comprehension_clauses(parser);
                parser_expect(parser, TOK_RBRACKET);
                return node;
            }

            /* Regular list literal */
            ast_node_t *node = ast_new(AST_LIST, line, column);
            node->data.collection.elts = slist_new(first);
            node->data.collection.ctx = CTX_LOAD;

            while (parser_match(parser, TOK_COMMA) && !parser_check(parser, TOK_RBRACKET)) {
                ast_node_t *elt = parse_expression(parser);
                node->data.collection.elts = slist_append(node->data.collection.elts, elt);
            }

            parser_expect(parser, TOK_RBRACKET);
            return node;
        }

        case TOK_LBRACE: {
            lexer_advance(parser->lexer);

            if (parser_check(parser, TOK_RBRACE)) {
                /* Empty dict (not empty set - use set() for that) */
                lexer_advance(parser->lexer);
                ast_node_t *node = ast_new(AST_DICT, line, column);
                node->data.dict.keys = NULL;
                node->data.dict.values = NULL;
                return node;
            }

            /* Parse first expression */
            ast_node_t *first = parse_expression(parser);

            /* Check if it's a dict or set based on ':' after first element */
            if (parser_check(parser, TOK_COLON)) {
                /* Dict: {key: value, ...} or dict comprehension */
                lexer_advance(parser->lexer);
                ast_node_t *first_value = parse_expression(parser);

                if (parser_check(parser, TOK_FOR)) {
                    /* Dict comprehension: {k: v for k, v in ...} */
                    ast_node_t *node = ast_new(AST_DICT_COMP, line, column);
                    node->data.dict_comp.key = first;
                    node->data.dict_comp.value = first_value;
                    node->data.dict_comp.generators = parse_comprehension_clauses(parser);
                    parser_expect(parser, TOK_RBRACE);
                    return node;
                }

                /* Regular dict literal */
                ast_node_t *node = ast_new(AST_DICT, line, column);
                node->data.dict.keys = slist_new(first);
                node->data.dict.values = slist_new(first_value);

                while (parser_match(parser, TOK_COMMA) && !parser_check(parser, TOK_RBRACE)) {
                    ast_node_t *key = parse_expression(parser);
                    parser_expect(parser, TOK_COLON);
                    ast_node_t *value = parse_expression(parser);
                    node->data.dict.keys = slist_append(node->data.dict.keys, key);
                    node->data.dict.values = slist_append(node->data.dict.values, value);
                }

                parser_expect(parser, TOK_RBRACE);
                return node;
            }

            /* Set: {a, b, ...} or set comprehension */
            if (parser_check(parser, TOK_FOR)) {
                /* Set comprehension: {x for x in ...} */
                ast_node_t *node = ast_new(AST_SET_COMP, line, column);
                node->data.comprehension_expr.elt = first;
                node->data.comprehension_expr.generators = parse_comprehension_clauses(parser);
                parser_expect(parser, TOK_RBRACE);
                return node;
            }

            /* Regular set literal */
            ast_node_t *node = ast_new(AST_SET, line, column);
            node->data.collection.elts = slist_new(first);
            node->data.collection.ctx = CTX_LOAD;

            while (parser_match(parser, TOK_COMMA) && !parser_check(parser, TOK_RBRACE)) {
                ast_node_t *elt = parse_expression(parser);
                node->data.collection.elts = slist_append(node->data.collection.elts, elt);
            }

            parser_expect(parser, TOK_RBRACE);
            return node;
        }

        default:
            parser_error(parser, "Unexpected token in expression: %s",
                        token_type_name(type));
            return NULL;
    }
}

static ast_node_t *parse_primary(parser_t *parser)
{
    ast_node_t *node = parse_atom(parser);

    while (node && !parser->error_msg) {
        int line = lexer_line(parser->lexer);
        int column = lexer_column(parser->lexer);

        if (parser_match(parser, TOK_DOT)) {
            /* Attribute access */
            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier after '.'");
                break;
            }
            ast_node_t *attr_node = ast_new(AST_ATTRIBUTE, line, column);
            attr_node->data.attribute.value = node;
            attr_node->data.attribute.attr = str_dup(lexer_text(parser->lexer));
            attr_node->data.attribute.ctx = CTX_LOAD;
            lexer_advance(parser->lexer);
            node = attr_node;
        } else if (parser_match(parser, TOK_LPAREN)) {
            /* Function call: func(a, b, *args, c=1, **kwargs) */
            ast_node_t *call_node = ast_new(AST_CALL, line, column);
            call_node->data.call.func = node;
            call_node->data.call.args = NULL;
            call_node->data.call.keywords = NULL;

            while (!parser_check(parser, TOK_RPAREN) && !parser->error_msg) {
                int arg_line = lexer_line(parser->lexer);
                int arg_column = lexer_column(parser->lexer);

                /* Check for **kwargs */
                if (parser_match(parser, TOK_DOUBLESTAR)) {
                    ast_node_t *kw = ast_new(AST_KEYWORD, arg_line, arg_column);
                    kw->data.keyword.arg = NULL;  /* NULL means **kwargs */
                    kw->data.keyword.value = parse_expression(parser);
                    call_node->data.call.keywords =
                        slist_append(call_node->data.call.keywords, kw);
                }
                /* Check for *args */
                else if (parser_match(parser, TOK_STAR)) {
                    ast_node_t *starred = ast_new(AST_STARRED, arg_line, arg_column);
                    starred->data.starred.value = parse_expression(parser);
                    starred->data.starred.ctx = CTX_LOAD;
                    call_node->data.call.args =
                        slist_append(call_node->data.call.args, starred);
                }
                /* Check for keyword argument: name=value */
                else if (parser_check(parser, TOK_IDENTIFIER)) {
                    /* Need to look ahead to see if it's name=value */
                    lexer_pos_t saved = lexer_save_pos(parser->lexer);
                    char *name = str_dup(lexer_text(parser->lexer));
                    lexer_advance(parser->lexer);

                    if (parser_match(parser, TOK_ASSIGN)) {
                        /* Keyword argument */
                        ast_node_t *kw = ast_new(AST_KEYWORD, arg_line, arg_column);
                        kw->data.keyword.arg = name;
                        kw->data.keyword.value = parse_expression(parser);
                        call_node->data.call.keywords =
                            slist_append(call_node->data.call.keywords, kw);
                    } else {
                        /* Regular positional argument - restore and parse */
                        free(name);
                        lexer_restore_pos(parser->lexer, saved);
                        ast_node_t *arg = parse_expression(parser);
                        call_node->data.call.args =
                            slist_append(call_node->data.call.args, arg);
                    }
                }
                /* Regular positional argument */
                else {
                    ast_node_t *arg = parse_expression(parser);
                    call_node->data.call.args =
                        slist_append(call_node->data.call.args, arg);
                }

                if (!parser_match(parser, TOK_COMMA)) {
                    break;
                }
            }

            parser_expect(parser, TOK_RPAREN);
            node = call_node;
        } else if (parser_match(parser, TOK_LBRACKET)) {
            /* Subscript - may be index or slice */
            ast_node_t *sub_node = ast_new(AST_SUBSCRIPT, line, column);
            sub_node->data.subscript.value = node;
            sub_node->data.subscript.ctx = CTX_LOAD;

            /*
             * Parse slice or simple index:
             *   a[1]       - simple index
             *   a[1:2]     - slice with lower and upper
             *   a[1:2:3]   - slice with lower, upper, step
             *   a[:]       - slice with all None
             *   a[::2]     - slice with step only
             */
            int slice_line = lexer_line(parser->lexer);
            int slice_column = lexer_column(parser->lexer);

            /* Check if this looks like a slice (starts with ':' or has ':' after expr) */
            if (parser_check(parser, TOK_COLON)) {
                /* Slice starting with : (no lower bound) */
                ast_node_t *slice = ast_new(AST_SLICE, slice_line, slice_column);
                slice->data.slice.lower = NULL;
                slice->data.slice.upper = NULL;
                slice->data.slice.step = NULL;

                lexer_advance(parser->lexer);  /* consume first ':' */

                /* Upper bound? */
                if (!parser_check(parser, TOK_COLON) && !parser_check(parser, TOK_RBRACKET)) {
                    slice->data.slice.upper = parse_expression(parser);
                }

                /* Step? */
                if (parser_match(parser, TOK_COLON)) {
                    if (!parser_check(parser, TOK_RBRACKET)) {
                        slice->data.slice.step = parse_expression(parser);
                    }
                }

                sub_node->data.subscript.slice = slice;
            } else {
                /* Parse an expression first */
                ast_node_t *first = parse_expression(parser);

                if (parser_check(parser, TOK_COLON)) {
                    /* It's a slice: first was lower bound */
                    ast_node_t *slice = ast_new(AST_SLICE, slice_line, slice_column);
                    slice->data.slice.lower = first;
                    slice->data.slice.upper = NULL;
                    slice->data.slice.step = NULL;

                    lexer_advance(parser->lexer);  /* consume ':' */

                    /* Upper bound? */
                    if (!parser_check(parser, TOK_COLON) && !parser_check(parser, TOK_RBRACKET)) {
                        slice->data.slice.upper = parse_expression(parser);
                    }

                    /* Step? */
                    if (parser_match(parser, TOK_COLON)) {
                        if (!parser_check(parser, TOK_RBRACKET)) {
                            slice->data.slice.step = parse_expression(parser);
                        }
                    }

                    sub_node->data.subscript.slice = slice;
                } else {
                    /* Simple index */
                    sub_node->data.subscript.slice = first;
                }
            }
            parser_expect(parser, TOK_RBRACKET);
            node = sub_node;
        } else {
            break;
        }
    }

    return node;
}

/*
 * Parse a full expression (entry point for expression parsing).
 */
static ast_node_t *parse_expression(parser_t *parser)
{
    return parse_expr_bp(parser, 0);
}

/* ========================================================================
 * Statement parsing
 * ======================================================================== */

static ast_node_t *parse_simple_stmt(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    if (parser_match(parser, TOK_PASS)) {
        return ast_new(AST_PASS, line, column);
    }

    if (parser_match(parser, TOK_BREAK)) {
        return ast_new(AST_BREAK, line, column);
    }

    if (parser_match(parser, TOK_CONTINUE)) {
        return ast_new(AST_CONTINUE, line, column);
    }

    if (parser_match(parser, TOK_RETURN)) {
        ast_node_t *node = ast_new(AST_RETURN, line, column);
        if (!parser_check(parser, TOK_NEWLINE) && !parser_check(parser, TOK_SEMICOLON)) {
            node->data.return_stmt.value = parse_expression(parser);
        } else {
            node->data.return_stmt.value = NULL;
        }
        return node;
    }

    if (parser_match(parser, TOK_IMPORT)) {
        ast_node_t *node = ast_new(AST_IMPORT, line, column);
        node->data.import_stmt.names = NULL;

        do {
            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected module name");
                break;
            }

            ast_node_t *alias = ast_new(AST_ALIAS, lexer_line(parser->lexer),
                                        lexer_column(parser->lexer));
            alias->data.alias.name = str_dup(lexer_text(parser->lexer));
            alias->data.alias.asname = NULL;
            lexer_advance(parser->lexer);

            /* Handle dotted names */
            while (parser_match(parser, TOK_DOT)) {
                if (!parser_check(parser, TOK_IDENTIFIER)) {
                    parser_error(parser, "Expected identifier after '.'");
                    break;
                }
                char *new_name = str_concat(alias->data.alias.name, ".",
                                            lexer_text(parser->lexer), NULL);
                free(alias->data.alias.name);
                alias->data.alias.name = new_name;
                lexer_advance(parser->lexer);
            }

            if (parser_match(parser, TOK_AS)) {
                if (!parser_check(parser, TOK_IDENTIFIER)) {
                    parser_error(parser, "Expected identifier after 'as'");
                    break;
                }
                alias->data.alias.asname = str_dup(lexer_text(parser->lexer));
                lexer_advance(parser->lexer);
            }

            node->data.import_stmt.names = slist_append(node->data.import_stmt.names, alias);
        } while (parser_match(parser, TOK_COMMA));

        return node;
    }

    if (parser_match(parser, TOK_FROM)) {
        /* from module import names */
        ast_node_t *node = ast_new(AST_IMPORT_FROM, line, column);
        node->data.import_from.module = NULL;
        node->data.import_from.names = NULL;
        node->data.import_from.level = 0;

        /* Handle relative imports (leading dots) */
        while (parser_match(parser, TOK_DOT)) {
            node->data.import_from.level++;
        }

        /* Module name (may be absent for relative imports like "from . import x") */
        if (parser_check(parser, TOK_IDENTIFIER)) {
            char *mod_name = str_dup(lexer_text(parser->lexer));
            lexer_advance(parser->lexer);

            /* Handle dotted module names */
            while (parser_match(parser, TOK_DOT)) {
                if (!parser_check(parser, TOK_IDENTIFIER)) {
                    parser_error(parser, "Expected identifier after '.'");
                    free(mod_name);
                    return node;
                }
                char *new_name = str_concat(mod_name, ".", lexer_text(parser->lexer), NULL);
                free(mod_name);
                mod_name = new_name;
                lexer_advance(parser->lexer);
            }
            node->data.import_from.module = mod_name;
        }

        /* Expect 'import' */
        if (!parser_match(parser, TOK_IMPORT)) {
            parser_error(parser, "Expected 'import' in from statement");
            return node;
        }

        /* Check for 'from x import *' */
        if (parser_match(parser, TOK_STAR)) {
            ast_node_t *alias = ast_new(AST_ALIAS, lexer_line(parser->lexer),
                                        lexer_column(parser->lexer));
            alias->data.alias.name = str_dup("*");
            alias->data.alias.asname = NULL;
            node->data.import_from.names = slist_append(node->data.import_from.names, alias);
            return node;
        }

        /* Check for parenthesized imports: from x import (a, b, c) */
        bool paren = parser_match(parser, TOK_LPAREN);

        /* Parse import names */
        do {
            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier in import");
                break;
            }

            ast_node_t *alias = ast_new(AST_ALIAS, lexer_line(parser->lexer),
                                        lexer_column(parser->lexer));
            alias->data.alias.name = str_dup(lexer_text(parser->lexer));
            alias->data.alias.asname = NULL;
            lexer_advance(parser->lexer);

            if (parser_match(parser, TOK_AS)) {
                if (!parser_check(parser, TOK_IDENTIFIER)) {
                    parser_error(parser, "Expected identifier after 'as'");
                    break;
                }
                alias->data.alias.asname = str_dup(lexer_text(parser->lexer));
                lexer_advance(parser->lexer);
            }

            node->data.import_from.names = slist_append(node->data.import_from.names, alias);
        } while (parser_match(parser, TOK_COMMA));

        if (paren) {
            if (!parser_match(parser, TOK_RPAREN)) {
                parser_error(parser, "Expected ')' after import list");
            }
        }

        return node;
    }

    if (parser_match(parser, TOK_RAISE)) {
        /* raise [exc [from cause]] */
        ast_node_t *node = ast_new(AST_RAISE, line, column);
        node->data.raise_stmt.exc = NULL;
        node->data.raise_stmt.cause = NULL;

        /* Check for exception expression */
        if (!parser_check(parser, TOK_NEWLINE) && !parser_check(parser, TOK_SEMICOLON)) {
            node->data.raise_stmt.exc = parse_expression(parser);

            /* Check for 'from cause' */
            if (parser_match(parser, TOK_FROM)) {
                node->data.raise_stmt.cause = parse_expression(parser);
            }
        }

        return node;
    }

    if (parser_match(parser, TOK_ASSERT)) {
        /* assert test [, msg] */
        ast_node_t *node = ast_new(AST_ASSERT, line, column);
        node->data.assert_stmt.test = parse_expression(parser);
        node->data.assert_stmt.msg = NULL;

        if (parser_match(parser, TOK_COMMA)) {
            node->data.assert_stmt.msg = parse_expression(parser);
        }

        return node;
    }

    if (parser_match(parser, TOK_DEL)) {
        /* del target, target, ... */
        ast_node_t *node = ast_new(AST_DELETE, line, column);
        node->data.delete_stmt.targets = NULL;

        do {
            ast_node_t *target = parse_expression(parser);
            node->data.delete_stmt.targets = slist_append(node->data.delete_stmt.targets, target);
        } while (parser_match(parser, TOK_COMMA));

        return node;
    }

    if (parser_match(parser, TOK_GLOBAL)) {
        /* global name, name, ... */
        ast_node_t *node = ast_new(AST_GLOBAL, line, column);
        node->data.global_stmt.names = NULL;

        do {
            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier");
                break;
            }
            char *name = str_dup(lexer_text(parser->lexer));
            node->data.global_stmt.names = slist_append(node->data.global_stmt.names, name);
            lexer_advance(parser->lexer);
        } while (parser_match(parser, TOK_COMMA));

        return node;
    }

    if (parser_match(parser, TOK_NONLOCAL)) {
        /* nonlocal name, name, ... */
        ast_node_t *node = ast_new(AST_NONLOCAL, line, column);
        node->data.global_stmt.names = NULL;  /* Reuses global_stmt structure */

        do {
            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected identifier");
                break;
            }
            char *name = str_dup(lexer_text(parser->lexer));
            node->data.global_stmt.names = slist_append(node->data.global_stmt.names, name);
            lexer_advance(parser->lexer);
        } while (parser_match(parser, TOK_COMMA));

        return node;
    }

    /*
     * Expression statement or assignment.
     * Handle starred expression at start: *rest, a = items
     */
    ast_node_t *expr;
    bool starts_with_star = false;

    if (parser_match(parser, TOK_STAR)) {
        /* Starred target at start: *rest, a = items */
        starts_with_star = true;
        ast_node_t *starred = ast_new(AST_STARRED, line, column);
        starred->data.starred.value = parse_expression(parser);
        starred->data.starred.ctx = CTX_STORE;
        expr = starred;
    } else {
        expr = parse_expression(parser);
        if (!expr) {
            return NULL;
        }
    }

    /*
     * Check for tuple unpacking: a, b = pair
     * If we see a comma, build a tuple target (unless it's followed by =).
     * This handles bare tuple targets like: a, b, c = values
     */
    if (parser_check(parser, TOK_COMMA) || starts_with_star) {
        /* Could be tuple unpacking or expression tuple */
        ast_node_t *tuple = ast_new(AST_TUPLE, line, column);
        tuple->data.collection.elts = slist_new(expr);
        tuple->data.collection.ctx = CTX_STORE;

        while (parser_match(parser, TOK_COMMA)) {
            /* Check for trailing comma before = */
            if (parser_check(parser, TOK_ASSIGN)) {
                break;
            }
            /* Check for starred target: a, *rest = items */
            if (parser_match(parser, TOK_STAR)) {
                ast_node_t *starred = ast_new(AST_STARRED, lexer_line(parser->lexer),
                                               lexer_column(parser->lexer));
                starred->data.starred.value = parse_expression(parser);
                starred->data.starred.ctx = CTX_STORE;
                tuple->data.collection.elts = slist_append(tuple->data.collection.elts, starred);
            } else if (!parser_check(parser, TOK_NEWLINE) && !parser_check(parser, TOK_SEMICOLON) &&
                       !parser_check(parser, TOK_ASSIGN) && !parser_check(parser, TOK_EOF)) {
                ast_node_t *elt = parse_expression(parser);
                tuple->data.collection.elts = slist_append(tuple->data.collection.elts, elt);
            }
        }

        /* Now check for assignment */
        if (parser_match(parser, TOK_ASSIGN)) {
            ast_node_t *node = ast_new(AST_ASSIGN, line, column);
            node->data.assign.targets = slist_new(tuple);
            node->data.assign.value = parse_expression(parser);
            return node;
        }

        /* Just a tuple expression statement */
        ast_node_t *node = ast_new(AST_EXPR_STMT, line, column);
        node->data.expr_stmt.value = tuple;
        return node;
    }

    /* Check for starred expression at start: *rest, a = items */
    /* (This case is handled above in the tuple branch) */

    /* Check for augmented assignment (+=, -=, etc.) */
    token_type_t tok = lexer_type(parser->lexer);
    bin_op_t aug_op;
    bool is_aug_assign = true;

    switch (tok) {
        case TOK_PLUSEQ:        aug_op = BINOP_ADD; break;
        case TOK_MINUSEQ:       aug_op = BINOP_SUB; break;
        case TOK_STAREQ:        aug_op = BINOP_MULT; break;
        case TOK_SLASHEQ:       aug_op = BINOP_DIV; break;
        case TOK_DOUBLESLASHEQ: aug_op = BINOP_FLOORDIV; break;
        case TOK_PERCENTEQ:     aug_op = BINOP_MOD; break;
        case TOK_DOUBLESTAREQ:  aug_op = BINOP_POW; break;
        case TOK_AMPERSANDEQ:   aug_op = BINOP_BITAND; break;
        case TOK_PIPEEQ:        aug_op = BINOP_BITOR; break;
        case TOK_CARETEQ:       aug_op = BINOP_BITXOR; break;
        case TOK_LSHIFTEQ:      aug_op = BINOP_LSHIFT; break;
        case TOK_RSHIFTEQ:      aug_op = BINOP_RSHIFT; break;
        case TOK_ATSTAREQ:      aug_op = BINOP_MATMULT; break;
        default:                is_aug_assign = false; break;
    }

    if (is_aug_assign) {
        lexer_advance(parser->lexer);
        ast_node_t *node = ast_new(AST_AUG_ASSIGN, line, column);
        node->data.aug_assign.target = expr;
        node->data.aug_assign.op = aug_op;
        node->data.aug_assign.value = parse_expression(parser);
        return node;
    }

    /* Check for simple assignment (may be chained: a = b = c) */
    if (parser_match(parser, TOK_ASSIGN)) {
        ast_node_t *node = ast_new(AST_ASSIGN, line, column);
        node->data.assign.targets = slist_new(expr);

        /* Handle chained assignment: a = b = c */
        ast_node_t *value = parse_expression(parser);
        while (parser_match(parser, TOK_ASSIGN)) {
            node->data.assign.targets = slist_append(node->data.assign.targets, value);
            value = parse_expression(parser);
        }
        node->data.assign.value = value;
        return node;
    }

    /* Plain expression statement */
    ast_node_t *node = ast_new(AST_EXPR_STMT, line, column);
    node->data.expr_stmt.value = expr;
    return node;
}

static slist_t *parse_block(parser_t *parser)
{
    slist_t *body = NULL;

    parser_expect(parser, TOK_COLON);
    parser_expect(parser, TOK_NEWLINE);
    parser_expect(parser, TOK_INDENT);

    while (!parser_check(parser, TOK_DEDENT) && !parser_check(parser, TOK_EOF)) {
        ast_node_t *stmt = parse_statement(parser);
        if (stmt) {
            body = slist_append(body, stmt);
        }
        if (parser->error_msg) {
            break;
        }
    }

    parser_expect(parser, TOK_DEDENT);
    return body;
}

/*
 * Parse a single function argument with optional annotation and default.
 * Returns an AST_ARG node.
 */
static ast_node_t *parse_func_arg(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    if (!parser_check(parser, TOK_IDENTIFIER)) {
        parser_error(parser, "Expected parameter name");
        return NULL;
    }

    ast_node_t *arg = ast_new(AST_ARG, line, column);
    arg->data.arg.arg = str_dup(lexer_text(parser->lexer));
    arg->data.arg.annotation = NULL;
    lexer_advance(parser->lexer);

    /* Optional type annotation: arg: type */
    if (parser_match(parser, TOK_COLON)) {
        arg->data.arg.annotation = parse_expression(parser);
    }

    return arg;
}

/*
 * Parse function arguments: (a, b=1, *args, c, d=2, **kwargs)
 *
 * Python argument order:
 *   1. Positional-only args (before /)
 *   2. Regular positional args
 *   3. *args (vararg)
 *   4. Keyword-only args (after * or *args)
 *   5. **kwargs (kwarg)
 */
static ast_node_t *parse_func_args(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    ast_node_t *args_node = ast_new(AST_ARGUMENTS, line, column);
    args_node->data.arguments.posonlyargs = NULL;
    args_node->data.arguments.args = NULL;
    args_node->data.arguments.vararg = NULL;
    args_node->data.arguments.kwonlyargs = NULL;
    args_node->data.arguments.kw_defaults = NULL;
    args_node->data.arguments.kwarg = NULL;
    args_node->data.arguments.defaults = NULL;

    bool seen_star = false;     /* After * or *args */
    bool seen_starstar = false; /* After **kwargs */
    slist_t *pending_defaults = NULL;  /* Defaults for current arg group */

    while (!parser_check(parser, TOK_RPAREN) && !parser->error_msg) {
        /* Check for **kwargs */
        if (parser_match(parser, TOK_DOUBLESTAR)) {
            if (seen_starstar) {
                parser_error(parser, "Duplicate **kwargs");
                break;
            }
            args_node->data.arguments.kwarg = parse_func_arg(parser);
            seen_starstar = true;
        }
        /* Check for *args or bare * */
        else if (parser_match(parser, TOK_STAR)) {
            if (seen_star) {
                parser_error(parser, "Duplicate *args");
                break;
            }
            /* Store accumulated defaults for positional args */
            args_node->data.arguments.defaults = pending_defaults;
            pending_defaults = NULL;

            /* Check if it's *args or bare * (for keyword-only separator) */
            if (parser_check(parser, TOK_IDENTIFIER)) {
                args_node->data.arguments.vararg = parse_func_arg(parser);
            }
            seen_star = true;
        }
        /* Check for / (positional-only separator) */
        else if (parser_match(parser, TOK_SLASH)) {
            /* Move args to posonlyargs */
            args_node->data.arguments.posonlyargs = args_node->data.arguments.args;
            args_node->data.arguments.args = NULL;
            /* Note: defaults stay with the args they belong to */
        }
        /* Regular argument */
        else if (parser_check(parser, TOK_IDENTIFIER)) {
            ast_node_t *arg = parse_func_arg(parser);

            /* Check for default value */
            ast_node_t *default_val = NULL;
            if (parser_match(parser, TOK_ASSIGN)) {
                default_val = parse_expression(parser);
            }

            if (seen_star) {
                /* Keyword-only argument */
                args_node->data.arguments.kwonlyargs =
                    slist_append(args_node->data.arguments.kwonlyargs, arg);
                /* kw_defaults must have same length as kwonlyargs, use NULL for no default */
                args_node->data.arguments.kw_defaults =
                    slist_append(args_node->data.arguments.kw_defaults, default_val);
            } else {
                /* Regular positional argument */
                args_node->data.arguments.args =
                    slist_append(args_node->data.arguments.args, arg);
                if (default_val) {
                    pending_defaults = slist_append(pending_defaults, default_val);
                }
            }
        } else {
            parser_error(parser, "Expected parameter name");
            break;
        }

        /* Comma between arguments, or end */
        if (!parser_match(parser, TOK_COMMA)) {
            break;
        }
    }

    /* Store any remaining defaults for positional args */
    if (!seen_star && pending_defaults) {
        args_node->data.arguments.defaults = pending_defaults;
    }

    return args_node;
}

/* Forward declaration for parse_decorated */
static ast_node_t *parse_decorated(parser_t *parser);

/*
 * Parse a pattern for match/case statements (Python 3.10+).
 * Patterns include literals, captures, wildcards, sequences, mappings, etc.
 */
static ast_node_t *parse_pattern(parser_t *parser);

static ast_node_t *parse_pattern_atom(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);
    token_type_t type = lexer_type(parser->lexer);

    switch (type) {
        /* Literal patterns */
        case TOK_INTEGER:
        case TOK_FLOAT:
        case TOK_STRING:
        case TOK_BYTES: {
            ast_node_t *node = ast_new(AST_MATCH_VALUE, line, column);
            node->data.pattern.value = parse_atom(parser);
            return node;
        }

        /* Singleton patterns */
        case TOK_TRUE:
        case TOK_FALSE:
        case TOK_NONE: {
            ast_node_t *node = ast_new(AST_MATCH_SINGLETON, line, column);
            node->data.pattern.value = parse_atom(parser);
            return node;
        }

        /* Negative literal: -1, -3.14 */
        case TOK_MINUS: {
            lexer_advance(parser->lexer);
            ast_node_t *node = ast_new(AST_MATCH_VALUE, line, column);
            ast_node_t *val = parse_atom(parser);
            ast_node_t *neg = ast_new(AST_UNARY_OP, line, column);
            neg->data.unary_op.op = UNARYOP_USUB;
            neg->data.unary_op.operand = val;
            node->data.pattern.value = neg;
            return node;
        }

        /* Capture pattern or class pattern: name or ClassName(...) */
        case TOK_IDENTIFIER: {
            char *name = str_dup(lexer_text(parser->lexer));
            lexer_advance(parser->lexer);

            /* Check for class pattern: ClassName(patterns...) */
            if (parser_check(parser, TOK_LPAREN)) {
                lexer_advance(parser->lexer);
                ast_node_t *node = ast_new(AST_MATCH_CLASS, line, column);
                /* Store class name as a Name node */
                ast_node_t *cls = ast_new(AST_NAME, line, column);
                cls->data.name.id = name;
                cls->data.name.ctx = CTX_LOAD;
                node->data.pattern.value = cls;
                node->data.pattern.patterns = NULL;

                /* Parse positional patterns */
                while (!parser_check(parser, TOK_RPAREN) && !parser->error_msg) {
                    ast_node_t *pat = parse_pattern(parser);
                    node->data.pattern.patterns = slist_append(node->data.pattern.patterns, pat);
                    if (!parser_match(parser, TOK_COMMA)) {
                        break;
                    }
                }
                parser_expect(parser, TOK_RPAREN);
                return node;
            }

            /* Check for wildcard: _ */
            if (strcmp(name, "_") == 0) {
                free(name);
                ast_node_t *node = ast_new(AST_MATCH_AS, line, column);
                node->data.pattern.value = NULL;  /* Wildcard */
                node->data.pattern.name = NULL;
                return node;
            }

            /* Capture pattern: name */
            ast_node_t *node = ast_new(AST_MATCH_AS, line, column);
            node->data.pattern.value = NULL;  /* Matches anything */
            node->data.pattern.name = name;
            return node;
        }

        /* Sequence pattern: [a, b, c] or (a, b, c) */
        case TOK_LBRACKET:
        case TOK_LPAREN: {
            token_type_t close = (type == TOK_LBRACKET) ? TOK_RBRACKET : TOK_RPAREN;
            lexer_advance(parser->lexer);

            ast_node_t *node = ast_new(AST_MATCH_SEQUENCE, line, column);
            node->data.pattern.patterns = NULL;

            while (!parser_check(parser, close) && !parser->error_msg) {
                /* Check for star pattern: *rest */
                if (parser_match(parser, TOK_STAR)) {
                    ast_node_t *star = ast_new(AST_MATCH_STAR, lexer_line(parser->lexer),
                                                lexer_column(parser->lexer));
                    if (parser_check(parser, TOK_IDENTIFIER)) {
                        star->data.pattern.name = str_dup(lexer_text(parser->lexer));
                        lexer_advance(parser->lexer);
                    } else {
                        star->data.pattern.name = NULL;  /* *_ */
                    }
                    node->data.pattern.patterns = slist_append(node->data.pattern.patterns, star);
                } else {
                    ast_node_t *pat = parse_pattern(parser);
                    node->data.pattern.patterns = slist_append(node->data.pattern.patterns, pat);
                }

                if (!parser_match(parser, TOK_COMMA)) {
                    break;
                }
            }

            parser_expect(parser, close);
            return node;
        }

        /* Mapping pattern: {key: pattern, ...} */
        case TOK_LBRACE: {
            lexer_advance(parser->lexer);
            ast_node_t *node = ast_new(AST_MATCH_MAPPING, line, column);
            node->data.mapping_match.keys = NULL;
            node->data.mapping_match.patterns = NULL;

            while (!parser_check(parser, TOK_RBRACE) && !parser->error_msg) {
                ast_node_t *key = parse_expression(parser);
                parser_expect(parser, TOK_COLON);
                ast_node_t *pat = parse_pattern(parser);
                node->data.mapping_match.keys = slist_append(node->data.mapping_match.keys, key);
                node->data.mapping_match.patterns = slist_append(node->data.mapping_match.patterns, pat);
                if (!parser_match(parser, TOK_COMMA)) {
                    break;
                }
            }

            parser_expect(parser, TOK_RBRACE);
            return node;
        }

        default:
            parser_error(parser, "Expected pattern");
            return NULL;
    }
}

/*
 * Parse a pattern with optional 'as name' and '|' alternatives.
 */
static ast_node_t *parse_pattern(parser_t *parser)
{
    ast_node_t *pattern = parse_pattern_atom(parser);
    if (!pattern || parser->error_msg) {
        return pattern;
    }

    /* Check for or pattern: pattern1 | pattern2 | ... */
    if (parser_check(parser, TOK_PIPE)) {
        ast_node_t *or_pat = ast_new(AST_MATCH_OR, pattern->line, pattern->column);
        or_pat->data.pattern.patterns = slist_new(pattern);

        while (parser_match(parser, TOK_PIPE)) {
            ast_node_t *alt = parse_pattern_atom(parser);
            or_pat->data.pattern.patterns = slist_append(or_pat->data.pattern.patterns, alt);
        }
        pattern = or_pat;
    }

    /* Check for as pattern: pattern as name */
    if (parser_match(parser, TOK_AS)) {
        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected identifier after 'as'");
            return pattern;
        }
        ast_node_t *as_pat = ast_new(AST_MATCH_AS, pattern->line, pattern->column);
        as_pat->data.pattern.value = pattern;
        as_pat->data.pattern.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);
        pattern = as_pat;
    }

    return pattern;
}

static ast_node_t *parse_compound_stmt(parser_t *parser)
{
    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    /* Check for decorators */
    if (parser_check(parser, TOK_AT)) {
        return parse_decorated(parser);
    }

    if (parser_match(parser, TOK_DEF)) {
        ast_node_t *node = ast_new(AST_FUNCTION_DEF, line, column);

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected function name");
            ast_free(node);
            return NULL;
        }
        node->data.func_def.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);

        parser_expect(parser, TOK_LPAREN);
        node->data.func_def.args = parse_func_args(parser);
        parser_expect(parser, TOK_RPAREN);

        /* Optional return annotation */
        if (parser_match(parser, TOK_ARROW)) {
            node->data.func_def.returns = parse_expression(parser);
        } else {
            node->data.func_def.returns = NULL;
        }

        node->data.func_def.body = parse_block(parser);
        node->data.func_def.decorator_list = NULL;

        return node;
    }

    if (parser_match(parser, TOK_CLASS)) {
        ast_node_t *node = ast_new(AST_CLASS_DEF, line, column);

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected class name");
            ast_free(node);
            return NULL;
        }
        node->data.class_def.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);

        node->data.class_def.bases = NULL;
        node->data.class_def.keywords = NULL;

        if (parser_match(parser, TOK_LPAREN)) {
            /* Parse base classes and keyword args */
            while (!parser_check(parser, TOK_RPAREN) && !parser->error_msg) {
                ast_node_t *arg = parse_expression(parser);
                node->data.class_def.bases = slist_append(node->data.class_def.bases, arg);

                if (!parser_match(parser, TOK_COMMA)) {
                    break;
                }
            }
            parser_expect(parser, TOK_RPAREN);
        }

        node->data.class_def.body = parse_block(parser);
        node->data.class_def.decorator_list = NULL;

        return node;
    }

    if (parser_match(parser, TOK_IF)) {
        ast_node_t *node = ast_new(AST_IF, line, column);
        node->data.if_stmt.test = parse_expression(parser);
        node->data.if_stmt.body = parse_block(parser);
        node->data.if_stmt.orelse = NULL;

        while (parser_match(parser, TOK_ELIF)) {
            ast_node_t *elif_node = ast_new(AST_IF, lexer_line(parser->lexer),
                                            lexer_column(parser->lexer));
            elif_node->data.if_stmt.test = parse_expression(parser);
            elif_node->data.if_stmt.body = parse_block(parser);
            elif_node->data.if_stmt.orelse = NULL;
            node->data.if_stmt.orelse = slist_new(elif_node);
            node = elif_node;
        }

        if (parser_match(parser, TOK_ELSE)) {
            node->data.if_stmt.orelse = parse_block(parser);
        }

        return node;
    }

    if (parser_match(parser, TOK_WHILE)) {
        ast_node_t *node = ast_new(AST_WHILE, line, column);
        node->data.while_stmt.test = parse_expression(parser);
        node->data.while_stmt.body = parse_block(parser);
        node->data.while_stmt.orelse = NULL;

        if (parser_match(parser, TOK_ELSE)) {
            node->data.while_stmt.orelse = parse_block(parser);
        }

        return node;
    }

    if (parser_match(parser, TOK_FOR)) {
        ast_node_t *node = ast_new(AST_FOR, line, column);
        /* Parse target - use parse_comp_target which stops before 'in' */
        node->data.for_stmt.target = parse_comp_target(parser);
        if (node->data.for_stmt.target && node->data.for_stmt.target->type == AST_NAME) {
            node->data.for_stmt.target->data.name.ctx = CTX_STORE;
        }
        parser_expect(parser, TOK_IN);
        node->data.for_stmt.iter = parse_expression(parser);
        node->data.for_stmt.body = parse_block(parser);
        node->data.for_stmt.orelse = NULL;

        if (parser_match(parser, TOK_ELSE)) {
            node->data.for_stmt.orelse = parse_block(parser);
        }

        return node;
    }

    if (parser_match(parser, TOK_TRY)) {
        /*
         * try statement:
         *   try: body except [Type [as name]]: handler ... [else: orelse] [finally: finalbody]
         *   try: body finally: finalbody
         */
        ast_node_t *node = ast_new(AST_TRY, line, column);
        node->data.try_stmt.body = parse_block(parser);
        node->data.try_stmt.handlers = NULL;
        node->data.try_stmt.orelse = NULL;
        node->data.try_stmt.finalbody = NULL;

        /* Parse except handlers */
        while (parser_match(parser, TOK_EXCEPT)) {
            int handler_line = lexer_line(parser->lexer);
            int handler_col = lexer_column(parser->lexer);

            ast_node_t *handler = ast_new(AST_EXCEPT_HANDLER, handler_line, handler_col);
            handler->data.except_handler.type = NULL;
            handler->data.except_handler.name = NULL;

            /* Check for exception type */
            if (!parser_check(parser, TOK_COLON)) {
                handler->data.except_handler.type = parse_expression(parser);

                /* Check for 'as name' */
                if (parser_match(parser, TOK_AS)) {
                    if (!parser_check(parser, TOK_IDENTIFIER)) {
                        parser_error(parser, "Expected identifier after 'as'");
                        break;
                    }
                    handler->data.except_handler.name = str_dup(lexer_text(parser->lexer));
                    lexer_advance(parser->lexer);
                }
            }

            handler->data.except_handler.body = parse_block(parser);
            node->data.try_stmt.handlers = slist_append(node->data.try_stmt.handlers, handler);
        }

        /* Optional else clause (only if there are except handlers) */
        if (node->data.try_stmt.handlers && parser_match(parser, TOK_ELSE)) {
            node->data.try_stmt.orelse = parse_block(parser);
        }

        /* Optional finally clause */
        if (parser_match(parser, TOK_FINALLY)) {
            node->data.try_stmt.finalbody = parse_block(parser);
        }

        /* Must have at least one handler or finally */
        if (!node->data.try_stmt.handlers && !node->data.try_stmt.finalbody) {
            parser_error(parser, "try statement must have except or finally clause");
        }

        return node;
    }

    if (parser_match(parser, TOK_WITH)) {
        /*
         * with statement:
         *   with expr [as name], expr [as name], ...: body
         */
        ast_node_t *node = ast_new(AST_WITH, line, column);
        node->data.with_stmt.items = NULL;

        do {
            int item_line = lexer_line(parser->lexer);
            int item_col = lexer_column(parser->lexer);

            ast_node_t *item = ast_new(AST_WITH_ITEM, item_line, item_col);
            item->data.with_item.context_expr = parse_expression(parser);
            item->data.with_item.optional_vars = NULL;

            if (parser_match(parser, TOK_AS)) {
                item->data.with_item.optional_vars = parse_expression(parser);
            }

            node->data.with_stmt.items = slist_append(node->data.with_stmt.items, item);
        } while (parser_match(parser, TOK_COMMA));

        node->data.with_stmt.body = parse_block(parser);

        return node;
    }

    if (parser_match(parser, TOK_MATCH)) {
        /*
         * match statement (Python 3.10+):
         *   match subject:
         *       case pattern [if guard]:
         *           body
         *       ...
         */
        ast_node_t *node = ast_new(AST_MATCH, line, column);
        node->data.match_stmt.subject = parse_expression(parser);
        node->data.match_stmt.cases = NULL;

        parser_expect(parser, TOK_COLON);
        parser_expect(parser, TOK_NEWLINE);
        parser_expect(parser, TOK_INDENT);

        /* Parse case clauses */
        while (parser_match(parser, TOK_CASE)) {
            int case_line = lexer_line(parser->lexer);
            int case_col = lexer_column(parser->lexer);

            ast_node_t *match_case = ast_new(AST_MATCH_CASE, case_line, case_col);
            match_case->data.match_case.pattern = parse_pattern(parser);
            match_case->data.match_case.guard = NULL;

            /* Optional guard: case pattern if condition: */
            if (parser_match(parser, TOK_IF)) {
                match_case->data.match_case.guard = parse_expression(parser);
            }

            match_case->data.match_case.body = parse_block(parser);
            node->data.match_stmt.cases = slist_append(node->data.match_stmt.cases, match_case);
        }

        parser_expect(parser, TOK_DEDENT);

        return node;
    }

    if (parser_match(parser, TOK_ASYNC)) {
        /* async def or async for or async with */
        if (parser_match(parser, TOK_DEF)) {
            ast_node_t *node = ast_new(AST_ASYNC_FUNCTION_DEF, line, column);

            if (!parser_check(parser, TOK_IDENTIFIER)) {
                parser_error(parser, "Expected function name");
                ast_free(node);
                return NULL;
            }
            node->data.func_def.name = str_dup(lexer_text(parser->lexer));
            lexer_advance(parser->lexer);

            parser_expect(parser, TOK_LPAREN);
            node->data.func_def.args = parse_func_args(parser);
            parser_expect(parser, TOK_RPAREN);

            if (parser_match(parser, TOK_ARROW)) {
                node->data.func_def.returns = parse_expression(parser);
            } else {
                node->data.func_def.returns = NULL;
            }

            node->data.func_def.body = parse_block(parser);
            node->data.func_def.decorator_list = NULL;

            return node;
        }

        parser_error(parser, "Expected 'def', 'for', or 'with' after 'async'");
        return NULL;
    }

    return NULL;
}

/*
 * Parse decorators and apply them to a function or class definition.
 * @decorator1
 * @decorator2(arg)
 * def func(): ...
 */
static ast_node_t *parse_decorated(parser_t *parser)
{
    slist_t *decorators = NULL;

    /* Parse all decorators */
    while (parser_match(parser, TOK_AT)) {
        /* Parse the decorator expression (could be name, call, or attribute) */
        ast_node_t *decorator = parse_expression(parser);
        decorators = slist_append(decorators, decorator);

        /* Expect newline after decorator */
        parser_expect(parser, TOK_NEWLINE);

        /* Skip any extra newlines */
        while (parser_match(parser, TOK_NEWLINE)) {
            /* Skip */
        }
    }

    int line = lexer_line(parser->lexer);
    int column = lexer_column(parser->lexer);

    /* Now parse the decorated definition */
    if (parser_match(parser, TOK_DEF)) {
        ast_node_t *node = ast_new(AST_FUNCTION_DEF, line, column);

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected function name");
            ast_free(node);
            return NULL;
        }
        node->data.func_def.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);

        parser_expect(parser, TOK_LPAREN);
        node->data.func_def.args = parse_func_args(parser);
        parser_expect(parser, TOK_RPAREN);

        if (parser_match(parser, TOK_ARROW)) {
            node->data.func_def.returns = parse_expression(parser);
        } else {
            node->data.func_def.returns = NULL;
        }

        node->data.func_def.body = parse_block(parser);
        node->data.func_def.decorator_list = decorators;

        return node;
    }

    if (parser_match(parser, TOK_CLASS)) {
        ast_node_t *node = ast_new(AST_CLASS_DEF, line, column);

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected class name");
            ast_free(node);
            return NULL;
        }
        node->data.class_def.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);

        node->data.class_def.bases = NULL;
        node->data.class_def.keywords = NULL;

        if (parser_match(parser, TOK_LPAREN)) {
            /* Parse base classes and keyword args */
            while (!parser_check(parser, TOK_RPAREN) && !parser->error_msg) {
                ast_node_t *arg = parse_expression(parser);
                node->data.class_def.bases = slist_append(node->data.class_def.bases, arg);

                if (!parser_match(parser, TOK_COMMA)) {
                    break;
                }
            }
            parser_expect(parser, TOK_RPAREN);
        }

        node->data.class_def.body = parse_block(parser);
        node->data.class_def.decorator_list = decorators;

        return node;
    }

    if (parser_match(parser, TOK_ASYNC)) {
        if (!parser_match(parser, TOK_DEF)) {
            parser_error(parser, "Expected 'def' after 'async'");
            return NULL;
        }

        ast_node_t *node = ast_new(AST_ASYNC_FUNCTION_DEF, line, column);

        if (!parser_check(parser, TOK_IDENTIFIER)) {
            parser_error(parser, "Expected function name");
            ast_free(node);
            return NULL;
        }
        node->data.func_def.name = str_dup(lexer_text(parser->lexer));
        lexer_advance(parser->lexer);

        parser_expect(parser, TOK_LPAREN);
        node->data.func_def.args = parse_func_args(parser);
        parser_expect(parser, TOK_RPAREN);

        if (parser_match(parser, TOK_ARROW)) {
            node->data.func_def.returns = parse_expression(parser);
        } else {
            node->data.func_def.returns = NULL;
        }

        node->data.func_def.body = parse_block(parser);
        node->data.func_def.decorator_list = decorators;

        return node;
    }

    parser_error(parser, "Expected 'def', 'class', or 'async def' after decorator");
    return NULL;
}

static ast_node_t *parse_statement(parser_t *parser)
{
    /* Skip empty lines */
    while (parser_match(parser, TOK_NEWLINE)) {
        /* Skip */
    }

    if (parser_check(parser, TOK_EOF) || parser_check(parser, TOK_DEDENT)) {
        return NULL;
    }

    /* Try compound statement first */
    token_type_t type = lexer_type(parser->lexer);
    if (type == TOK_DEF || type == TOK_CLASS || type == TOK_IF ||
        type == TOK_WHILE || type == TOK_FOR || type == TOK_TRY ||
        type == TOK_WITH || type == TOK_ASYNC || type == TOK_MATCH ||
        type == TOK_AT) {
        return parse_compound_stmt(parser);
    }

    /* Simple statement */
    ast_node_t *stmt = parse_simple_stmt(parser);

    /* Expect newline or semicolon after simple statement */
    if (!parser_check(parser, TOK_NEWLINE) && !parser_check(parser, TOK_EOF) &&
        !parser_check(parser, TOK_DEDENT) && !parser_check(parser, TOK_SEMICOLON)) {
        parser_error(parser, "Expected newline after statement");
    }

    parser_match(parser, TOK_NEWLINE);
    parser_match(parser, TOK_SEMICOLON);

    return stmt;
}

/* ========================================================================
 * Main parse function
 * ======================================================================== */

ast_node_t *parser_parse(parser_t *parser)
{
    ast_node_t *module = ast_new(AST_MODULE, 1, 1);
    module->data.module.body = NULL;

    while (!parser_check(parser, TOK_EOF) && !parser->error_msg) {
        ast_node_t *stmt = parse_statement(parser);
        if (stmt) {
            module->data.module.body = slist_append(module->data.module.body, stmt);
        }
    }

    if (parser->error_msg) {
        fprintf(stderr, "%s:%d:%d: error: %s\n",
                parser->source->filename,
                parser->error_line,
                parser->error_column,
                parser->error_msg);
        ast_free(module);
        return NULL;
    }

    return module;
}

