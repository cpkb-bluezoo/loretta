/*
 * constpool.c
 * JVM Constant pool management
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

#include "constpool.h"

#define INITIAL_CAPACITY 64

const_pool_t *const_pool_new(void)
{
    const_pool_t *cp = malloc(sizeof(const_pool_t));
    if (!cp) {
        return NULL;
    }

    cp->capacity = INITIAL_CAPACITY;
    cp->entries = calloc(cp->capacity, sizeof(const_entry_t));
    if (!cp->entries) {
        free(cp);
        return NULL;
    }

    cp->count = 1;  /* Index 0 is unused per JVM spec */
    cp->utf8_cache = hashtable_new();

    return cp;
}

void const_pool_free(const_pool_t *cp)
{
    if (!cp) {
        return;
    }

    /* Free UTF8 strings */
    for (uint16_t i = 1; i < cp->count; i++) {
        if (cp->entries[i].tag == CONST_UTF8) {
            free(cp->entries[i].data.utf8);
        }
    }

    free(cp->entries);
    hashtable_free(cp->utf8_cache);
    free(cp);
}

static bool ensure_capacity(const_pool_t *cp, uint16_t needed)
{
    if (cp->count + needed <= cp->capacity) {
        return true;
    }

    uint16_t new_capacity = cp->capacity * 2;
    while (new_capacity < cp->count + needed) {
        new_capacity *= 2;
    }

    const_entry_t *new_entries = realloc(cp->entries,
                                         new_capacity * sizeof(const_entry_t));
    if (!new_entries) {
        return false;
    }

    memset(new_entries + cp->capacity, 0,
           (new_capacity - cp->capacity) * sizeof(const_entry_t));

    cp->entries = new_entries;
    cp->capacity = new_capacity;
    return true;
}

uint16_t cp_add_utf8(const_pool_t *cp, const char *str)
{
    if (!cp || !str) {
        return 0;
    }

    /* Check cache for existing entry */
    void *cached = hashtable_lookup(cp->utf8_cache, str);
    if (cached) {
        return (uint16_t)(uintptr_t)cached;
    }

    if (!ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_UTF8;
    cp->entries[index].data.utf8 = str_dup(str);

    /* Cache the index */
    hashtable_insert(cp->utf8_cache, str, (void *)(uintptr_t)index);

    return index;
}

uint16_t cp_add_integer(const_pool_t *cp, int32_t value)
{
    if (!cp || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_INTEGER;
    cp->entries[index].data.integer = value;
    return index;
}

uint16_t cp_add_float(const_pool_t *cp, float value)
{
    if (!cp || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_FLOAT;
    cp->entries[index].data.float_val = value;
    return index;
}

uint16_t cp_add_long(const_pool_t *cp, int64_t value)
{
    if (!cp || !ensure_capacity(cp, 2)) {
        return 0;
    }

    uint16_t index = cp->count;
    cp->entries[index].tag = CONST_LONG;
    cp->entries[index].data.long_val = value;
    cp->count += 2;  /* Long takes 2 slots */
    return index;
}

uint16_t cp_add_double(const_pool_t *cp, double value)
{
    if (!cp || !ensure_capacity(cp, 2)) {
        return 0;
    }

    uint16_t index = cp->count;
    cp->entries[index].tag = CONST_DOUBLE;
    cp->entries[index].data.double_val = value;
    cp->count += 2;  /* Double takes 2 slots */
    return index;
}

uint16_t cp_add_class(const_pool_t *cp, const char *name)
{
    if (!cp || !name) {
        return 0;
    }

    uint16_t name_index = cp_add_utf8(cp, name);
    if (!name_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_CLASS;
    cp->entries[index].data.class_index = name_index;
    return index;
}

uint16_t cp_add_string(const_pool_t *cp, const char *str)
{
    if (!cp || !str) {
        return 0;
    }

    uint16_t string_index = cp_add_utf8(cp, str);
    if (!string_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_STRING;
    cp->entries[index].data.string_index = string_index;
    return index;
}

uint16_t cp_add_name_and_type(const_pool_t *cp, const char *name,
                               const char *descriptor)
{
    if (!cp || !name || !descriptor) {
        return 0;
    }

    uint16_t name_index = cp_add_utf8(cp, name);
    uint16_t desc_index = cp_add_utf8(cp, descriptor);
    if (!name_index || !desc_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_NAME_AND_TYPE;
    cp->entries[index].data.name_type.name_index = name_index;
    cp->entries[index].data.name_type.descriptor_index = desc_index;
    return index;
}

uint16_t cp_add_fieldref(const_pool_t *cp, const char *class_name,
                          const char *name, const char *descriptor)
{
    if (!cp || !class_name || !name || !descriptor) {
        return 0;
    }

    uint16_t class_index = cp_add_class(cp, class_name);
    uint16_t nat_index = cp_add_name_and_type(cp, name, descriptor);
    if (!class_index || !nat_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_FIELDREF;
    cp->entries[index].data.ref.class_index = class_index;
    cp->entries[index].data.ref.name_type_index = nat_index;
    return index;
}

uint16_t cp_add_methodref(const_pool_t *cp, const char *class_name,
                           const char *name, const char *descriptor)
{
    if (!cp || !class_name || !name || !descriptor) {
        return 0;
    }

    uint16_t class_index = cp_add_class(cp, class_name);
    uint16_t nat_index = cp_add_name_and_type(cp, name, descriptor);
    if (!class_index || !nat_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_METHODREF;
    cp->entries[index].data.ref.class_index = class_index;
    cp->entries[index].data.ref.name_type_index = nat_index;
    return index;
}

uint16_t cp_add_interface_methodref(const_pool_t *cp, const char *class_name,
                                     const char *name, const char *descriptor)
{
    if (!cp || !class_name || !name || !descriptor) {
        return 0;
    }

    uint16_t class_index = cp_add_class(cp, class_name);
    uint16_t nat_index = cp_add_name_and_type(cp, name, descriptor);
    if (!class_index || !nat_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_INTERFACE_METHODREF;
    cp->entries[index].data.ref.class_index = class_index;
    cp->entries[index].data.ref.name_type_index = nat_index;
    return index;
}

uint16_t cp_add_method_handle(const_pool_t *cp, uint8_t reference_kind,
                               uint16_t reference_index)
{
    if (!cp || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_METHOD_HANDLE;
    cp->entries[index].data.method_handle.reference_kind = reference_kind;
    cp->entries[index].data.method_handle.reference_index = reference_index;
    return index;
}

uint16_t cp_add_method_type(const_pool_t *cp, const char *descriptor)
{
    if (!cp || !descriptor) {
        return 0;
    }

    uint16_t desc_index = cp_add_utf8(cp, descriptor);
    if (!desc_index || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_METHOD_TYPE;
    cp->entries[index].data.method_type_index = desc_index;
    return index;
}

uint16_t cp_add_invoke_dynamic(const_pool_t *cp, uint16_t bootstrap_idx,
                                uint16_t nat_idx)
{
    if (!cp || !ensure_capacity(cp, 1)) {
        return 0;
    }

    uint16_t index = cp->count++;
    cp->entries[index].tag = CONST_INVOKE_DYNAMIC;
    cp->entries[index].data.dynamic.bootstrap_method_attr_index = bootstrap_idx;
    cp->entries[index].data.dynamic.name_and_type_index = nat_idx;
    return index;
}

