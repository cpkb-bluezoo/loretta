/*
 * loretta.c
 * Main entry point for the loretta Python to JVM compiler
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
#include "codegen.h"

/* ========================================================================
 * Compiler options
 * ======================================================================== */

compiler_options_t *compiler_options_new(void)
{
    compiler_options_t *opts = calloc(1, sizeof(compiler_options_t));
    if (opts) {
        opts->output_dir = NULL;
        opts->source_files = NULL;
        opts->verbose = false;
        opts->debug_info = true;
    }
    return opts;
}

void compiler_options_free(compiler_options_t *opts)
{
    if (!opts) {
        return;
    }
    free(opts->output_dir);
    slist_free_full(opts->source_files, free);
    free(opts);
}

/* ========================================================================
 * Source file handling
 * ======================================================================== */

source_file_t *source_file_new(const char *filename)
{
    source_file_t *src = calloc(1, sizeof(source_file_t));
    if (src) {
        src->filename = str_dup(filename);
    }
    return src;
}

void source_file_free(source_file_t *src)
{
    if (!src) {
        return;
    }
    free(src->filename);
    free(src->contents);
    free(src);
}

bool source_file_load(source_file_t *src)
{
    if (!src || !src->filename) {
        return false;
    }

    src->contents = file_get_contents(src->filename, &src->length);
    return src->contents != NULL;
}

/* ========================================================================
 * Compilation
 * ======================================================================== */

int compile(compiler_options_t *opts)
{
    if (!opts || !opts->source_files) {
        fprintf(stderr, "error: no input files\n");
        return 1;
    }

    int errors = 0;

    for (slist_t *s = opts->source_files; s; s = s->next) {
        const char *filename = s->data;

        if (opts->verbose) {
            printf("Compiling %s\n", filename);
        }

        /* Load source file */
        source_file_t *source = source_file_new(filename);
        if (!source || !source_file_load(source)) {
            fprintf(stderr, "error: cannot read file '%s'\n", filename);
            source_file_free(source);
            errors++;
            continue;
        }

        /* Lex */
        lexer_t *lexer = lexer_new(source);
        if (!lexer) {
            fprintf(stderr, "error: failed to create lexer for '%s'\n", filename);
            source_file_free(source);
            errors++;
            continue;
        }

        /* Parse */
        parser_t *parser = parser_new(lexer, source);
        if (!parser) {
            fprintf(stderr, "error: failed to create parser for '%s'\n", filename);
            lexer_free(lexer);
            source_file_free(source);
            errors++;
            continue;
        }

        ast_node_t *ast = parser_parse(parser);
        if (!ast) {
            errors++;
            parser_free(parser);
            lexer_free(lexer);
            source_file_free(source);
            continue;
        }

        if (opts->verbose) {
            printf("AST for %s:\n", filename);
            ast_print(ast, 0);
        }

        /* Analyze */
        analyzer_t *analyzer = analyzer_new();
        if (!analyzer || !analyzer_analyze(analyzer, ast, source)) {
            errors++;
            analyzer_free(analyzer);
            ast_free(ast);
            parser_free(parser);
            lexer_free(lexer);
            source_file_free(source);
            continue;
        }

        /* Generate code */
        if (codegen_module(ast, analyzer, source, opts) != 0) {
            errors++;
        }

        /* Cleanup */
        analyzer_free(analyzer);
        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
        source_file_free(source);
    }

    return errors > 0 ? 1 : 0;
}

/* ========================================================================
 * Version and usage
 * ======================================================================== */

void print_version(void)
{
    printf("loretta %s\n", LORETTA_VERSION);
    printf("Python 3 to JVM bytecode compiler\n");
    printf("Copyright (C) 2026 Chris Burdess\n");
    printf("License: GPLv3+\n");
}

void print_usage(const char *program_name)
{
    printf("Usage: %s [options] <source files>\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -d <dir>       Output directory for class files\n");
    printf("  -v, --verbose  Verbose output\n");
    printf("  -g             Generate debug information (default)\n");
    printf("  -version       Print version and exit\n");
    printf("  -help          Print this help and exit\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s -d build hello.py\n", program_name);
}

/* ========================================================================
 * Main entry point
 * ======================================================================== */

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    compiler_options_t *opts = compiler_options_new();
    if (!opts) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-version") == 0 || strcmp(arg, "--version") == 0) {
            print_version();
            compiler_options_free(opts);
            return 0;
        }

        if (strcmp(arg, "-help") == 0 || strcmp(arg, "--help") == 0 ||
            strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            compiler_options_free(opts);
            return 0;
        }

        if (strcmp(arg, "-d") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: -d requires an argument\n");
                compiler_options_free(opts);
                return 1;
            }
            opts->output_dir = str_dup(argv[++i]);
            continue;
        }

        if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0) {
            opts->verbose = true;
            continue;
        }

        if (strcmp(arg, "-g") == 0) {
            opts->debug_info = true;
            continue;
        }

        if (arg[0] == '-') {
            fprintf(stderr, "error: unknown option '%s'\n", arg);
            compiler_options_free(opts);
            return 1;
        }

        /* Assume it's a source file */
        opts->source_files = slist_append(opts->source_files, str_dup(arg));
    }

    if (!opts->source_files) {
        fprintf(stderr, "error: no input files\n");
        print_usage(argv[0]);
        compiler_options_free(opts);
        return 1;
    }

    int result = compile(opts);

    compiler_options_free(opts);
    return result;
}

