/*
 * analyze.c
 * Semantic analysis - scope and symbol resolution
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
 * Symbol management
 * ======================================================================== */

static symbol_t *symbol_new(const char *name, symbol_kind_t kind)
{
    symbol_t *sym = calloc(1, sizeof(symbol_t));
    if (sym) {
        sym->name = str_dup(name);
        sym->kind = kind;
        sym->slot = -1;
    }
    return sym;
}

static void symbol_free(symbol_t *sym)
{
    if (sym) {
        free(sym->name);
        free(sym);
    }
}

/* ========================================================================
 * Scope management
 * ======================================================================== */

scope_t *scope_new(scope_type_t type, scope_t *parent, const char *name)
{
    scope_t *scope = calloc(1, sizeof(scope_t));
    if (!scope) {
        return NULL;
    }

    scope->type = type;
    scope->parent = parent;
    scope->symbols = hashtable_new();
    scope->children = NULL;
    scope->name = name ? str_dup(name) : NULL;
    scope->next_slot = 0;
    scope->free_vars = NULL;
    scope->cell_vars = NULL;

    if (parent) {
        parent->children = slist_append(parent->children, scope);
    }

    return scope;
}

void scope_free(scope_t *scope)
{
    if (!scope) {
        return;
    }

    hashtable_free_full(scope->symbols, (void (*)(void *))symbol_free);
    slist_free_full(scope->children, (void (*)(void *))scope_free);
    slist_free(scope->free_vars);
    slist_free(scope->cell_vars);
    free(scope->name);
    free(scope);
}

symbol_t *scope_define(scope_t *scope, const char *name, symbol_kind_t kind)
{
    /* Check for redefinition in current scope */
    if (hashtable_lookup(scope->symbols, name)) {
        return NULL;  /* Already defined */
    }

    symbol_t *sym = symbol_new(name, kind);
    if (!sym) {
        return NULL;
    }

    /* Allocate slot for local variables and parameters */
    if (kind == SYM_VARIABLE || kind == SYM_PARAMETER) {
        sym->slot = scope->next_slot++;
    }

    hashtable_insert(scope->symbols, name, sym);
    return sym;
}

symbol_t *scope_lookup(scope_t *scope, const char *name)
{
    while (scope) {
        symbol_t *sym = hashtable_lookup(scope->symbols, name);
        if (sym) {
            return sym;
        }
        scope = scope->parent;
    }
    return NULL;
}

symbol_t *scope_lookup_local(scope_t *scope, const char *name)
{
    return hashtable_lookup(scope->symbols, name);
}

/* ========================================================================
 * Analyzer management
 * ======================================================================== */

analyzer_t *analyzer_new(void)
{
    analyzer_t *analyzer = calloc(1, sizeof(analyzer_t));
    if (!analyzer) {
        return NULL;
    }

    analyzer->global_scope = scope_new(SCOPE_MODULE, NULL, NULL);
    analyzer->current_scope = analyzer->global_scope;
    analyzer->errors = NULL;
    analyzer->warnings = NULL;

    return analyzer;
}

void analyzer_free(analyzer_t *analyzer)
{
    if (!analyzer) {
        return;
    }

    scope_free(analyzer->global_scope);
    slist_free_full(analyzer->errors, free);
    slist_free_full(analyzer->warnings, free);
    free(analyzer);
}

void analyzer_error(analyzer_t *analyzer, int line, int col, const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    char *msg = malloc(strlen(buf) + 64);
    if (msg) {
        snprintf(msg, strlen(buf) + 64, "%s:%d:%d: error: %s",
                analyzer->source->filename, line, col, buf);
        analyzer->errors = slist_append(analyzer->errors, msg);
    }
}

void analyzer_warning(analyzer_t *analyzer, int line, int col, const char *fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    char *msg = malloc(strlen(buf) + 64);
    if (msg) {
        snprintf(msg, strlen(buf) + 64, "%s:%d:%d: warning: %s",
                analyzer->source->filename, line, col, buf);
        analyzer->warnings = slist_append(analyzer->warnings, msg);
    }
}

/* ========================================================================
 * AST analysis
 * ======================================================================== */

static void analyze_statement(analyzer_t *analyzer, ast_node_t *node);
static void analyze_expression(analyzer_t *analyzer, ast_node_t *node);

static void analyze_expression(analyzer_t *analyzer, ast_node_t *node)
{
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_NAME: {
            /* Look up name in scope */
            symbol_t *sym = scope_lookup(analyzer->current_scope, node->data.name.id);
            if (!sym && node->data.name.ctx == CTX_LOAD) {
                /* Undefined variable - could be a builtin */
                /* For now, just note it; full implementation needs builtin table */
            }
            if (sym) {
                sym->is_referenced = true;
            }
            break;
        }

        case AST_CALL:
            analyze_expression(analyzer, node->data.call.func);
            for (slist_t *s = node->data.call.args; s; s = s->next) {
                analyze_expression(analyzer, s->data);
            }
            break;

        case AST_ATTRIBUTE:
            analyze_expression(analyzer, node->data.attribute.value);
            break;

        case AST_SUBSCRIPT:
            analyze_expression(analyzer, node->data.subscript.value);
            analyze_expression(analyzer, node->data.subscript.slice);
            break;

        case AST_BIN_OP:
            analyze_expression(analyzer, node->data.bin_op.left);
            analyze_expression(analyzer, node->data.bin_op.right);
            break;

        case AST_UNARY_OP:
            analyze_expression(analyzer, node->data.unary_op.operand);
            break;

        case AST_LIST:
        case AST_TUPLE:
        case AST_SET:
            for (slist_t *s = node->data.collection.elts; s; s = s->next) {
                analyze_expression(analyzer, s->data);
            }
            break;

        case AST_DICT:
            for (slist_t *s = node->data.dict.keys; s; s = s->next) {
                analyze_expression(analyzer, s->data);
            }
            for (slist_t *s = node->data.dict.values; s; s = s->next) {
                analyze_expression(analyzer, s->data);
            }
            break;

        case AST_CONSTANT:
            /* No analysis needed for constants */
            break;

        default:
            break;
    }
}

static void analyze_statement(analyzer_t *analyzer, ast_node_t *node)
{
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_FUNCTION_DEF:
        case AST_ASYNC_FUNCTION_DEF: {
            /* Define function in current scope */
            symbol_t *sym = scope_define(analyzer->current_scope,
                                         node->data.func_def.name, SYM_FUNCTION);
            if (!sym) {
                analyzer_warning(analyzer, node->line, node->column,
                               "Redefinition of '%s'", node->data.func_def.name);
            }

            /* Create new scope for function body */
            scope_t *func_scope = scope_new(SCOPE_FUNCTION, analyzer->current_scope,
                                           node->data.func_def.name);
            analyzer->current_scope = func_scope;

            /* TODO: Define parameters in function scope */

            /* Analyze body */
            for (slist_t *s = node->data.func_def.body; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }

            analyzer->current_scope = func_scope->parent;
            break;
        }

        case AST_CLASS_DEF: {
            /* Define class in current scope */
            symbol_t *sym = scope_define(analyzer->current_scope,
                                         node->data.class_def.name, SYM_CLASS);
            if (!sym) {
                analyzer_warning(analyzer, node->line, node->column,
                               "Redefinition of '%s'", node->data.class_def.name);
            }

            /* Analyze base classes */
            for (slist_t *s = node->data.class_def.bases; s; s = s->next) {
                analyze_expression(analyzer, s->data);
            }

            /* Create new scope for class body */
            scope_t *class_scope = scope_new(SCOPE_CLASS, analyzer->current_scope,
                                            node->data.class_def.name);
            analyzer->current_scope = class_scope;

            /* Analyze body */
            for (slist_t *s = node->data.class_def.body; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }

            analyzer->current_scope = class_scope->parent;
            break;
        }

        case AST_ASSIGN: {
            /* Analyze RHS first */
            analyze_expression(analyzer, node->data.assign.value);

            /* Define targets in current scope if they're simple names */
            for (slist_t *s = node->data.assign.targets; s; s = s->next) {
                ast_node_t *target = s->data;
                if (target->type == AST_NAME) {
                    symbol_t *sym = scope_lookup_local(analyzer->current_scope,
                                                       target->data.name.id);
                    if (!sym) {
                        sym = scope_define(analyzer->current_scope,
                                          target->data.name.id, SYM_VARIABLE);
                    }
                    if (sym) {
                        sym->is_assigned = true;
                    }
                } else {
                    analyze_expression(analyzer, target);
                }
            }
            break;
        }

        case AST_RETURN:
            analyze_expression(analyzer, node->data.return_stmt.value);
            break;

        case AST_IF:
            analyze_expression(analyzer, node->data.if_stmt.test);
            for (slist_t *s = node->data.if_stmt.body; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            for (slist_t *s = node->data.if_stmt.orelse; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            break;

        case AST_WHILE:
            analyze_expression(analyzer, node->data.while_stmt.test);
            for (slist_t *s = node->data.while_stmt.body; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            for (slist_t *s = node->data.while_stmt.orelse; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            break;

        case AST_FOR:
            analyze_expression(analyzer, node->data.for_stmt.iter);
            /* Define loop target */
            if (node->data.for_stmt.target->type == AST_NAME) {
                symbol_t *sym = scope_lookup_local(analyzer->current_scope,
                                                   node->data.for_stmt.target->data.name.id);
                if (!sym) {
                    scope_define(analyzer->current_scope,
                                node->data.for_stmt.target->data.name.id, SYM_VARIABLE);
                }
            }
            for (slist_t *s = node->data.for_stmt.body; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            for (slist_t *s = node->data.for_stmt.orelse; s; s = s->next) {
                analyze_statement(analyzer, s->data);
            }
            break;

        case AST_EXPR_STMT:
            analyze_expression(analyzer, node->data.expr_stmt.value);
            break;

        case AST_IMPORT:
            /* Define imported names */
            for (slist_t *s = node->data.import_stmt.names; s; s = s->next) {
                ast_node_t *alias = s->data;
                const char *name = alias->data.alias.asname ?
                                  alias->data.alias.asname :
                                  alias->data.alias.name;
                /* Use first component if dotted name */
                char *dot = strchr(name, '.');
                if (dot) {
                    char *first = str_ndup(name, dot - name);
                    scope_define(analyzer->current_scope, first, SYM_IMPORT);
                    free(first);
                } else {
                    scope_define(analyzer->current_scope, name, SYM_IMPORT);
                }
            }
            break;

        case AST_GLOBAL:
            for (slist_t *s = node->data.global_stmt.names; s; s = s->next) {
                const char *name = s->data;
                symbol_t *sym = scope_lookup_local(analyzer->current_scope, name);
                if (sym) {
                    sym->kind = SYM_GLOBAL;
                } else {
                    scope_define(analyzer->current_scope, name, SYM_GLOBAL);
                }
            }
            break;

        case AST_NONLOCAL:
            for (slist_t *s = node->data.global_stmt.names; s; s = s->next) {
                const char *name = s->data;
                symbol_t *sym = scope_lookup_local(analyzer->current_scope, name);
                if (sym) {
                    sym->kind = SYM_NONLOCAL;
                } else {
                    scope_define(analyzer->current_scope, name, SYM_NONLOCAL);
                }
            }
            break;

        case AST_PASS:
        case AST_BREAK:
        case AST_CONTINUE:
            /* No analysis needed */
            break;

        default:
            break;
    }
}

bool analyzer_analyze(analyzer_t *analyzer, ast_node_t *ast, source_file_t *source)
{
    if (!analyzer || !ast || ast->type != AST_MODULE) {
        return false;
    }

    analyzer->source = source;

    /* Analyze all statements in the module */
    for (slist_t *s = ast->data.module.body; s; s = s->next) {
        analyze_statement(analyzer, s->data);
    }

    /* Print errors and warnings */
    for (slist_t *s = analyzer->errors; s; s = s->next) {
        fprintf(stderr, "%s\n", (char *)s->data);
    }
    for (slist_t *s = analyzer->warnings; s; s = s->next) {
        fprintf(stderr, "%s\n", (char *)s->data);
    }

    return analyzer->errors == NULL;
}

