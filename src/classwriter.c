/*
 * classwriter.c
 * JVM class file writer
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

#include "classwriter.h"

/* ========================================================================
 * Attribute management
 * ======================================================================== */

static void attribute_free(attribute_t *attr)
{
    if (!attr) {
        return;
    }

    switch (attr->type) {
        case ATTR_CODE:
            bytebuf_free(attr->data.code.code);
            slist_free_full(attr->data.code.exception_table, free);
            slist_free_full(attr->data.code.attributes, (void (*)(void *))attribute_free);
            break;

        case ATTR_LINE_NUMBER_TABLE:
            slist_free_full(attr->data.line_numbers.entries, free);
            break;

        case ATTR_BOOTSTRAP_METHODS:
            for (uint16_t i = 0; i < attr->data.bootstrap.count; i++) {
                free(attr->data.bootstrap.methods[i].arguments);
            }
            free(attr->data.bootstrap.methods);
            break;

        case ATTR_RAW:
            free(attr->data.raw.data);
            break;

        default:
            break;
    }

    free(attr);
}

attribute_t *code_attr_new(const_pool_t *cp)
{
    attribute_t *attr = calloc(1, sizeof(attribute_t));
    if (!attr) {
        return NULL;
    }

    attr->type = ATTR_CODE;
    attr->name_index = cp_add_utf8(cp, "Code");
    attr->data.code.code = bytebuf_new();
    attr->data.code.exception_table = NULL;
    attr->data.code.attributes = NULL;
    attr->data.code.max_stack = 0;
    attr->data.code.max_locals = 0;

    return attr;
}

void code_attr_free(attribute_t *attr)
{
    attribute_free(attr);
}

void code_attr_add_exception(attribute_t *attr, uint16_t start_pc,
                              uint16_t end_pc, uint16_t handler_pc,
                              uint16_t catch_type)
{
    if (!attr || attr->type != ATTR_CODE) {
        return;
    }

    exception_entry_t *entry = malloc(sizeof(exception_entry_t));
    if (!entry) {
        return;
    }

    entry->start_pc = start_pc;
    entry->end_pc = end_pc;
    entry->handler_pc = handler_pc;
    entry->catch_type = catch_type;

    attr->data.code.exception_table = slist_append(attr->data.code.exception_table, entry);
}

void code_attr_add_line_number(attribute_t *attr, const_pool_t *cp,
                                uint16_t pc, uint16_t line)
{
    if (!attr || attr->type != ATTR_CODE) {
        return;
    }

    /* Find or create LineNumberTable attribute */
    attribute_t *lnt = NULL;
    for (slist_t *s = attr->data.code.attributes; s; s = s->next) {
        attribute_t *a = s->data;
        if (a->type == ATTR_LINE_NUMBER_TABLE) {
            lnt = a;
            break;
        }
    }

    if (!lnt) {
        lnt = calloc(1, sizeof(attribute_t));
        if (!lnt) {
            return;
        }
        lnt->type = ATTR_LINE_NUMBER_TABLE;
        lnt->name_index = cp_add_utf8(cp, "LineNumberTable");
        lnt->data.line_numbers.entries = NULL;
        attr->data.code.attributes = slist_append(attr->data.code.attributes, lnt);
    }

    line_number_entry_t *entry = malloc(sizeof(line_number_entry_t));
    if (!entry) {
        return;
    }

    entry->start_pc = pc;
    entry->line_number = line;
    lnt->data.line_numbers.entries = slist_append(lnt->data.line_numbers.entries, entry);
}

/* ========================================================================
 * Field/method management
 * ======================================================================== */

static void field_info_free(field_info_t *field)
{
    if (field) {
        slist_free_full(field->attributes, (void (*)(void *))attribute_free);
        free(field);
    }
}

static void method_info_free(method_info_t *method)
{
    if (method) {
        slist_free_full(method->attributes, (void (*)(void *))attribute_free);
        free(method);
    }
}

/* ========================================================================
 * Class writer implementation
 * ======================================================================== */

class_writer_t *class_writer_new(const char *class_name, const char *super_name,
                                  uint16_t access_flags)
{
    class_writer_t *cw = calloc(1, sizeof(class_writer_t));
    if (!cw) {
        return NULL;
    }

    cw->cp = const_pool_new();
    if (!cw->cp) {
        free(cw);
        return NULL;
    }

    cw->access_flags = access_flags | ACC_SUPER;
    cw->this_class = cp_add_class(cw->cp, class_name);
    cw->super_class = cp_add_class(cw->cp, super_name ? super_name : "java/lang/Object");

    cw->interfaces = NULL;
    cw->interfaces_count = 0;
    cw->interfaces_capacity = 0;

    cw->fields = NULL;
    cw->methods = NULL;
    cw->attributes = NULL;
    cw->bootstrap_attr = NULL;

    return cw;
}

void class_writer_free(class_writer_t *cw)
{
    if (!cw) {
        return;
    }

    const_pool_free(cw->cp);
    free(cw->interfaces);
    slist_free_full(cw->fields, (void (*)(void *))field_info_free);
    slist_free_full(cw->methods, (void (*)(void *))method_info_free);
    slist_free_full(cw->attributes, (void (*)(void *))attribute_free);
    free(cw);
}

void class_writer_add_interface(class_writer_t *cw, const char *interface_name)
{
    if (!cw || !interface_name) {
        return;
    }

    if (cw->interfaces_count >= cw->interfaces_capacity) {
        uint16_t new_cap = cw->interfaces_capacity == 0 ? 4 : cw->interfaces_capacity * 2;
        uint16_t *new_ifs = realloc(cw->interfaces, new_cap * sizeof(uint16_t));
        if (!new_ifs) {
            return;
        }
        cw->interfaces = new_ifs;
        cw->interfaces_capacity = new_cap;
    }

    cw->interfaces[cw->interfaces_count++] = cp_add_class(cw->cp, interface_name);
}

field_info_t *class_writer_add_field(class_writer_t *cw, const char *name,
                                      const char *descriptor, uint16_t access_flags)
{
    if (!cw || !name || !descriptor) {
        return NULL;
    }

    field_info_t *field = calloc(1, sizeof(field_info_t));
    if (!field) {
        return NULL;
    }

    field->access_flags = access_flags;
    field->name_index = cp_add_utf8(cw->cp, name);
    field->descriptor_index = cp_add_utf8(cw->cp, descriptor);
    field->attributes = NULL;

    cw->fields = slist_append(cw->fields, field);
    return field;
}

method_info_t *class_writer_add_method(class_writer_t *cw, const char *name,
                                        const char *descriptor, uint16_t access_flags)
{
    if (!cw || !name || !descriptor) {
        return NULL;
    }

    method_info_t *method = calloc(1, sizeof(method_info_t));
    if (!method) {
        return NULL;
    }

    method->access_flags = access_flags;
    method->name_index = cp_add_utf8(cw->cp, name);
    method->descriptor_index = cp_add_utf8(cw->cp, descriptor);
    method->attributes = NULL;

    cw->methods = slist_append(cw->methods, method);
    return method;
}

const_pool_t *class_writer_get_cp(class_writer_t *cw)
{
    return cw ? cw->cp : NULL;
}

uint16_t class_writer_add_bootstrap_method(class_writer_t *cw,
                                            uint16_t method_handle_index,
                                            uint16_t *arguments,
                                            uint16_t num_arguments)
{
    if (!cw) {
        return 0;
    }

    /* Create bootstrap methods attribute if needed */
    if (!cw->bootstrap_attr) {
        cw->bootstrap_attr = calloc(1, sizeof(attribute_t));
        if (!cw->bootstrap_attr) {
            return 0;
        }
        cw->bootstrap_attr->type = ATTR_BOOTSTRAP_METHODS;
        cw->bootstrap_attr->name_index = cp_add_utf8(cw->cp, "BootstrapMethods");
        cw->bootstrap_attr->data.bootstrap.methods = NULL;
        cw->bootstrap_attr->data.bootstrap.count = 0;
        cw->bootstrap_attr->data.bootstrap.capacity = 0;
        cw->attributes = slist_append(cw->attributes, cw->bootstrap_attr);
    }

    attribute_t *bsm = cw->bootstrap_attr;

    /* Ensure capacity */
    if (bsm->data.bootstrap.count >= bsm->data.bootstrap.capacity) {
        uint16_t new_cap = bsm->data.bootstrap.capacity == 0 ? 4 :
                          bsm->data.bootstrap.capacity * 2;
        bootstrap_method_t *new_methods = realloc(bsm->data.bootstrap.methods,
                                                   new_cap * sizeof(bootstrap_method_t));
        if (!new_methods) {
            return 0;
        }
        bsm->data.bootstrap.methods = new_methods;
        bsm->data.bootstrap.capacity = new_cap;
    }

    uint16_t index = bsm->data.bootstrap.count++;
    bootstrap_method_t *entry = &bsm->data.bootstrap.methods[index];
    entry->method_handle_index = method_handle_index;
    entry->num_arguments = num_arguments;
    entry->arguments = NULL;

    if (num_arguments > 0) {
        entry->arguments = malloc(num_arguments * sizeof(uint16_t));
        if (entry->arguments) {
            memcpy(entry->arguments, arguments, num_arguments * sizeof(uint16_t));
        }
    }

    return index;
}

void class_writer_set_source_file(class_writer_t *cw, const char *filename)
{
    if (!cw || !filename) {
        return;
    }

    attribute_t *attr = calloc(1, sizeof(attribute_t));
    if (!attr) {
        return;
    }

    attr->type = ATTR_SOURCE_FILE;
    attr->name_index = cp_add_utf8(cw->cp, "SourceFile");
    attr->data.source_file.sourcefile_index = cp_add_utf8(cw->cp, filename);

    cw->attributes = slist_append(cw->attributes, attr);
}

/* ========================================================================
 * Class file serialization
 * ======================================================================== */

static void write_cp_entry(bytebuf_t *buf, const_pool_t *cp, uint16_t index)
{
    const_entry_t *entry = &cp->entries[index];
    bytebuf_write_u8(buf, entry->tag);

    switch (entry->tag) {
        case CONST_UTF8: {
            size_t len = strlen(entry->data.utf8);
            bytebuf_write_u16(buf, len);
            bytebuf_write_bytes(buf, (uint8_t *)entry->data.utf8, len);
            break;
        }

        case CONST_INTEGER:
            bytebuf_write_u32(buf, entry->data.integer);
            break;

        case CONST_FLOAT:
            bytebuf_write_u32(buf, *(uint32_t *)&entry->data.float_val);
            break;

        case CONST_LONG:
            bytebuf_write_u32(buf, (entry->data.long_val >> 32) & 0xFFFFFFFF);
            bytebuf_write_u32(buf, entry->data.long_val & 0xFFFFFFFF);
            break;

        case CONST_DOUBLE: {
            uint64_t bits = *(uint64_t *)&entry->data.double_val;
            bytebuf_write_u32(buf, (bits >> 32) & 0xFFFFFFFF);
            bytebuf_write_u32(buf, bits & 0xFFFFFFFF);
            break;
        }

        case CONST_CLASS:
        case CONST_STRING:
        case CONST_METHOD_TYPE:
            bytebuf_write_u16(buf, entry->data.class_index);
            break;

        case CONST_FIELDREF:
        case CONST_METHODREF:
        case CONST_INTERFACE_METHODREF:
            bytebuf_write_u16(buf, entry->data.ref.class_index);
            bytebuf_write_u16(buf, entry->data.ref.name_type_index);
            break;

        case CONST_NAME_AND_TYPE:
            bytebuf_write_u16(buf, entry->data.name_type.name_index);
            bytebuf_write_u16(buf, entry->data.name_type.descriptor_index);
            break;

        case CONST_METHOD_HANDLE:
            bytebuf_write_u8(buf, entry->data.method_handle.reference_kind);
            bytebuf_write_u16(buf, entry->data.method_handle.reference_index);
            break;

        case CONST_INVOKE_DYNAMIC:
        case CONST_DYNAMIC:
            bytebuf_write_u16(buf, entry->data.dynamic.bootstrap_method_attr_index);
            bytebuf_write_u16(buf, entry->data.dynamic.name_and_type_index);
            break;

        default:
            break;
    }
}

static void write_attribute(bytebuf_t *buf, attribute_t *attr)
{
    bytebuf_write_u16(buf, attr->name_index);

    /* Calculate attribute length */
    size_t len_offset = buf->len;
    bytebuf_write_u32(buf, 0);  /* Placeholder for length */
    size_t start = buf->len;

    switch (attr->type) {
        case ATTR_CODE: {
            bytebuf_write_u16(buf, attr->data.code.max_stack);
            bytebuf_write_u16(buf, attr->data.code.max_locals);
            bytebuf_write_u32(buf, attr->data.code.code->len);
            bytebuf_write_bytes(buf, attr->data.code.code->data, attr->data.code.code->len);

            /* Exception table */
            uint16_t exception_count = slist_length(attr->data.code.exception_table);
            bytebuf_write_u16(buf, exception_count);
            for (slist_t *s = attr->data.code.exception_table; s; s = s->next) {
                exception_entry_t *e = s->data;
                bytebuf_write_u16(buf, e->start_pc);
                bytebuf_write_u16(buf, e->end_pc);
                bytebuf_write_u16(buf, e->handler_pc);
                bytebuf_write_u16(buf, e->catch_type);
            }

            /* Nested attributes */
            uint16_t attr_count = slist_length(attr->data.code.attributes);
            bytebuf_write_u16(buf, attr_count);
            for (slist_t *s = attr->data.code.attributes; s; s = s->next) {
                write_attribute(buf, s->data);
            }
            break;
        }

        case ATTR_LINE_NUMBER_TABLE: {
            uint16_t count = slist_length(attr->data.line_numbers.entries);
            bytebuf_write_u16(buf, count);
            for (slist_t *s = attr->data.line_numbers.entries; s; s = s->next) {
                line_number_entry_t *e = s->data;
                bytebuf_write_u16(buf, e->start_pc);
                bytebuf_write_u16(buf, e->line_number);
            }
            break;
        }

        case ATTR_BOOTSTRAP_METHODS: {
            bytebuf_write_u16(buf, attr->data.bootstrap.count);
            for (uint16_t i = 0; i < attr->data.bootstrap.count; i++) {
                bootstrap_method_t *bsm = &attr->data.bootstrap.methods[i];
                bytebuf_write_u16(buf, bsm->method_handle_index);
                bytebuf_write_u16(buf, bsm->num_arguments);
                for (uint16_t j = 0; j < bsm->num_arguments; j++) {
                    bytebuf_write_u16(buf, bsm->arguments[j]);
                }
            }
            break;
        }

        case ATTR_SOURCE_FILE:
            bytebuf_write_u16(buf, attr->data.source_file.sourcefile_index);
            break;

        case ATTR_RAW:
            bytebuf_write_bytes(buf, attr->data.raw.data, attr->data.raw.length);
            break;

        default:
            break;
    }

    /* Patch attribute length */
    bytebuf_patch_u32(buf, len_offset, buf->len - start);
}

bytebuf_t *class_writer_to_bytes(class_writer_t *cw)
{
    if (!cw) {
        return NULL;
    }

    bytebuf_t *buf = bytebuf_new();
    if (!buf) {
        return NULL;
    }

    /* Magic number */
    bytebuf_write_u32(buf, CLASS_MAGIC);

    /* Version */
    bytebuf_write_u16(buf, CLASS_MINOR_VERSION);
    bytebuf_write_u16(buf, CLASS_MAJOR_VERSION);

    /* Constant pool */
    bytebuf_write_u16(buf, cw->cp->count);
    for (uint16_t i = 1; i < cw->cp->count; i++) {
        write_cp_entry(buf, cw->cp, i);
        /* Long and double take 2 slots */
        if (cw->cp->entries[i].tag == CONST_LONG ||
            cw->cp->entries[i].tag == CONST_DOUBLE) {
            i++;
        }
    }

    /* Access flags, this/super class */
    bytebuf_write_u16(buf, cw->access_flags);
    bytebuf_write_u16(buf, cw->this_class);
    bytebuf_write_u16(buf, cw->super_class);

    /* Interfaces */
    bytebuf_write_u16(buf, cw->interfaces_count);
    for (uint16_t i = 0; i < cw->interfaces_count; i++) {
        bytebuf_write_u16(buf, cw->interfaces[i]);
    }

    /* Fields */
    uint16_t field_count = slist_length(cw->fields);
    bytebuf_write_u16(buf, field_count);
    for (slist_t *s = cw->fields; s; s = s->next) {
        field_info_t *f = s->data;
        bytebuf_write_u16(buf, f->access_flags);
        bytebuf_write_u16(buf, f->name_index);
        bytebuf_write_u16(buf, f->descriptor_index);
        uint16_t attr_count = slist_length(f->attributes);
        bytebuf_write_u16(buf, attr_count);
        for (slist_t *a = f->attributes; a; a = a->next) {
            write_attribute(buf, a->data);
        }
    }

    /* Methods */
    uint16_t method_count = slist_length(cw->methods);
    bytebuf_write_u16(buf, method_count);
    for (slist_t *s = cw->methods; s; s = s->next) {
        method_info_t *m = s->data;
        bytebuf_write_u16(buf, m->access_flags);
        bytebuf_write_u16(buf, m->name_index);
        bytebuf_write_u16(buf, m->descriptor_index);
        uint16_t attr_count = slist_length(m->attributes);
        bytebuf_write_u16(buf, attr_count);
        for (slist_t *a = m->attributes; a; a = a->next) {
            write_attribute(buf, a->data);
        }
    }

    /* Class attributes */
    uint16_t attr_count = slist_length(cw->attributes);
    bytebuf_write_u16(buf, attr_count);
    for (slist_t *s = cw->attributes; s; s = s->next) {
        write_attribute(buf, s->data);
    }

    return buf;
}

bool class_writer_write_file(class_writer_t *cw, const char *output_dir)
{
    if (!cw || !output_dir) {
        return false;
    }

    /* Get class name from constant pool */
    const_entry_t *class_entry = &cw->cp->entries[cw->this_class];
    const_entry_t *name_entry = &cw->cp->entries[class_entry->data.class_index];
    const char *class_name = name_entry->data.utf8;

    /* Build output path */
    char *path = str_concat(output_dir, "/", class_name, ".class", NULL);
    if (!path) {
        return false;
    }

    /* Create parent directories */
    char *dir = str_dup(path);
    char *last_sep = strrchr(dir, '/');
    if (last_sep) {
        *last_sep = '\0';
        mkdir_p(dir);
    }
    free(dir);

    /* Serialize and write */
    bytebuf_t *buf = class_writer_to_bytes(cw);
    if (!buf) {
        free(path);
        return false;
    }

    bool result = file_put_contents(path, buf->data, buf->len);

    bytebuf_free(buf);
    free(path);
    return result;
}

