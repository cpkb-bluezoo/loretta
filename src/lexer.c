/*
 * lexer.c
 * Python 3 lexical analyzer (feedforward style)
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
 * Token type names
 * ======================================================================== */

const char *token_type_name(token_type_t type)
{
    switch (type) {
        case TOK_EOF:           return "EOF";
        case TOK_NEWLINE:       return "NEWLINE";
        case TOK_INDENT:        return "INDENT";
        case TOK_DEDENT:        return "DEDENT";
        case TOK_IDENTIFIER:    return "IDENTIFIER";
        case TOK_INTEGER:       return "INTEGER";
        case TOK_FLOAT:         return "FLOAT";
        case TOK_IMAGINARY:     return "IMAGINARY";
        case TOK_STRING:        return "STRING";
        case TOK_BYTES:         return "BYTES";
        case TOK_FSTRING_START: return "FSTRING_START";
        case TOK_FSTRING_MIDDLE: return "FSTRING_MIDDLE";
        case TOK_FSTRING_END:   return "FSTRING_END";

        /* Keywords */
        case TOK_FALSE:         return "False";
        case TOK_NONE:          return "None";
        case TOK_TRUE:          return "True";
        case TOK_AND:           return "and";
        case TOK_AS:            return "as";
        case TOK_ASSERT:        return "assert";
        case TOK_ASYNC:         return "async";
        case TOK_AWAIT:         return "await";
        case TOK_BREAK:         return "break";
        case TOK_CLASS:         return "class";
        case TOK_CONTINUE:      return "continue";
        case TOK_DEF:           return "def";
        case TOK_DEL:           return "del";
        case TOK_ELIF:          return "elif";
        case TOK_ELSE:          return "else";
        case TOK_EXCEPT:        return "except";
        case TOK_FINALLY:       return "finally";
        case TOK_FOR:           return "for";
        case TOK_FROM:          return "from";
        case TOK_GLOBAL:        return "global";
        case TOK_IF:            return "if";
        case TOK_IMPORT:        return "import";
        case TOK_IN:            return "in";
        case TOK_IS:            return "is";
        case TOK_LAMBDA:        return "lambda";
        case TOK_NONLOCAL:      return "nonlocal";
        case TOK_NOT:           return "not";
        case TOK_OR:            return "or";
        case TOK_PASS:          return "pass";
        case TOK_RAISE:         return "raise";
        case TOK_RETURN:        return "return";
        case TOK_TRY:           return "try";
        case TOK_WHILE:         return "while";
        case TOK_WITH:          return "with";
        case TOK_YIELD:         return "yield";
        case TOK_MATCH:         return "match";
        case TOK_CASE:          return "case";
        case TOK_TYPE:          return "type";

        /* Operators */
        case TOK_PLUS:          return "+";
        case TOK_MINUS:         return "-";
        case TOK_STAR:          return "*";
        case TOK_DOUBLESTAR:    return "**";
        case TOK_SLASH:         return "/";
        case TOK_DOUBLESLASH:   return "//";
        case TOK_PERCENT:       return "%";
        case TOK_AT:            return "@";
        case TOK_LSHIFT:        return "<<";
        case TOK_RSHIFT:        return ">>";
        case TOK_AMPERSAND:     return "&";
        case TOK_PIPE:          return "|";
        case TOK_CARET:         return "^";
        case TOK_TILDE:         return "~";
        case TOK_WALRUS:        return ":=";
        case TOK_LT:            return "<";
        case TOK_GT:            return ">";
        case TOK_LE:            return "<=";
        case TOK_GE:            return ">=";
        case TOK_EQ:            return "==";
        case TOK_NE:            return "!=";

        /* Delimiters */
        case TOK_LPAREN:        return "(";
        case TOK_RPAREN:        return ")";
        case TOK_LBRACKET:      return "[";
        case TOK_RBRACKET:      return "]";
        case TOK_LBRACE:        return "{";
        case TOK_RBRACE:        return "}";
        case TOK_COMMA:         return ",";
        case TOK_COLON:         return ":";
        case TOK_DOT:           return ".";
        case TOK_SEMICOLON:     return ";";
        case TOK_ASSIGN:        return "=";
        case TOK_ARROW:         return "->";
        case TOK_ELLIPSIS:      return "...";

        /* Augmented assignment */
        case TOK_PLUSEQ:        return "+=";
        case TOK_MINUSEQ:       return "-=";
        case TOK_STAREQ:        return "*=";
        case TOK_SLASHEQ:       return "/=";
        case TOK_DOUBLESLASHEQ: return "//=";
        case TOK_PERCENTEQ:     return "%=";
        case TOK_ATSTAREQ:      return "@=";
        case TOK_AMPERSANDEQ:   return "&=";
        case TOK_PIPEEQ:        return "|=";
        case TOK_CARETEQ:       return "^=";
        case TOK_RSHIFTEQ:      return ">>=";
        case TOK_LSHIFTEQ:      return "<<=";
        case TOK_DOUBLESTAREQ:  return "**=";

        case TOK_ERROR:         return "ERROR";
        default:                return "UNKNOWN";
    }
}

/* ========================================================================
 * Keyword lookup
 * ======================================================================== */

typedef struct keyword_entry
{
    const char *name;
    token_type_t type;
} keyword_entry_t;

static keyword_entry_t keywords[] = {
    {"False",    TOK_FALSE},
    {"None",     TOK_NONE},
    {"True",     TOK_TRUE},
    {"and",      TOK_AND},
    {"as",       TOK_AS},
    {"assert",   TOK_ASSERT},
    {"async",    TOK_ASYNC},
    {"await",    TOK_AWAIT},
    {"break",    TOK_BREAK},
    {"class",    TOK_CLASS},
    {"continue", TOK_CONTINUE},
    {"def",      TOK_DEF},
    {"del",      TOK_DEL},
    {"elif",     TOK_ELIF},
    {"else",     TOK_ELSE},
    {"except",   TOK_EXCEPT},
    {"finally",  TOK_FINALLY},
    {"for",      TOK_FOR},
    {"from",     TOK_FROM},
    {"global",   TOK_GLOBAL},
    {"if",       TOK_IF},
    {"import",   TOK_IMPORT},
    {"in",       TOK_IN},
    {"is",       TOK_IS},
    {"lambda",   TOK_LAMBDA},
    {"nonlocal", TOK_NONLOCAL},
    {"not",      TOK_NOT},
    {"or",       TOK_OR},
    {"pass",     TOK_PASS},
    {"raise",    TOK_RAISE},
    {"return",   TOK_RETURN},
    {"try",      TOK_TRY},
    {"while",    TOK_WHILE},
    {"with",     TOK_WITH},
    {"yield",    TOK_YIELD},
    /* Soft keywords (context-dependent in full parser) */
    {"match",    TOK_MATCH},
    {"case",     TOK_CASE},
    {"type",     TOK_TYPE},
    {NULL,       TOK_EOF}
};

static token_type_t lookup_keyword(const char *name)
{
    for (int i = 0; keywords[i].name != NULL; i++) {
        if (strcmp(keywords[i].name, name) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENTIFIER;
}

/* ========================================================================
 * Lexer helper functions
 * ======================================================================== */

static void lexer_set_token(lexer_t *lexer, token_type_t type,
                           const char *text_start, size_t text_len,
                           int line, int column)
{
    lexer->token.type = type;
    lexer->token.text_start = text_start;
    lexer->token.text_len = text_len;
    lexer->token.line = line;
    lexer->token.column = column;
    lexer->token.value.int_value = 0;
}

static void lexer_set_token_buf(lexer_t *lexer, token_type_t type,
                               const char *text, size_t text_len,
                               int line, int column)
{
    if (text_len >= sizeof(lexer->text_buf)) {
        text_len = sizeof(lexer->text_buf) - 1;
    }
    memcpy(lexer->text_buf, text, text_len);
    lexer->text_buf[text_len] = '\0';

    lexer->token.type = type;
    lexer->token.text_start = lexer->text_buf;
    lexer->token.text_len = text_len;
    lexer->token.line = line;
    lexer->token.column = column;
    lexer->token.value.int_value = 0;
}

static void lexer_set_token_static(lexer_t *lexer, token_type_t type,
                                  const char *static_text,
                                  int line, int column)
{
    lexer->token.type = type;
    lexer->token.text_start = static_text;
    lexer->token.text_len = strlen(static_text);
    lexer->token.line = line;
    lexer->token.column = column;
    lexer->token.value.int_value = 0;
}

static char lexer_peek(lexer_t *lexer)
{
    if (lexer->pos >= lexer->end) {
        return '\0';
    }
    return *lexer->pos;
}

static char lexer_peek_ahead(lexer_t *lexer, int offset)
{
    if (lexer->pos + offset >= lexer->end) {
        return '\0';
    }
    return lexer->pos[offset];
}

static void lexer_advance_char(lexer_t *lexer)
{
    if (lexer->pos >= lexer->end) {
        return;
    }
    if (*lexer->pos == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->pos++;
}

static bool is_identifier_start(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
    /* Note: Full Python allows Unicode, but we limit to ASCII for simplicity */
}

static bool is_identifier_part(char c)
{
    return is_identifier_start(c) || (c >= '0' && c <= '9');
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

/* ========================================================================
 * Token scanning
 * ======================================================================== */

static void lexer_scan_identifier(lexer_t *lexer)
{
    int start_line = lexer->line;
    int start_column = lexer->column;
    const char *start = lexer->pos;

    while (lexer->pos < lexer->end && is_identifier_part(lexer_peek(lexer))) {
        lexer_advance_char(lexer);
    }

    size_t len = lexer->pos - start;

    /* Copy to buffer for keyword lookup */
    if (len < sizeof(lexer->text_buf)) {
        memcpy(lexer->text_buf, start, len);
        lexer->text_buf[len] = '\0';
    }

    token_type_t type = lookup_keyword(lexer->text_buf);
    lexer_set_token(lexer, type, start, len, start_line, start_column);
}

static void lexer_scan_number(lexer_t *lexer)
{
    int start_line = lexer->line;
    int start_column = lexer->column;
    const char *start = lexer->pos;
    token_type_t type = TOK_INTEGER;
    bool is_float = false;
    bool is_imaginary = false;

    /* Check for hex, octal, binary prefixes */
    if (lexer_peek(lexer) == '0') {
        char next = lexer_peek_ahead(lexer, 1);
        if (next == 'x' || next == 'X') {
            /* Hex */
            lexer_advance_char(lexer);
            lexer_advance_char(lexer);
            while (lexer->pos < lexer->end) {
                char c = lexer_peek(lexer);
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F') || c == '_') {
                    lexer_advance_char(lexer);
                } else {
                    break;
                }
            }
            goto finish_number;
        } else if (next == 'o' || next == 'O') {
            /* Octal */
            lexer_advance_char(lexer);
            lexer_advance_char(lexer);
            while (lexer->pos < lexer->end) {
                char c = lexer_peek(lexer);
                if ((c >= '0' && c <= '7') || c == '_') {
                    lexer_advance_char(lexer);
                } else {
                    break;
                }
            }
            goto finish_number;
        } else if (next == 'b' || next == 'B') {
            /* Binary */
            lexer_advance_char(lexer);
            lexer_advance_char(lexer);
            while (lexer->pos < lexer->end) {
                char c = lexer_peek(lexer);
                if (c == '0' || c == '1' || c == '_') {
                    lexer_advance_char(lexer);
                } else {
                    break;
                }
            }
            goto finish_number;
        }
    }

    /* Decimal integer or float */
    while (lexer->pos < lexer->end) {
        char c = lexer_peek(lexer);
        if (is_digit(c) || c == '_') {
            lexer_advance_char(lexer);
        } else {
            break;
        }
    }

    /* Check for decimal point */
    if (lexer_peek(lexer) == '.' && is_digit(lexer_peek_ahead(lexer, 1))) {
        is_float = true;
        lexer_advance_char(lexer);  /* Skip '.' */
        while (lexer->pos < lexer->end) {
            char c = lexer_peek(lexer);
            if (is_digit(c) || c == '_') {
                lexer_advance_char(lexer);
            } else {
                break;
            }
        }
    }

    /* Check for exponent */
    char c = lexer_peek(lexer);
    if (c == 'e' || c == 'E') {
        is_float = true;
        lexer_advance_char(lexer);
        c = lexer_peek(lexer);
        if (c == '+' || c == '-') {
            lexer_advance_char(lexer);
        }
        while (lexer->pos < lexer->end) {
            c = lexer_peek(lexer);
            if (is_digit(c) || c == '_') {
                lexer_advance_char(lexer);
            } else {
                break;
            }
        }
    }

finish_number:
    /* Check for imaginary suffix */
    c = lexer_peek(lexer);
    if (c == 'j' || c == 'J') {
        is_imaginary = true;
        lexer_advance_char(lexer);
    }

    if (is_imaginary) {
        type = TOK_IMAGINARY;
    } else if (is_float) {
        type = TOK_FLOAT;
    }

    size_t len = lexer->pos - start;
    lexer_set_token(lexer, type, start, len, start_line, start_column);

    /* Parse numeric value */
    if (len < sizeof(lexer->text_buf)) {
        /* Remove underscores for parsing */
        size_t j = 0;
        for (size_t i = 0; i < len; i++) {
            if (start[i] != '_' && start[i] != 'j' && start[i] != 'J') {
                lexer->text_buf[j++] = start[i];
            }
        }
        lexer->text_buf[j] = '\0';

        if (type == TOK_FLOAT || type == TOK_IMAGINARY) {
            lexer->token.value.float_value = strtod(lexer->text_buf, NULL);
        } else {
            /* Detect base from prefix */
            if (len > 2 && start[0] == '0') {
                if (start[1] == 'x' || start[1] == 'X') {
                    lexer->token.value.int_value = strtoll(lexer->text_buf, NULL, 16);
                } else if (start[1] == 'o' || start[1] == 'O') {
                    lexer->token.value.int_value = strtoll(lexer->text_buf + 2, NULL, 8);
                } else if (start[1] == 'b' || start[1] == 'B') {
                    lexer->token.value.int_value = strtoll(lexer->text_buf + 2, NULL, 2);
                } else {
                    lexer->token.value.int_value = strtoll(lexer->text_buf, NULL, 10);
                }
            } else {
                lexer->token.value.int_value = strtoll(lexer->text_buf, NULL, 10);
            }
        }
    }
}

static void lexer_scan_string(lexer_t *lexer, bool is_bytes, bool is_fstring, bool is_raw)
{
    int start_line = lexer->line;
    int start_column = lexer->column;
    size_t buf_pos = 0;

    char quote = lexer_peek(lexer);
    bool triple = false;

    lexer_advance_char(lexer);
    if (lexer_peek(lexer) == quote && lexer_peek_ahead(lexer, 1) == quote) {
        triple = true;
        lexer_advance_char(lexer);
        lexer_advance_char(lexer);
    }

    while (lexer->pos < lexer->end) {
        char c = lexer_peek(lexer);

        if (!triple && c == '\n') {
            /* Unterminated string */
            lexer_set_token_static(lexer, TOK_ERROR, "Unterminated string",
                                  start_line, start_column);
            return;
        }

        if (c == quote) {
            if (triple) {
                if (lexer_peek_ahead(lexer, 1) == quote &&
                    lexer_peek_ahead(lexer, 2) == quote) {
                    lexer_advance_char(lexer);
                    lexer_advance_char(lexer);
                    lexer_advance_char(lexer);
                    break;
                }
            } else {
                lexer_advance_char(lexer);
                break;
            }
        }

        if (c == '\\') {
            lexer_advance_char(lexer);
            if (lexer->pos >= lexer->end) {
                break;
            }
            c = lexer_peek(lexer);
            if (is_raw) {
                /* Raw string: backslash only escapes the closing quote (so it's not stored) */
                if (c != quote) {
                    if (buf_pos < sizeof(lexer->text_buf) - 1) {
                        lexer->text_buf[buf_pos++] = '\\';
                    }
                    if (buf_pos < sizeof(lexer->text_buf) - 1) {
                        lexer->text_buf[buf_pos++] = c;
                    }
                } else {
                    if (buf_pos < sizeof(lexer->text_buf) - 1) {
                        lexer->text_buf[buf_pos++] = '\\';
                    }
                }
            } else {
                char esc;
                switch (c) {
                    case 'n':  esc = '\n'; break;
                    case 't':  esc = '\t'; break;
                    case 'r':  esc = '\r'; break;
                    case '\\': esc = '\\'; break;
                    case '\'': esc = '\''; break;
                    case '"':  esc = '"'; break;
                    case '0':  esc = '\0'; break;
                    case '\n':
                        /* Line continuation */
                        lexer_advance_char(lexer);
                        continue;
                    default:   esc = c; break;
                }
                if (buf_pos < sizeof(lexer->text_buf) - 1) {
                    lexer->text_buf[buf_pos++] = esc;
                }
            }
        } else {
            if (buf_pos < sizeof(lexer->text_buf) - 1) {
                lexer->text_buf[buf_pos++] = c;
            }
        }
        lexer_advance_char(lexer);
    }

    lexer->text_buf[buf_pos] = '\0';

    token_type_t type;
    if (is_fstring) {
        /* Simplified: treat f-string as regular string for now */
        type = TOK_STRING;
    } else if (is_bytes) {
        type = TOK_BYTES;
    } else {
        type = TOK_STRING;
    }

    lexer_set_token_buf(lexer, type, lexer->text_buf, buf_pos,
                       start_line, start_column);
}

/* ========================================================================
 * Indentation handling
 * ======================================================================== */

static int lexer_count_indent(lexer_t *lexer)
{
    int indent = 0;
    while (lexer->pos < lexer->end) {
        char c = lexer_peek(lexer);
        if (c == ' ') {
            indent++;
            lexer_advance_char(lexer);
        } else if (c == '\t') {
            /* Tab = align to multiple of 8 */
            indent = (indent / 8 + 1) * 8;
            lexer_advance_char(lexer);
        } else {
            break;
        }
    }
    return indent;
}

/* ========================================================================
 * Main lexer functions
 * ======================================================================== */

lexer_t *lexer_new(source_file_t *source)
{
    if (!source || !source->contents) {
        return NULL;
    }

    lexer_t *lexer = malloc(sizeof(lexer_t));
    if (!lexer) {
        return NULL;
    }

    lexer->source = source;
    lexer->pos = source->contents;
    lexer->end = source->contents + source->length;
    lexer->line = 1;
    lexer->column = 1;
    lexer->error_msg = NULL;

    /* Initialize indentation tracking */
    lexer->indent_stack[0] = 0;
    lexer->indent_depth = 0;
    lexer->pending_dedents = 0;
    lexer->at_line_start = true;
    lexer->paren_depth = 0;

    /* Initialize token state */
    lexer->token.type = TOK_EOF;
    lexer->token.text_start = "";
    lexer->token.text_len = 0;
    lexer->token.line = 1;
    lexer->token.column = 1;
    lexer->token.value.int_value = 0;
    lexer->text_buf[0] = '\0';

    /* Advance to first token */
    lexer_advance(lexer);

    return lexer;
}

void lexer_free(lexer_t *lexer)
{
    if (!lexer) {
        return;
    }
    free(lexer->error_msg);
    free(lexer);
}

void lexer_advance(lexer_t *lexer)
{
    /* Handle pending DEDENT tokens */
    if (lexer->pending_dedents > 0) {
        lexer->pending_dedents--;
        lexer_set_token_static(lexer, TOK_DEDENT, "", lexer->line, lexer->column);
        return;
    }

    /* Skip whitespace and comments, handle newlines */
    while (lexer->pos < lexer->end) {
        char c = lexer_peek(lexer);

        /* Handle line start (indentation) */
        if (lexer->at_line_start && lexer->paren_depth == 0) {
            int indent = lexer_count_indent(lexer);

            /* Skip blank lines and comment-only lines */
            c = lexer_peek(lexer);
            if (c == '\n' || c == '\r' || c == '#') {
                if (c == '#') {
                    while (lexer->pos < lexer->end && lexer_peek(lexer) != '\n') {
                        lexer_advance_char(lexer);
                    }
                }
                if (lexer_peek(lexer) == '\n') {
                    lexer_advance_char(lexer);
                }
                continue;
            }

            if (c == '\0') {
                /* End of file - emit DEDENT tokens */
                if (lexer->indent_depth > 0) {
                    lexer->pending_dedents = lexer->indent_depth - 1;
                    lexer->indent_depth = 0;
                    lexer_set_token_static(lexer, TOK_DEDENT, "", lexer->line, lexer->column);
                    return;
                }
                break;
            }

            lexer->at_line_start = false;

            int current_indent = lexer->indent_stack[lexer->indent_depth];

            if (indent > current_indent) {
                /* Indent */
                if (lexer->indent_depth < MAX_INDENT_STACK - 1) {
                    lexer->indent_depth++;
                    lexer->indent_stack[lexer->indent_depth] = indent;
                }
                lexer_set_token_static(lexer, TOK_INDENT, "", lexer->line, lexer->column);
                return;
            } else if (indent < current_indent) {
                /* Dedent(s) */
                while (lexer->indent_depth > 0 &&
                       indent < lexer->indent_stack[lexer->indent_depth]) {
                    lexer->indent_depth--;
                    lexer->pending_dedents++;
                }
                if (indent != lexer->indent_stack[lexer->indent_depth]) {
                    lexer_set_token_static(lexer, TOK_ERROR,
                                          "Inconsistent indentation",
                                          lexer->line, lexer->column);
                    return;
                }
                if (lexer->pending_dedents > 0) {
                    lexer->pending_dedents--;
                    lexer_set_token_static(lexer, TOK_DEDENT, "",
                                          lexer->line, lexer->column);
                    return;
                }
            }
            /* If indent == current_indent, continue to scan token */
        }

        /* Skip spaces (not at line start) */
        if (c == ' ' || c == '\t') {
            lexer_advance_char(lexer);
            continue;
        }

        /* Skip comments */
        if (c == '#') {
            while (lexer->pos < lexer->end && lexer_peek(lexer) != '\n') {
                lexer_advance_char(lexer);
            }
            continue;
        }

        /* Handle newlines */
        if (c == '\n' || c == '\r') {
            if (lexer->paren_depth > 0) {
                /* Implicit line continuation inside (), [], {} */
                lexer_advance_char(lexer);
                continue;
            }
            lexer_advance_char(lexer);
            lexer->at_line_start = true;
            lexer_set_token_static(lexer, TOK_NEWLINE, "\n",
                                  lexer->line - 1, lexer->column);
            return;
        }

        /* Line continuation */
        if (c == '\\' && lexer_peek_ahead(lexer, 1) == '\n') {
            lexer_advance_char(lexer);
            lexer_advance_char(lexer);
            continue;
        }

        break;
    }

    if (lexer->pos >= lexer->end) {
        /* Emit final DEDENTs at EOF */
        if (lexer->indent_depth > 0) {
            lexer->pending_dedents = lexer->indent_depth - 1;
            lexer->indent_depth = 0;
            lexer_set_token_static(lexer, TOK_DEDENT, "", lexer->line, lexer->column);
            return;
        }
        lexer_set_token_static(lexer, TOK_EOF, "", lexer->line, lexer->column);
        return;
    }

    int start_line = lexer->line;
    int start_column = lexer->column;
    char c = lexer_peek(lexer);

    /* Identifiers and keywords */
    if (is_identifier_start(c)) {
        /* Check for string prefixes: b, r, f, br, rb, fr, rf */
        bool is_bytes = false;
        bool is_raw = false;
        bool is_fstring = false;

        if ((c == 'b' || c == 'B') &&
            (lexer_peek_ahead(lexer, 1) == '\'' || lexer_peek_ahead(lexer, 1) == '"')) {
            is_bytes = true;
            lexer_advance_char(lexer);
        } else if ((c == 'r' || c == 'R') &&
            (lexer_peek_ahead(lexer, 1) == '\'' || lexer_peek_ahead(lexer, 1) == '"')) {
            is_raw = true;
            lexer_advance_char(lexer);
        } else if ((c == 'f' || c == 'F') &&
            (lexer_peek_ahead(lexer, 1) == '\'' || lexer_peek_ahead(lexer, 1) == '"')) {
            is_fstring = true;
            lexer_advance_char(lexer);
        }

        c = lexer_peek(lexer);
        if (c == '\'' || c == '"') {
            lexer_scan_string(lexer, is_bytes, is_fstring, is_raw);
            return;
        }

        lexer_scan_identifier(lexer);
        return;
    }

    /* Numbers */
    if (is_digit(c) || (c == '.' && is_digit(lexer_peek_ahead(lexer, 1)))) {
        lexer_scan_number(lexer);
        return;
    }

    /* Strings */
    if (c == '\'' || c == '"') {
        lexer_scan_string(lexer, false, false, false);
        return;
    }

    /* Operators and punctuation */
    lexer_advance_char(lexer);

    switch (c) {
        case '(':
            lexer->paren_depth++;
            lexer_set_token_static(lexer, TOK_LPAREN, "(", start_line, start_column);
            return;
        case ')':
            if (lexer->paren_depth > 0) {
                lexer->paren_depth--;
            }
            lexer_set_token_static(lexer, TOK_RPAREN, ")", start_line, start_column);
            return;
        case '[':
            lexer->paren_depth++;
            lexer_set_token_static(lexer, TOK_LBRACKET, "[", start_line, start_column);
            return;
        case ']':
            if (lexer->paren_depth > 0) {
                lexer->paren_depth--;
            }
            lexer_set_token_static(lexer, TOK_RBRACKET, "]", start_line, start_column);
            return;
        case '{':
            lexer->paren_depth++;
            lexer_set_token_static(lexer, TOK_LBRACE, "{", start_line, start_column);
            return;
        case '}':
            if (lexer->paren_depth > 0) {
                lexer->paren_depth--;
            }
            lexer_set_token_static(lexer, TOK_RBRACE, "}", start_line, start_column);
            return;
        case ',':
            lexer_set_token_static(lexer, TOK_COMMA, ",", start_line, start_column);
            return;
        case ';':
            lexer_set_token_static(lexer, TOK_SEMICOLON, ";", start_line, start_column);
            return;
        case '~':
            lexer_set_token_static(lexer, TOK_TILDE, "~", start_line, start_column);
            return;
        case '@':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_ATSTAREQ, "@=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_AT, "@", start_line, start_column);
            return;

        case ':':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_WALRUS, ":=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_COLON, ":", start_line, start_column);
            return;

        case '.':
            if (lexer_peek(lexer) == '.' && lexer_peek_ahead(lexer, 1) == '.') {
                lexer_advance_char(lexer);
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_ELLIPSIS, "...", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_DOT, ".", start_line, start_column);
            return;

        case '+':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_PLUSEQ, "+=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_PLUS, "+", start_line, start_column);
            return;

        case '-':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_MINUSEQ, "-=", start_line, start_column);
                return;
            }
            if (lexer_peek(lexer) == '>') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_ARROW, "->", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_MINUS, "-", start_line, start_column);
            return;

        case '*':
            if (lexer_peek(lexer) == '*') {
                lexer_advance_char(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance_char(lexer);
                    lexer_set_token_static(lexer, TOK_DOUBLESTAREQ, "**=", start_line, start_column);
                    return;
                }
                lexer_set_token_static(lexer, TOK_DOUBLESTAR, "**", start_line, start_column);
                return;
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_STAREQ, "*=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_STAR, "*", start_line, start_column);
            return;

        case '/':
            if (lexer_peek(lexer) == '/') {
                lexer_advance_char(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance_char(lexer);
                    lexer_set_token_static(lexer, TOK_DOUBLESLASHEQ, "//=", start_line, start_column);
                    return;
                }
                lexer_set_token_static(lexer, TOK_DOUBLESLASH, "//", start_line, start_column);
                return;
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_SLASHEQ, "/=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_SLASH, "/", start_line, start_column);
            return;

        case '%':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_PERCENTEQ, "%=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_PERCENT, "%", start_line, start_column);
            return;

        case '&':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_AMPERSANDEQ, "&=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_AMPERSAND, "&", start_line, start_column);
            return;

        case '|':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_PIPEEQ, "|=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_PIPE, "|", start_line, start_column);
            return;

        case '^':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_CARETEQ, "^=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_CARET, "^", start_line, start_column);
            return;

        case '<':
            if (lexer_peek(lexer) == '<') {
                lexer_advance_char(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance_char(lexer);
                    lexer_set_token_static(lexer, TOK_LSHIFTEQ, "<<=", start_line, start_column);
                    return;
                }
                lexer_set_token_static(lexer, TOK_LSHIFT, "<<", start_line, start_column);
                return;
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_LE, "<=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_LT, "<", start_line, start_column);
            return;

        case '>':
            if (lexer_peek(lexer) == '>') {
                lexer_advance_char(lexer);
                if (lexer_peek(lexer) == '=') {
                    lexer_advance_char(lexer);
                    lexer_set_token_static(lexer, TOK_RSHIFTEQ, ">>=", start_line, start_column);
                    return;
                }
                lexer_set_token_static(lexer, TOK_RSHIFT, ">>", start_line, start_column);
                return;
            }
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_GE, ">=", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_GT, ">", start_line, start_column);
            return;

        case '=':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_EQ, "==", start_line, start_column);
                return;
            }
            lexer_set_token_static(lexer, TOK_ASSIGN, "=", start_line, start_column);
            return;

        case '!':
            if (lexer_peek(lexer) == '=') {
                lexer_advance_char(lexer);
                lexer_set_token_static(lexer, TOK_NE, "!=", start_line, start_column);
                return;
            }
            snprintf(lexer->text_buf, sizeof(lexer->text_buf),
                    "Unexpected character: '%c'", c);
            lexer_set_token_buf(lexer, TOK_ERROR, lexer->text_buf,
                               strlen(lexer->text_buf), start_line, start_column);
            return;

        default:
            snprintf(lexer->text_buf, sizeof(lexer->text_buf),
                    "Unexpected character: '%c'", c);
            lexer_set_token_buf(lexer, TOK_ERROR, lexer->text_buf,
                               strlen(lexer->text_buf), start_line, start_column);
            return;
    }
}

/* ========================================================================
 * Lexer accessor functions
 * ======================================================================== */

token_type_t lexer_type(lexer_t *lexer)
{
    return lexer->token.type;
}

const char *lexer_text(lexer_t *lexer)
{
    if (lexer->token.text_start == lexer->text_buf) {
        return lexer->text_buf;
    }

    size_t len = lexer->token.text_len;
    if (len >= sizeof(lexer->text_buf)) {
        len = sizeof(lexer->text_buf) - 1;
    }
    memcpy(lexer->text_buf, lexer->token.text_start, len);
    lexer->text_buf[len] = '\0';
    return lexer->text_buf;
}

size_t lexer_text_len(lexer_t *lexer)
{
    return lexer->token.text_len;
}

int lexer_line(lexer_t *lexer)
{
    return lexer->token.line;
}

int lexer_column(lexer_t *lexer)
{
    return lexer->token.column;
}

long long lexer_int_value(lexer_t *lexer)
{
    return lexer->token.value.int_value;
}

double lexer_float_value(lexer_t *lexer)
{
    return lexer->token.value.float_value;
}

lexer_pos_t lexer_save_pos(lexer_t *lexer)
{
    lexer_pos_t p;
    p.pos = lexer->pos;
    p.line = lexer->line;
    p.column = lexer->column;
    p.token_type = lexer->token.type;
    p.token_text_start = lexer->token.text_start;
    p.token_text_len = lexer->token.text_len;
    p.token_line = lexer->token.line;
    p.token_column = lexer->token.column;
    p.indent_depth = lexer->indent_depth;
    p.pending_dedents = lexer->pending_dedents;
    p.at_line_start = lexer->at_line_start;
    p.paren_depth = lexer->paren_depth;
    return p;
}

void lexer_restore_pos(lexer_t *lexer, lexer_pos_t p)
{
    lexer->pos = p.pos;
    lexer->line = p.line;
    lexer->column = p.column;
    lexer->token.type = p.token_type;
    lexer->token.text_start = p.token_text_start;
    lexer->token.text_len = p.token_text_len;
    lexer->token.line = p.token_line;
    lexer->token.column = p.token_column;
    lexer->indent_depth = p.indent_depth;
    lexer->pending_dedents = p.pending_dedents;
    lexer->at_line_start = p.at_line_start;
    lexer->paren_depth = p.paren_depth;
}

