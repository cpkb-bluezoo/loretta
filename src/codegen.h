/*
 * codegen.h
 * Code generation for loretta
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

#ifndef CODEGEN_H
#define CODEGEN_H

#include "loretta.h"
#include "constpool.h"
#include "classwriter.h"
#include "indy.h"
#include "stackmap.h"

/* ========================================================================
 * JVM Bytecode Instructions
 * ======================================================================== */

/* Constants */
#define OP_NOP              0x00
#define OP_ACONST_NULL      0x01
#define OP_ICONST_M1        0x02
#define OP_ICONST_0         0x03
#define OP_ICONST_1         0x04
#define OP_ICONST_2         0x05
#define OP_ICONST_3         0x06
#define OP_ICONST_4         0x07
#define OP_ICONST_5         0x08
#define OP_LCONST_0         0x09
#define OP_LCONST_1         0x0A
#define OP_FCONST_0         0x0B
#define OP_FCONST_1         0x0C
#define OP_FCONST_2         0x0D
#define OP_DCONST_0         0x0E
#define OP_DCONST_1         0x0F
#define OP_BIPUSH           0x10
#define OP_SIPUSH           0x11
#define OP_LDC              0x12
#define OP_LDC_W            0x13
#define OP_LDC2_W           0x14

/* Loads */
#define OP_ILOAD            0x15
#define OP_LLOAD            0x16
#define OP_FLOAD            0x17
#define OP_DLOAD            0x18
#define OP_ALOAD            0x19
#define OP_ILOAD_0          0x1A
#define OP_ILOAD_1          0x1B
#define OP_ILOAD_2          0x1C
#define OP_ILOAD_3          0x1D
#define OP_LLOAD_0          0x1E
#define OP_LLOAD_1          0x1F
#define OP_LLOAD_2          0x20
#define OP_LLOAD_3          0x21
#define OP_FLOAD_0          0x22
#define OP_FLOAD_1          0x23
#define OP_FLOAD_2          0x24
#define OP_FLOAD_3          0x25
#define OP_DLOAD_0          0x26
#define OP_DLOAD_1          0x27
#define OP_DLOAD_2          0x28
#define OP_DLOAD_3          0x29
#define OP_ALOAD_0          0x2A
#define OP_ALOAD_1          0x2B
#define OP_ALOAD_2          0x2C
#define OP_ALOAD_3          0x2D
#define OP_IALOAD           0x2E
#define OP_LALOAD           0x2F
#define OP_FALOAD           0x30
#define OP_DALOAD           0x31
#define OP_AALOAD           0x32
#define OP_BALOAD           0x33
#define OP_CALOAD           0x34
#define OP_SALOAD           0x35

/* Stores */
#define OP_ISTORE           0x36
#define OP_LSTORE           0x37
#define OP_FSTORE           0x38
#define OP_DSTORE           0x39
#define OP_ASTORE           0x3A
#define OP_ISTORE_0         0x3B
#define OP_ISTORE_1         0x3C
#define OP_ISTORE_2         0x3D
#define OP_ISTORE_3         0x3E
#define OP_LSTORE_0         0x3F
#define OP_LSTORE_1         0x40
#define OP_LSTORE_2         0x41
#define OP_LSTORE_3         0x42
#define OP_FSTORE_0         0x43
#define OP_FSTORE_1         0x44
#define OP_FSTORE_2         0x45
#define OP_FSTORE_3         0x46
#define OP_DSTORE_0         0x47
#define OP_DSTORE_1         0x48
#define OP_DSTORE_2         0x49
#define OP_DSTORE_3         0x4A
#define OP_ASTORE_0         0x4B
#define OP_ASTORE_1         0x4C
#define OP_ASTORE_2         0x4D
#define OP_ASTORE_3         0x4E
#define OP_IASTORE          0x4F
#define OP_LASTORE          0x50
#define OP_FASTORE          0x51
#define OP_DASTORE          0x52
#define OP_AASTORE          0x53
#define OP_BASTORE          0x54
#define OP_CASTORE          0x55
#define OP_SASTORE          0x56

/* Stack */
#define OP_POP              0x57
#define OP_POP2             0x58
#define OP_DUP              0x59
#define OP_DUP_X1           0x5A
#define OP_DUP_X2           0x5B
#define OP_DUP2             0x5C
#define OP_DUP2_X1          0x5D
#define OP_DUP2_X2          0x5E
#define OP_SWAP             0x5F

/* Math (not used for Python - use invokedynamic) */
#define OP_IADD             0x60
#define OP_LADD             0x61
#define OP_FADD             0x62
#define OP_DADD             0x63
#define OP_ISUB             0x64
#define OP_LSUB             0x65
#define OP_FSUB             0x66
#define OP_DSUB             0x67
#define OP_IMUL             0x68
#define OP_LMUL             0x69
#define OP_FMUL             0x6A
#define OP_DMUL             0x6B
#define OP_IDIV             0x6C
#define OP_LDIV             0x6D
#define OP_FDIV             0x6E
#define OP_DDIV             0x6F
#define OP_IREM             0x70
#define OP_LREM             0x71
#define OP_FREM             0x72
#define OP_DREM             0x73
#define OP_INEG             0x74
#define OP_LNEG             0x75
#define OP_FNEG             0x76
#define OP_DNEG             0x77
#define OP_ISHL             0x78
#define OP_LSHL             0x79
#define OP_ISHR             0x7A
#define OP_LSHR             0x7B
#define OP_IUSHR            0x7C
#define OP_LUSHR            0x7D
#define OP_IAND             0x7E
#define OP_LAND             0x7F
#define OP_IOR              0x80
#define OP_LOR              0x81
#define OP_IXOR             0x82
#define OP_LXOR             0x83
#define OP_IINC             0x84

/* Conversions */
#define OP_I2L              0x85
#define OP_I2F              0x86
#define OP_I2D              0x87
#define OP_L2I              0x88
#define OP_L2F              0x89
#define OP_L2D              0x8A
#define OP_F2I              0x8B
#define OP_F2L              0x8C
#define OP_F2D              0x8D
#define OP_D2I              0x8E
#define OP_D2L              0x8F
#define OP_D2F              0x90
#define OP_I2B              0x91
#define OP_I2C              0x92
#define OP_I2S              0x93

/* Comparisons */
#define OP_LCMP             0x94
#define OP_FCMPL            0x95
#define OP_FCMPG            0x96
#define OP_DCMPL            0x97
#define OP_DCMPG            0x98
#define OP_IFEQ             0x99
#define OP_IFNE             0x9A
#define OP_IFLT             0x9B
#define OP_IFGE             0x9C
#define OP_IFGT             0x9D
#define OP_IFLE             0x9E
#define OP_IF_ICMPEQ        0x9F
#define OP_IF_ICMPNE        0xA0
#define OP_IF_ICMPLT        0xA1
#define OP_IF_ICMPGE        0xA2
#define OP_IF_ICMPGT        0xA3
#define OP_IF_ICMPLE        0xA4
#define OP_IF_ACMPEQ        0xA5
#define OP_IF_ACMPNE        0xA6

/* Control */
#define OP_GOTO             0xA7
#define OP_JSR              0xA8
#define OP_RET              0xA9
#define OP_TABLESWITCH      0xAA
#define OP_LOOKUPSWITCH     0xAB
#define OP_IRETURN          0xAC
#define OP_LRETURN          0xAD
#define OP_FRETURN          0xAE
#define OP_DRETURN          0xAF
#define OP_ARETURN          0xB0
#define OP_RETURN           0xB1

/* References */
#define OP_GETSTATIC        0xB2
#define OP_PUTSTATIC        0xB3
#define OP_GETFIELD         0xB4
#define OP_PUTFIELD         0xB5
#define OP_INVOKEVIRTUAL    0xB6
#define OP_INVOKESPECIAL    0xB7
#define OP_INVOKESTATIC     0xB8
#define OP_INVOKEINTERFACE  0xB9
#define OP_INVOKEDYNAMIC    0xBA
#define OP_NEW              0xBB
#define OP_NEWARRAY         0xBC
#define OP_ANEWARRAY        0xBD
#define OP_ARRAYLENGTH      0xBE
#define OP_ATHROW           0xBF
#define OP_CHECKCAST        0xC0
#define OP_INSTANCEOF       0xC1
#define OP_MONITORENTER     0xC2
#define OP_MONITOREXIT      0xC3

/* Extended */
#define OP_WIDE             0xC4
#define OP_MULTIANEWARRAY   0xC5
#define OP_IFNULL           0xC6
#define OP_IFNONNULL        0xC7
#define OP_GOTO_W           0xC8
#define OP_JSR_W            0xC9

/* Array types for newarray */
#define T_BOOLEAN           4
#define T_CHAR              5
#define T_FLOAT             6
#define T_DOUBLE            7
#define T_BYTE              8
#define T_SHORT             9
#define T_INT               10
#define T_LONG              11

/* ========================================================================
 * Code generation context
 * ======================================================================== */

/**
 * Label for forward/backward jumps.
 */
typedef struct label
{
    int32_t offset;                 /* Bytecode offset, or -1 if unresolved */
    slist_t *references;            /* List of offsets that reference this label */
} label_t;

/**
 * Local variable slot allocation.
 */
typedef struct local_var
{
    char *name;
    int slot;
    int start_pc;                   /* Scope start */
    int end_pc;                     /* Scope end (or -1 if still open) */
} local_var_t;

/**
 * Code generation context for a single method/function.
 */
typedef struct codegen_ctx codegen_ctx_t;

struct codegen_ctx
{
    class_writer_t *cw;             /* Class being generated */
    method_info_t *method;          /* Current method */
    attribute_t *code_attr;         /* Code attribute being built */
    bytebuf_t *code;                /* Bytecode buffer */

    /* invokedynamic cache for this class */
    indy_cache_t *indy_cache;

    /* Local variable management */
    hashtable_t *locals;            /* name -> local_var_t* */
    int next_local;                 /* Next local slot */
    int max_locals;                 /* Maximum locals seen */

    /* Closure support */
    codegen_ctx_t *parent_ctx;      /* Enclosing function context (for closures) */
    slist_t *captured_vars;         /* List of captured variable names (char*) */
    int closure_slot;               /* Local slot for closure array (-1 if none) */

    /* Class context (when compiling methods inside a class) */
    const char *current_class_name; /* Non-null when inside a class definition */

    /* Global/nonlocal declarations */
    slist_t *global_names;          /* Names declared global */
    slist_t *nonlocal_names;        /* Names declared nonlocal */
    bool is_module_level;           /* True when generating module-level code */

    /* Stack tracking */
    int stack_depth;                /* Current stack depth */
    int max_stack;                  /* Maximum stack depth seen */

    /* Label management */
    slist_t *labels;                /* All labels for cleanup */

    /* Loop/exception handling context */
    slist_t *loop_stack;            /* Stack of loop contexts for break/continue */
    slist_t *try_stack;             /* Stack of try blocks */

    /* Source mapping */
    int current_line;               /* Current source line */
    source_file_t *source;

    /* Scope reference for symbol lookup */
    scope_t *scope;

    /* StackMapTable for verification (Java 7+) */
    stack_map_table_t *stackmap;

    /* Error handling */
    char *error_msg;
};

/**
 * Loop context for break/continue.
 */
typedef struct loop_ctx
{
    label_t *break_label;           /* Label for break statements */
    label_t *continue_label;        /* Label for continue statements */
} loop_ctx_t;

/* ========================================================================
 * Code generator API
 * ======================================================================== */

/**
 * Generate bytecode for a Python module.
 *
 * @param ast      Analyzed AST (module node)
 * @param analyzer Analyzer with scope/symbol information
 * @param source   Source file
 * @param opts     Compiler options
 * @return         0 on success, non-zero on error
 */
int codegen_module(ast_node_t *ast, analyzer_t *analyzer,
                    source_file_t *source, compiler_options_t *opts);

/* ========================================================================
 * Internal code generation functions
 * ======================================================================== */

/**
 * Create a new code generation context.
 */
codegen_ctx_t *codegen_ctx_new(class_writer_t *cw, method_info_t *method,
                                indy_cache_t *cache, scope_t *scope,
                                source_file_t *source);

/**
 * Free a code generation context.
 */
void codegen_ctx_free(codegen_ctx_t *ctx);

/**
 * Allocate a local variable slot.
 */
int codegen_alloc_local(codegen_ctx_t *ctx, const char *name);

/**
 * Get the slot for a local variable.
 * Returns -1 if not found.
 */
int codegen_get_local(codegen_ctx_t *ctx, const char *name);

/**
 * Create a new label.
 */
label_t *codegen_new_label(codegen_ctx_t *ctx);

/**
 * Mark a label at the current position.
 */
void codegen_mark_label(codegen_ctx_t *ctx, label_t *label);

/**
 * Emit a jump to a label (resolves forward references later).
 */
void codegen_emit_jump(codegen_ctx_t *ctx, uint8_t opcode, label_t *label);

/**
 * Resolve all forward references to labels.
 */
void codegen_resolve_labels(codegen_ctx_t *ctx);

/* ========================================================================
 * Bytecode emission helpers
 * ======================================================================== */

void emit_u8(codegen_ctx_t *ctx, uint8_t val);
void emit_u16(codegen_ctx_t *ctx, uint16_t val);
void emit_i16(codegen_ctx_t *ctx, int16_t val);

/* Push constant values */
void emit_iconst(codegen_ctx_t *ctx, int32_t value);
void emit_lconst(codegen_ctx_t *ctx, int64_t value);
void emit_fconst(codegen_ctx_t *ctx, float value);
void emit_dconst(codegen_ctx_t *ctx, double value);
void emit_aconst_null(codegen_ctx_t *ctx);
void emit_ldc_string(codegen_ctx_t *ctx, const char *str);

/* Local variable access */
void emit_aload(codegen_ctx_t *ctx, int slot);
void emit_astore(codegen_ctx_t *ctx, int slot);

/* Method invocation */
void emit_invokestatic(codegen_ctx_t *ctx, const char *class_name,
                        const char *method_name, const char *descriptor);
void emit_invokevirtual(codegen_ctx_t *ctx, const char *class_name,
                         const char *method_name, const char *descriptor);
void emit_invokespecial(codegen_ctx_t *ctx, const char *class_name,
                         const char *method_name, const char *descriptor);
void emit_invokeinterface(codegen_ctx_t *ctx, const char *class_name,
                           const char *method_name, const char *descriptor,
                           uint8_t count);
void emit_invokedynamic(codegen_ctx_t *ctx, uint16_t indy_index);

/* Field access */
void emit_getstatic(codegen_ctx_t *ctx, const char *class_name,
                     const char *field_name, const char *descriptor);
void emit_putstatic(codegen_ctx_t *ctx, const char *class_name,
                     const char *field_name, const char *descriptor);
void emit_getfield(codegen_ctx_t *ctx, const char *class_name,
                    const char *field_name, const char *descriptor);
void emit_putfield(codegen_ctx_t *ctx, const char *class_name,
                    const char *field_name, const char *descriptor);

/* Object creation */
void emit_new(codegen_ctx_t *ctx, const char *class_name);
void emit_newarray(codegen_ctx_t *ctx, uint8_t atype);
void emit_anewarray(codegen_ctx_t *ctx, const char *class_name);

/* Type checking */
void emit_checkcast(codegen_ctx_t *ctx, const char *class_name);
void emit_instanceof(codegen_ctx_t *ctx, const char *class_name);

/* Stack adjustment tracking */
void stack_push(codegen_ctx_t *ctx, int count);
void stack_pop(codegen_ctx_t *ctx, int count);

#endif /* CODEGEN_H */

