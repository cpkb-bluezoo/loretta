/*
 * classwriter.h
 * JVM class file writer for loretta
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

#ifndef CLASSWRITER_H
#define CLASSWRITER_H

#include <stdint.h>
#include "util.h"
#include "constpool.h"

/* ========================================================================
 * JVM Class File Format Constants
 * ======================================================================== */

#define CLASS_MAGIC 0xCAFEBABE

/* Class file version for Java 11 (minimum for invokedynamic features we need) */
#define CLASS_MAJOR_VERSION 55
#define CLASS_MINOR_VERSION 0

/* Access flags */
#define ACC_PUBLIC       0x0001
#define ACC_PRIVATE      0x0002
#define ACC_PROTECTED    0x0004
#define ACC_STATIC       0x0008
#define ACC_FINAL        0x0010
#define ACC_SUPER        0x0020     /* Treat superclass methods specially */
#define ACC_SYNCHRONIZED 0x0020     /* Method: synchronized */
#define ACC_VOLATILE     0x0040     /* Field: volatile */
#define ACC_BRIDGE       0x0040     /* Method: bridge */
#define ACC_TRANSIENT    0x0080     /* Field: transient */
#define ACC_VARARGS      0x0080     /* Method: varargs */
#define ACC_NATIVE       0x0100     /* Method: native */
#define ACC_INTERFACE    0x0200     /* Class: interface */
#define ACC_ABSTRACT     0x0400
#define ACC_STRICT       0x0800     /* Method: strictfp */
#define ACC_SYNTHETIC    0x1000     /* Not in source code */
#define ACC_ANNOTATION   0x2000     /* Class: annotation type */
#define ACC_ENUM         0x4000

/* ========================================================================
 * Field info
 * ======================================================================== */

typedef struct field_info
{
    uint16_t access_flags;
    uint16_t name_index;            /* CP index */
    uint16_t descriptor_index;      /* CP index */
    slist_t *attributes;            /* List of attribute_t* */
} field_info_t;

/* ========================================================================
 * Method info
 * ======================================================================== */

typedef struct method_info
{
    uint16_t access_flags;
    uint16_t name_index;            /* CP index */
    uint16_t descriptor_index;      /* CP index */
    slist_t *attributes;            /* List of attribute_t* */
} method_info_t;

/* ========================================================================
 * Attribute types
 * ======================================================================== */

typedef enum attribute_type
{
    ATTR_CODE,
    ATTR_LINE_NUMBER_TABLE,
    ATTR_LOCAL_VARIABLE_TABLE,
    ATTR_STACK_MAP_TABLE,
    ATTR_BOOTSTRAP_METHODS,
    ATTR_SOURCE_FILE,
    ATTR_RAW                        /* Raw bytes, for generic attributes */
} attribute_type_t;

/* Exception table entry for Code attribute */
typedef struct exception_entry
{
    uint16_t start_pc;
    uint16_t end_pc;
    uint16_t handler_pc;
    uint16_t catch_type;            /* CP index, or 0 for any */
} exception_entry_t;

/* Line number table entry */
typedef struct line_number_entry
{
    uint16_t start_pc;
    uint16_t line_number;
} line_number_entry_t;

/* Bootstrap method entry */
typedef struct bootstrap_method
{
    uint16_t method_handle_index;   /* CP index of MethodHandle */
    uint16_t *arguments;            /* CP indices of bootstrap args */
    uint16_t num_arguments;
} bootstrap_method_t;

/* Attribute structure */
typedef struct attribute
{
    attribute_type_t type;
    uint16_t name_index;            /* CP index of attribute name */
    union {
        /* ATTR_CODE */
        struct {
            uint16_t max_stack;
            uint16_t max_locals;
            bytebuf_t *code;        /* Bytecode */
            slist_t *exception_table;   /* List of exception_entry_t* */
            slist_t *attributes;        /* Nested attributes */
        } code;

        /* ATTR_LINE_NUMBER_TABLE */
        struct {
            slist_t *entries;       /* List of line_number_entry_t* */
        } line_numbers;

        /* ATTR_BOOTSTRAP_METHODS */
        struct {
            bootstrap_method_t *methods;
            uint16_t count;
            uint16_t capacity;
        } bootstrap;

        /* ATTR_SOURCE_FILE */
        struct {
            uint16_t sourcefile_index;  /* CP index */
        } source_file;

        /* ATTR_RAW */
        struct {
            uint8_t *data;
            uint32_t length;
        } raw;
    } data;
} attribute_t;

/* ========================================================================
 * Class writer
 * ======================================================================== */

typedef struct class_writer
{
    const_pool_t *cp;               /* Constant pool */

    uint16_t access_flags;
    uint16_t this_class;            /* CP index */
    uint16_t super_class;           /* CP index (0 for Object) */

    uint16_t *interfaces;           /* Array of CP indices */
    uint16_t interfaces_count;
    uint16_t interfaces_capacity;

    slist_t *fields;                /* List of field_info_t* */
    slist_t *methods;               /* List of method_info_t* */
    slist_t *attributes;            /* List of attribute_t* (class attributes) */

    /* Bootstrap methods tracking */
    attribute_t *bootstrap_attr;    /* Lazily created */
} class_writer_t;

/* ========================================================================
 * Class writer API
 * ======================================================================== */

/**
 * Create a new class writer.
 * @param class_name Internal class name (e.g., "com/example/MyClass")
 * @param super_name Internal superclass name, or NULL for java/lang/Object
 * @param access_flags Class access flags
 */
class_writer_t *class_writer_new(const char *class_name, const char *super_name,
                                  uint16_t access_flags);

/**
 * Free a class writer and all associated data.
 */
void class_writer_free(class_writer_t *cw);

/**
 * Add an implemented interface.
 */
void class_writer_add_interface(class_writer_t *cw, const char *interface_name);

/**
 * Add a field to the class.
 */
field_info_t *class_writer_add_field(class_writer_t *cw, const char *name,
                                      const char *descriptor, uint16_t access_flags);

/**
 * Add a method to the class.
 */
method_info_t *class_writer_add_method(class_writer_t *cw, const char *name,
                                        const char *descriptor, uint16_t access_flags);

/**
 * Get the class's constant pool.
 */
const_pool_t *class_writer_get_cp(class_writer_t *cw);

/**
 * Add a bootstrap method and return its index.
 */
uint16_t class_writer_add_bootstrap_method(class_writer_t *cw,
                                            uint16_t method_handle_index,
                                            uint16_t *arguments,
                                            uint16_t num_arguments);

/**
 * Set the SourceFile attribute.
 */
void class_writer_set_source_file(class_writer_t *cw, const char *filename);

/**
 * Write the class file to a byte buffer.
 */
bytebuf_t *class_writer_to_bytes(class_writer_t *cw);

/**
 * Write the class file to disk.
 * Creates parent directories as needed.
 */
bool class_writer_write_file(class_writer_t *cw, const char *output_dir);

/* ========================================================================
 * Code attribute builder
 * ======================================================================== */

/**
 * Create a Code attribute for a method.
 */
attribute_t *code_attr_new(const_pool_t *cp);

/**
 * Free a Code attribute.
 */
void code_attr_free(attribute_t *attr);

/**
 * Add an exception table entry.
 */
void code_attr_add_exception(attribute_t *attr, uint16_t start_pc,
                              uint16_t end_pc, uint16_t handler_pc,
                              uint16_t catch_type);

/**
 * Add a line number entry.
 */
void code_attr_add_line_number(attribute_t *attr, const_pool_t *cp,
                                uint16_t pc, uint16_t line);

/**
 * Set the StackMapTable attribute on a Code attribute.
 * Takes ownership of the data buffer.
 */
void code_attr_set_stack_map_table(attribute_t *attr, const_pool_t *cp,
                                    uint8_t *data, uint32_t length);

#endif /* CLASSWRITER_H */

