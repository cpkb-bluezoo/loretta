/*
 * loretta.h
 * Main header for the loretta Python 3 to JVM bytecode compiler
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

#ifndef LORETTA_H
#define LORETTA_H

#include "util.h"

#define LORETTA_VERSION "0.1.0"

/* ========================================================================
 * Compiler options
 * ======================================================================== */

typedef struct compiler_options
{
    char *output_dir;               /* Output directory for class files */
    slist_t *source_files;          /* List of source files to compile */
    bool verbose;                   /* Verbose output */
    bool debug_info;                /* Generate debug information */
} compiler_options_t;

compiler_options_t *compiler_options_new(void);
void compiler_options_free(compiler_options_t *opts);

/* ========================================================================
 * Source file representation
 * ======================================================================== */

typedef struct source_file
{
    char *filename;                 /* Path to source file */
    char *contents;                 /* File contents */
    size_t length;                  /* Length of contents */
} source_file_t;

source_file_t *source_file_new(const char *filename);
void source_file_free(source_file_t *src);
bool source_file_load(source_file_t *src);

/* ========================================================================
 * Token types (Python 3)
 *
 * Reference: https://docs.python.org/3/reference/lexical_analysis.html
 * ======================================================================== */

typedef enum token_type
{
    /* End of file / input */
    TOK_EOF = 0,

    /* Indentation tokens (Python-specific) */
    TOK_NEWLINE,                    /* Logical line terminator */
    TOK_INDENT,                     /* Indentation increase */
    TOK_DEDENT,                     /* Indentation decrease */

    /* Literals */
    TOK_IDENTIFIER,
    TOK_INTEGER,                    /* 42, 0x2a, 0o52, 0b101010 */
    TOK_FLOAT,                      /* 3.14, 1e-10 */
    TOK_IMAGINARY,                  /* 3j, 1.5j */
    TOK_STRING,                     /* 'str', "str", '''str''', """str""" */
    TOK_BYTES,                      /* b'bytes', b"bytes" */
    TOK_FSTRING_START,              /* f" or f' - start of f-string */
    TOK_FSTRING_MIDDLE,             /* Literal part of f-string */
    TOK_FSTRING_END,                /* End of f-string */

    /* Keywords */
    TOK_FALSE,
    TOK_NONE,
    TOK_TRUE,
    TOK_AND,
    TOK_AS,
    TOK_ASSERT,
    TOK_ASYNC,
    TOK_AWAIT,
    TOK_BREAK,
    TOK_CLASS,
    TOK_CONTINUE,
    TOK_DEF,
    TOK_DEL,
    TOK_ELIF,
    TOK_ELSE,
    TOK_EXCEPT,
    TOK_FINALLY,
    TOK_FOR,
    TOK_FROM,
    TOK_GLOBAL,
    TOK_IF,
    TOK_IMPORT,
    TOK_IN,
    TOK_IS,
    TOK_LAMBDA,
    TOK_NONLOCAL,
    TOK_NOT,
    TOK_OR,
    TOK_PASS,
    TOK_RAISE,
    TOK_RETURN,
    TOK_TRY,
    TOK_WHILE,
    TOK_WITH,
    TOK_YIELD,

    /* Soft keywords (Python 3.10+) */
    TOK_MATCH,
    TOK_CASE,
    TOK_TYPE,                       /* Python 3.12+ type statement */

    /* Operators */
    TOK_PLUS,                       /* + */
    TOK_MINUS,                      /* - */
    TOK_STAR,                       /* * */
    TOK_DOUBLESTAR,                 /* ** */
    TOK_SLASH,                      /* / */
    TOK_DOUBLESLASH,                /* // */
    TOK_PERCENT,                    /* % */
    TOK_AT,                         /* @ */
    TOK_LSHIFT,                     /* << */
    TOK_RSHIFT,                     /* >> */
    TOK_AMPERSAND,                  /* & */
    TOK_PIPE,                       /* | */
    TOK_CARET,                      /* ^ */
    TOK_TILDE,                      /* ~ */
    TOK_WALRUS,                     /* := */
    TOK_LT,                         /* < */
    TOK_GT,                         /* > */
    TOK_LE,                         /* <= */
    TOK_GE,                         /* >= */
    TOK_EQ,                         /* == */
    TOK_NE,                         /* != */

    /* Delimiters */
    TOK_LPAREN,                     /* ( */
    TOK_RPAREN,                     /* ) */
    TOK_LBRACKET,                   /* [ */
    TOK_RBRACKET,                   /* ] */
    TOK_LBRACE,                     /* { */
    TOK_RBRACE,                     /* } */
    TOK_COMMA,                      /* , */
    TOK_COLON,                      /* : */
    TOK_DOT,                        /* . */
    TOK_SEMICOLON,                  /* ; */
    TOK_ASSIGN,                     /* = */
    TOK_ARROW,                      /* -> */
    TOK_ELLIPSIS,                   /* ... */

    /* Augmented assignment operators */
    TOK_PLUSEQ,                     /* += */
    TOK_MINUSEQ,                    /* -= */
    TOK_STAREQ,                     /* *= */
    TOK_SLASHEQ,                    /* /= */
    TOK_DOUBLESLASHEQ,              /* //= */
    TOK_PERCENTEQ,                  /* %= */
    TOK_ATSTAREQ,                   /* @= */
    TOK_AMPERSANDEQ,                /* &= */
    TOK_PIPEEQ,                     /* |= */
    TOK_CARETEQ,                    /* ^= */
    TOK_RSHIFTEQ,                   /* >>= */
    TOK_LSHIFTEQ,                   /* <<= */
    TOK_DOUBLESTAREQ,               /* **= */

    /* Special */
    TOK_ERROR                       /* Lexer error */
} token_type_t;

/* Token type name helper */
const char *token_type_name(token_type_t type);

/* ========================================================================
 * Lexer (feedforward style)
 *
 * The lexer maintains the current token state. The parser pulls tokens
 * by calling lexer_advance(). Token data is accessed through accessor
 * functions rather than by creating token objects.
 *
 * Python-specific: The lexer tracks indentation and emits INDENT/DEDENT
 * tokens as needed. It also handles explicit and implicit line joining.
 * ======================================================================== */

/* Maximum indentation stack depth */
#define MAX_INDENT_STACK 128

typedef struct lexer
{
    source_file_t *source;          /* Source file being lexed */
    const char *pos;                /* Current position in source */
    const char *end;                /* End of source */
    int line;                       /* Current line number */
    int column;                     /* Current column number */
    char *error_msg;                /* Error message if any */

    /* Indentation tracking */
    int indent_stack[MAX_INDENT_STACK];
    int indent_depth;               /* Current depth in indent_stack */
    int pending_dedents;            /* Number of DEDENT tokens to emit */
    bool at_line_start;             /* At beginning of logical line */
    int paren_depth;                /* Nesting depth of (), [], {} */

    /* Current token state */
    struct {
        token_type_t type;          /* Token type */
        int line;                   /* Token start line */
        int column;                 /* Token start column */
        const char *text_start;     /* Points into source or text_buf */
        size_t text_len;            /* Length of text */
        union {
            long long int_value;    /* Integer value */
            double float_value;     /* Float value */
        } value;
    } token;

    /* Buffer for tokens that need processing (string escapes, etc.) */
    char text_buf[8192];
} lexer_t;

/* Lexer API - feedforward style */
lexer_t *lexer_new(source_file_t *source);
void lexer_free(lexer_t *lexer);

void lexer_advance(lexer_t *lexer);             /* Move to next token */
token_type_t lexer_type(lexer_t *lexer);        /* Current token type */
const char *lexer_text(lexer_t *lexer);         /* Current token text (null-terminated) */
size_t lexer_text_len(lexer_t *lexer);          /* Current token text length */
int lexer_line(lexer_t *lexer);                 /* Current token line */
int lexer_column(lexer_t *lexer);               /* Current token column */
long long lexer_int_value(lexer_t *lexer);      /* Current token integer value */
double lexer_float_value(lexer_t *lexer);       /* Current token float value */

/* Lexer position for backtracking (rarely needed in Python) */
typedef struct lexer_pos
{
    const char *pos;
    int line, column;
    token_type_t token_type;
    const char *token_text_start;
    size_t token_text_len;
    int token_line, token_column;
    int indent_depth;
    int pending_dedents;
    bool at_line_start;
    int paren_depth;
} lexer_pos_t;

lexer_pos_t lexer_save_pos(lexer_t *lexer);
void lexer_restore_pos(lexer_t *lexer, lexer_pos_t pos);

/* ========================================================================
 * AST Node Types
 *
 * Python AST is based on the official Python AST specification.
 * Reference: https://docs.python.org/3/library/ast.html
 * ======================================================================== */

typedef enum ast_node_type
{
    /* Module (root) */
    AST_MODULE,

    /* Statements */
    AST_FUNCTION_DEF,               /* def name(args): body */
    AST_ASYNC_FUNCTION_DEF,         /* async def name(args): body */
    AST_CLASS_DEF,                  /* class name(bases): body */
    AST_RETURN,                     /* return value */
    AST_DELETE,                     /* del targets */
    AST_ASSIGN,                     /* targets = value */
    AST_AUG_ASSIGN,                 /* target op= value */
    AST_ANN_ASSIGN,                 /* target: annotation = value */
    AST_FOR,                        /* for target in iter: body else: orelse */
    AST_ASYNC_FOR,                  /* async for ... */
    AST_WHILE,                      /* while test: body else: orelse */
    AST_IF,                         /* if test: body elif: ... else: orelse */
    AST_WITH,                       /* with items: body */
    AST_ASYNC_WITH,                 /* async with ... */
    AST_MATCH,                      /* match subject: cases */
    AST_RAISE,                      /* raise exc from cause */
    AST_TRY,                        /* try: body except: ... else: ... finally: ... */
    AST_TRY_STAR,                   /* try: body except*: ... (Python 3.11+) */
    AST_ASSERT,                     /* assert test, msg */
    AST_IMPORT,                     /* import names */
    AST_IMPORT_FROM,                /* from module import names */
    AST_GLOBAL,                     /* global names */
    AST_NONLOCAL,                   /* nonlocal names */
    AST_EXPR_STMT,                  /* Expression as statement */
    AST_PASS,                       /* pass */
    AST_BREAK,                      /* break */
    AST_CONTINUE,                   /* continue */

    /* Expressions */
    AST_BOOL_OP,                    /* a and b, a or b */
    AST_NAMED_EXPR,                 /* x := value (walrus operator) */
    AST_BIN_OP,                     /* a + b, a * b, etc. */
    AST_UNARY_OP,                   /* -a, not a, ~a */
    AST_LAMBDA,                     /* lambda args: body */
    AST_IF_EXP,                     /* a if test else b */
    AST_DICT,                       /* {k: v, ...} */
    AST_SET,                        /* {a, b, ...} */
    AST_LIST_COMP,                  /* [x for x in iter] */
    AST_SET_COMP,                   /* {x for x in iter} */
    AST_DICT_COMP,                  /* {k: v for k, v in iter} */
    AST_GENERATOR_EXP,              /* (x for x in iter) */
    AST_AWAIT,                      /* await expr */
    AST_YIELD,                      /* yield value */
    AST_YIELD_FROM,                 /* yield from iter */
    AST_COMPARE,                    /* a < b < c */
    AST_CALL,                       /* func(args) */
    AST_FORMATTED_VALUE,            /* f-string {value} */
    AST_JOINED_STR,                 /* f"..." (f-string) */
    AST_CONSTANT,                   /* Literal: 42, "str", None, True, etc. */
    AST_ATTRIBUTE,                  /* value.attr */
    AST_SUBSCRIPT,                  /* value[slice] */
    AST_STARRED,                    /* *value (in assignment/call) */
    AST_NAME,                       /* Identifier reference */
    AST_LIST,                       /* [a, b, c] */
    AST_TUPLE,                      /* (a, b, c) or a, b, c */
    AST_SLICE,                      /* lower:upper:step */

    /* Expression context (stored as flag, not separate node) */
    /* Load, Store, Del */

    /* Boolean operators */
    /* And, Or */

    /* Binary operators */
    /* Add, Sub, Mult, MatMult, Div, Mod, Pow, LShift, RShift,
       BitOr, BitXor, BitAnd, FloorDiv */

    /* Unary operators */
    /* Invert (~), Not, UAdd (+), USub (-) */

    /* Comparison operators */
    /* Eq, NotEq, Lt, LtE, Gt, GtE, Is, IsNot, In, NotIn */

    /* Comprehension (helper for list/set/dict/generator comprehensions) */
    AST_COMPREHENSION,              /* for target in iter if conds */

    /* Exception handler */
    AST_EXCEPT_HANDLER,             /* except Type as name: body */

    /* Function/lambda arguments */
    AST_ARGUMENTS,                  /* Function arguments specification */
    AST_ARG,                        /* Single argument */

    /* Keyword argument */
    AST_KEYWORD,                    /* name=value in call */

    /* Alias for import */
    AST_ALIAS,                      /* name as asname */

    /* With item */
    AST_WITH_ITEM,                  /* context_expr as optional_vars */

    /* Match/case patterns (Python 3.10+) */
    AST_MATCH_VALUE,
    AST_MATCH_SINGLETON,
    AST_MATCH_SEQUENCE,
    AST_MATCH_MAPPING,
    AST_MATCH_CLASS,
    AST_MATCH_STAR,
    AST_MATCH_AS,
    AST_MATCH_OR,
    AST_MATCH_CASE

} ast_node_type_t;

/* Expression context */
typedef enum expr_context
{
    CTX_LOAD,                       /* Read value */
    CTX_STORE,                      /* Write value */
    CTX_DEL                         /* Delete value */
} expr_context_t;

/* Binary operators */
typedef enum bin_op
{
    BINOP_ADD,
    BINOP_SUB,
    BINOP_MULT,
    BINOP_MATMULT,                  /* @ operator */
    BINOP_DIV,
    BINOP_MOD,
    BINOP_POW,
    BINOP_LSHIFT,
    BINOP_RSHIFT,
    BINOP_BITOR,
    BINOP_BITXOR,
    BINOP_BITAND,
    BINOP_FLOORDIV
} bin_op_t;

/* Unary operators */
typedef enum unary_op
{
    UNARYOP_INVERT,                 /* ~ */
    UNARYOP_NOT,                    /* not */
    UNARYOP_UADD,                   /* + */
    UNARYOP_USUB                    /* - */
} unary_op_t;

/* Comparison operators */
typedef enum cmp_op
{
    CMPOP_EQ,
    CMPOP_NOTEQ,
    CMPOP_LT,
    CMPOP_LTE,
    CMPOP_GT,
    CMPOP_GTE,
    CMPOP_IS,
    CMPOP_ISNOT,
    CMPOP_IN,
    CMPOP_NOTIN
} cmp_op_t;

/* Boolean operators */
typedef enum bool_op
{
    BOOLOP_AND,
    BOOLOP_OR
} bool_op_t;

/* Forward declarations */
typedef struct ast_node ast_node_t;

/* ========================================================================
 * AST Node Structure
 * ======================================================================== */

struct ast_node
{
    ast_node_type_t type;           /* Node type */
    int line;                       /* Source line */
    int column;                     /* Source column */

    /* Node-specific data stored in a union */
    union {
        /* AST_MODULE */
        struct {
            slist_t *body;          /* List of statement nodes */
        } module;

        /* AST_FUNCTION_DEF, AST_ASYNC_FUNCTION_DEF */
        struct {
            char *name;
            ast_node_t *args;       /* AST_ARGUMENTS */
            slist_t *body;
            slist_t *decorator_list;
            ast_node_t *returns;    /* Return type annotation */
        } func_def;

        /* AST_CLASS_DEF */
        struct {
            char *name;
            slist_t *bases;         /* Base classes */
            slist_t *keywords;      /* Keyword args (metaclass, etc.) */
            slist_t *body;
            slist_t *decorator_list;
        } class_def;

        /* AST_RETURN */
        struct {
            ast_node_t *value;      /* May be NULL */
        } return_stmt;

        /* AST_DELETE */
        struct {
            slist_t *targets;
        } delete_stmt;

        /* AST_ASSIGN */
        struct {
            slist_t *targets;       /* Left-hand sides (multiple for a = b = c) */
            ast_node_t *value;
        } assign;

        /* AST_AUG_ASSIGN */
        struct {
            ast_node_t *target;
            bin_op_t op;
            ast_node_t *value;
        } aug_assign;

        /* AST_ANN_ASSIGN */
        struct {
            ast_node_t *target;
            ast_node_t *annotation;
            ast_node_t *value;      /* May be NULL */
            bool simple;            /* Is target a simple Name */
        } ann_assign;

        /* AST_FOR, AST_ASYNC_FOR */
        struct {
            ast_node_t *target;
            ast_node_t *iter;
            slist_t *body;
            slist_t *orelse;
        } for_stmt;

        /* AST_WHILE */
        struct {
            ast_node_t *test;
            slist_t *body;
            slist_t *orelse;
        } while_stmt;

        /* AST_IF */
        struct {
            ast_node_t *test;
            slist_t *body;
            slist_t *orelse;        /* elif/else chain */
        } if_stmt;

        /* AST_WITH, AST_ASYNC_WITH */
        struct {
            slist_t *items;         /* List of AST_WITH_ITEM */
            slist_t *body;
        } with_stmt;

        /* AST_RAISE */
        struct {
            ast_node_t *exc;        /* May be NULL (bare raise) */
            ast_node_t *cause;      /* May be NULL */
        } raise_stmt;

        /* AST_TRY, AST_TRY_STAR */
        struct {
            slist_t *body;
            slist_t *handlers;      /* List of AST_EXCEPT_HANDLER */
            slist_t *orelse;
            slist_t *finalbody;
        } try_stmt;

        /* AST_ASSERT */
        struct {
            ast_node_t *test;
            ast_node_t *msg;        /* May be NULL */
        } assert_stmt;

        /* AST_IMPORT */
        struct {
            slist_t *names;         /* List of AST_ALIAS */
        } import_stmt;

        /* AST_IMPORT_FROM */
        struct {
            char *module;           /* May be NULL for relative imports */
            slist_t *names;         /* List of AST_ALIAS */
            int level;              /* Relative import level (number of dots) */
        } import_from;

        /* AST_GLOBAL, AST_NONLOCAL */
        struct {
            slist_t *names;         /* List of char* */
        } global_stmt;

        /* AST_EXPR_STMT */
        struct {
            ast_node_t *value;
        } expr_stmt;

        /* AST_BOOL_OP */
        struct {
            bool_op_t op;
            slist_t *values;
        } bool_op;

        /* AST_NAMED_EXPR (walrus) */
        struct {
            ast_node_t *target;     /* Must be AST_NAME */
            ast_node_t *value;
        } named_expr;

        /* AST_BIN_OP */
        struct {
            ast_node_t *left;
            bin_op_t op;
            ast_node_t *right;
        } bin_op;

        /* AST_UNARY_OP */
        struct {
            unary_op_t op;
            ast_node_t *operand;
        } unary_op;

        /* AST_LAMBDA */
        struct {
            ast_node_t *args;       /* AST_ARGUMENTS */
            ast_node_t *body;
        } lambda;

        /* AST_IF_EXP */
        struct {
            ast_node_t *test;
            ast_node_t *body;       /* Value if true */
            ast_node_t *orelse;     /* Value if false */
        } if_exp;

        /* AST_DICT */
        struct {
            slist_t *keys;          /* May contain NULL for **d unpacking */
            slist_t *values;
        } dict;

        /* AST_SET, AST_LIST, AST_TUPLE */
        struct {
            slist_t *elts;
            expr_context_t ctx;
        } collection;

        /* AST_LIST_COMP, AST_SET_COMP, AST_GENERATOR_EXP */
        struct {
            ast_node_t *elt;
            slist_t *generators;    /* List of AST_COMPREHENSION */
        } comprehension_expr;

        /* AST_DICT_COMP */
        struct {
            ast_node_t *key;
            ast_node_t *value;
            slist_t *generators;
        } dict_comp;

        /* AST_AWAIT, AST_YIELD, AST_YIELD_FROM */
        struct {
            ast_node_t *value;      /* May be NULL for bare yield */
        } await_yield;

        /* AST_COMPARE */
        struct {
            ast_node_t *left;
            slist_t *ops;           /* List of cmp_op_t (stored as pointers) */
            slist_t *comparators;   /* List of ast_node_t* */
        } compare;

        /* AST_CALL */
        struct {
            ast_node_t *func;
            slist_t *args;          /* Positional arguments */
            slist_t *keywords;      /* Keyword arguments (AST_KEYWORD) */
        } call;

        /* AST_CONSTANT */
        struct {
            token_type_t kind;      /* Original token type */
            union {
                long long int_val;
                double float_val;
                char *str_val;
                bool bool_val;
            } value;
        } constant;

        /* AST_ATTRIBUTE */
        struct {
            ast_node_t *value;
            char *attr;
            expr_context_t ctx;
        } attribute;

        /* AST_SUBSCRIPT */
        struct {
            ast_node_t *value;
            ast_node_t *slice;
            expr_context_t ctx;
        } subscript;

        /* AST_STARRED */
        struct {
            ast_node_t *value;
            expr_context_t ctx;
        } starred;

        /* AST_NAME */
        struct {
            char *id;
            expr_context_t ctx;
        } name;

        /* AST_SLICE */
        struct {
            ast_node_t *lower;      /* May be NULL */
            ast_node_t *upper;      /* May be NULL */
            ast_node_t *step;       /* May be NULL */
        } slice;

        /* AST_COMPREHENSION */
        struct {
            ast_node_t *target;
            ast_node_t *iter;
            slist_t *ifs;
            bool is_async;
        } comprehension;

        /* AST_EXCEPT_HANDLER */
        struct {
            ast_node_t *type;       /* Exception type (may be NULL for bare except) */
            char *name;             /* Bound name (may be NULL) */
            slist_t *body;
        } except_handler;

        /* AST_ARGUMENTS */
        struct {
            slist_t *posonlyargs;   /* Positional-only (before /) */
            slist_t *args;          /* Regular positional args */
            ast_node_t *vararg;     /* *args (AST_ARG or NULL) */
            slist_t *kwonlyargs;    /* Keyword-only (after *) */
            slist_t *kw_defaults;   /* Defaults for kwonlyargs */
            ast_node_t *kwarg;      /* **kwargs (AST_ARG or NULL) */
            slist_t *defaults;      /* Defaults for args */
        } arguments;

        /* AST_ARG */
        struct {
            char *arg;              /* Argument name */
            ast_node_t *annotation; /* Type annotation (may be NULL) */
        } arg;

        /* AST_KEYWORD */
        struct {
            char *arg;              /* NULL for **kwargs unpacking */
            ast_node_t *value;
        } keyword;

        /* AST_ALIAS */
        struct {
            char *name;
            char *asname;           /* May be NULL */
        } alias;

        /* AST_WITH_ITEM */
        struct {
            ast_node_t *context_expr;
            ast_node_t *optional_vars;  /* May be NULL */
        } with_item;

        /* AST_MATCH */
        struct {
            ast_node_t *subject;
            slist_t *cases;         /* List of AST_MATCH_CASE */
        } match_stmt;

        /* AST_MATCH_CASE */
        struct {
            ast_node_t *pattern;
            ast_node_t *guard;      /* May be NULL */
            slist_t *body;
        } match_case;

        /* Pattern nodes (simplified) */
        struct {
            ast_node_t *value;      /* For MATCH_VALUE, MATCH_SINGLETON */
            char *name;             /* For MATCH_AS binding */
            slist_t *patterns;      /* For MATCH_SEQUENCE, MATCH_OR */
        } pattern;

    } data;
};

/* AST node constructors */
ast_node_t *ast_new(ast_node_type_t type, int line, int column);
void ast_free(ast_node_t *node);
void ast_print(ast_node_t *node, int indent);
const char *ast_type_name(ast_node_type_t type);

/* ========================================================================
 * Parser (feedforward with lexer)
 * ======================================================================== */

typedef struct parser
{
    lexer_t *lexer;                 /* Lexer for tokenization */
    source_file_t *source;          /* Source file for error messages */
    char *error_msg;                /* Error message if any */
    int error_line;                 /* Error line number */
    int error_column;               /* Error column number */
} parser_t;

parser_t *parser_new(lexer_t *lexer, source_file_t *source);
void parser_free(parser_t *parser);
ast_node_t *parser_parse(parser_t *parser);

/* ========================================================================
 * Scope and Symbol Table
 * ======================================================================== */

typedef enum symbol_kind
{
    SYM_VARIABLE,                   /* Local variable */
    SYM_PARAMETER,                  /* Function parameter */
    SYM_FUNCTION,                   /* Function definition */
    SYM_CLASS,                      /* Class definition */
    SYM_GLOBAL,                     /* Declared global */
    SYM_NONLOCAL,                   /* Declared nonlocal */
    SYM_FREE,                       /* Free variable (captured from enclosing scope) */
    SYM_CELL,                       /* Cell variable (captured by nested function) */
    SYM_IMPORT                      /* Imported name */
} symbol_kind_t;

typedef struct symbol
{
    char *name;
    symbol_kind_t kind;
    int scope_level;                /* Nesting level */
    int slot;                       /* Local variable slot (for codegen) */
    bool is_referenced;             /* Is this symbol used? */
    bool is_assigned;               /* Is this symbol assigned? */
} symbol_t;

typedef enum scope_type
{
    SCOPE_MODULE,
    SCOPE_CLASS,
    SCOPE_FUNCTION,
    SCOPE_COMPREHENSION             /* List/dict/set comprehension or generator */
} scope_type_t;

typedef struct scope
{
    scope_type_t type;
    struct scope *parent;
    hashtable_t *symbols;           /* name -> symbol_t* */
    slist_t *children;              /* Child scopes */
    char *name;                     /* Function/class name, or NULL for module */
    int next_slot;                  /* Next local variable slot */
    slist_t *free_vars;             /* Free variables (captured) */
    slist_t *cell_vars;             /* Cell variables (captured by nested) */
} scope_t;

scope_t *scope_new(scope_type_t type, scope_t *parent, const char *name);
void scope_free(scope_t *scope);
symbol_t *scope_define(scope_t *scope, const char *name, symbol_kind_t kind);
symbol_t *scope_lookup(scope_t *scope, const char *name);
symbol_t *scope_lookup_local(scope_t *scope, const char *name);

/* ========================================================================
 * Semantic Analyzer
 * ======================================================================== */

typedef struct analyzer
{
    scope_t *global_scope;          /* Module-level scope */
    scope_t *current_scope;         /* Current scope during analysis */
    slist_t *errors;                /* List of error messages */
    slist_t *warnings;              /* List of warning messages */
    source_file_t *source;
} analyzer_t;

analyzer_t *analyzer_new(void);
void analyzer_free(analyzer_t *analyzer);
bool analyzer_analyze(analyzer_t *analyzer, ast_node_t *ast, source_file_t *source);
void analyzer_error(analyzer_t *analyzer, int line, int col, const char *fmt, ...);
void analyzer_warning(analyzer_t *analyzer, int line, int col, const char *fmt, ...);

/* ========================================================================
 * Compilation
 * ======================================================================== */

/**
 * Compile a list of source files.
 *
 * @param opts  Compiler options
 * @return      0 on success, non-zero on failure
 */
int compile(compiler_options_t *opts);

/**
 * Print the software version.
 */
void print_version(void);

/**
 * Print usage information.
 */
void print_usage(const char *program_name);

#endif /* LORETTA_H */

