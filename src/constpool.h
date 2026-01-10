/*
 * constpool.h
 * JVM Constant pool management for loretta
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

#ifndef CONSTPOOL_H
#define CONSTPOOL_H

#include <stdint.h>
#include "util.h"

/*
 * Constant Pool Tag Types (JVM class file format)
 */
typedef enum const_tag
{
    CONST_UTF8               = 1,
    CONST_INTEGER            = 3,
    CONST_FLOAT              = 4,
    CONST_LONG               = 5,
    CONST_DOUBLE             = 6,
    CONST_CLASS              = 7,
    CONST_STRING             = 8,
    CONST_FIELDREF           = 9,
    CONST_METHODREF          = 10,
    CONST_INTERFACE_METHODREF = 11,
    CONST_NAME_AND_TYPE      = 12,
    CONST_METHOD_HANDLE      = 15,
    CONST_METHOD_TYPE        = 16,
    CONST_DYNAMIC            = 17,
    CONST_INVOKE_DYNAMIC     = 18,
    CONST_MODULE             = 19,
    CONST_PACKAGE            = 20
} const_tag_t;

/*
 * Constant Pool Entry
 */
typedef struct const_entry
{
    const_tag_t tag;
    union {
        char *utf8;                 /* CONST_UTF8 */
        int32_t integer;            /* CONST_INTEGER */
        float float_val;            /* CONST_FLOAT */
        int64_t long_val;           /* CONST_LONG */
        double double_val;          /* CONST_DOUBLE */
        uint16_t class_index;       /* CONST_CLASS - index to UTF8 */
        uint16_t string_index;      /* CONST_STRING - index to UTF8 */
        struct {
            uint16_t class_index;
            uint16_t name_type_index;
        } ref;                      /* CONST_FIELDREF, CONST_METHODREF, CONST_INTERFACE_METHODREF */
        struct {
            uint16_t name_index;
            uint16_t descriptor_index;
        } name_type;                /* CONST_NAME_AND_TYPE */
        struct {
            uint8_t reference_kind;
            uint16_t reference_index;
        } method_handle;            /* CONST_METHOD_HANDLE */
        uint16_t method_type_index; /* CONST_METHOD_TYPE - index to UTF8 descriptor */
        struct {
            uint16_t bootstrap_method_attr_index;
            uint16_t name_and_type_index;
        } dynamic;                  /* CONST_DYNAMIC, CONST_INVOKE_DYNAMIC */
    } data;
} const_entry_t;

/*
 * Constant Pool Builder
 */
typedef struct const_pool
{
    const_entry_t *entries;
    uint16_t count;                 /* Next index to use (1-based, 0 is unused) */
    uint16_t capacity;
    hashtable_t *utf8_cache;        /* UTF8 deduplication cache */
} const_pool_t;

/* ========================================================================
 * Constant Pool Management
 * ======================================================================== */

/**
 * Create a new constant pool.
 * Index 0 is reserved (unused per JVM spec).
 */
const_pool_t *const_pool_new(void);

/**
 * Free a constant pool and all its entries.
 */
void const_pool_free(const_pool_t *cp);

/**
 * Add a UTF8 string entry (deduplicated).
 */
uint16_t cp_add_utf8(const_pool_t *cp, const char *str);

/**
 * Add an integer constant.
 */
uint16_t cp_add_integer(const_pool_t *cp, int32_t value);

/**
 * Add a float constant.
 */
uint16_t cp_add_float(const_pool_t *cp, float value);

/**
 * Add a long constant (occupies 2 slots).
 */
uint16_t cp_add_long(const_pool_t *cp, int64_t value);

/**
 * Add a double constant (occupies 2 slots).
 */
uint16_t cp_add_double(const_pool_t *cp, double value);

/**
 * Add a class reference.
 * @param name Internal class name (e.g., "java/lang/Object")
 */
uint16_t cp_add_class(const_pool_t *cp, const char *name);

/**
 * Add a string constant.
 */
uint16_t cp_add_string(const_pool_t *cp, const char *str);

/**
 * Add a NameAndType entry.
 */
uint16_t cp_add_name_and_type(const_pool_t *cp, const char *name,
                               const char *descriptor);

/**
 * Add a field reference.
 */
uint16_t cp_add_fieldref(const_pool_t *cp, const char *class_name,
                          const char *name, const char *descriptor);

/**
 * Add a method reference.
 */
uint16_t cp_add_methodref(const_pool_t *cp, const char *class_name,
                           const char *name, const char *descriptor);

/**
 * Add an interface method reference.
 */
uint16_t cp_add_interface_methodref(const_pool_t *cp, const char *class_name,
                                     const char *name, const char *descriptor);

/**
 * Add a MethodHandle entry.
 * @param reference_kind One of REF_* constants (1-9)
 * @param reference_index CP index of the referenced field/method
 */
uint16_t cp_add_method_handle(const_pool_t *cp, uint8_t reference_kind,
                               uint16_t reference_index);

/**
 * Add a MethodType entry.
 * @param descriptor Method type descriptor (e.g., "(II)I")
 */
uint16_t cp_add_method_type(const_pool_t *cp, const char *descriptor);

/**
 * Add an InvokeDynamic entry.
 * @param bootstrap_idx Index into BootstrapMethods attribute
 * @param nat_idx CP index of CONSTANT_NameAndType
 */
uint16_t cp_add_invoke_dynamic(const_pool_t *cp, uint16_t bootstrap_idx,
                                uint16_t nat_idx);

/* ========================================================================
 * Method Handle Reference Kinds (JVM Spec §5.4.3.5)
 * ======================================================================== */

#define REF_getField         1
#define REF_getStatic        2
#define REF_putField         3
#define REF_putStatic        4
#define REF_invokeVirtual    5
#define REF_invokeStatic     6
#define REF_invokeSpecial    7
#define REF_newInvokeSpecial 8
#define REF_invokeInterface  9

#endif /* CONSTPOOL_H */

