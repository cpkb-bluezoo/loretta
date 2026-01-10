/*
 * util.c
 * Utility functions and data structures
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

#include "util.h"

/* ========================================================================
 * Singly-linked list
 * ======================================================================== */

slist_t *slist_new(void *data)
{
    slist_t *node = malloc(sizeof(slist_t));
    if (node) {
        node->data = data;
        node->next = NULL;
    }
    return node;
}

slist_t *slist_append(slist_t *list, void *data)
{
    slist_t *node = slist_new(data);
    if (!node) {
        return list;
    }
    if (!list) {
        return node;
    }
    slist_t *last = slist_last(list);
    last->next = node;
    return list;
}

slist_t *slist_prepend(slist_t *list, void *data)
{
    slist_t *node = slist_new(data);
    if (!node) {
        return list;
    }
    node->next = list;
    return node;
}

slist_t *slist_last(slist_t *list)
{
    if (!list) {
        return NULL;
    }
    while (list->next) {
        list = list->next;
    }
    return list;
}

slist_t *slist_reverse(slist_t *list)
{
    slist_t *prev = NULL;
    slist_t *curr = list;
    while (curr) {
        slist_t *next = curr->next;
        curr->next = prev;
        prev = curr;
        curr = next;
    }
    return prev;
}

size_t slist_length(slist_t *list)
{
    size_t len = 0;
    while (list) {
        len++;
        list = list->next;
    }
    return len;
}

void slist_free(slist_t *list)
{
    while (list) {
        slist_t *next = list->next;
        free(list);
        list = next;
    }
}

void slist_free_full(slist_t *list, void (*free_func)(void *))
{
    while (list) {
        slist_t *next = list->next;
        if (free_func && list->data) {
            free_func(list->data);
        }
        free(list);
        list = next;
    }
}

/* ========================================================================
 * Hash table
 * ======================================================================== */

unsigned int str_hash(const char *str)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

hashtable_t *hashtable_new(void)
{
    hashtable_t *ht = malloc(sizeof(hashtable_t));
    if (!ht) {
        return NULL;
    }
    ht->size = HASHTABLE_INITIAL_SIZE;
    ht->count = 0;
    ht->buckets = calloc(ht->size, sizeof(hashtable_entry_t *));
    if (!ht->buckets) {
        free(ht);
        return NULL;
    }
    return ht;
}

void hashtable_free(hashtable_t *ht)
{
    if (!ht) {
        return;
    }
    for (size_t i = 0; i < ht->size; i++) {
        hashtable_entry_t *entry = ht->buckets[i];
        while (entry) {
            hashtable_entry_t *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

void hashtable_free_full(hashtable_t *ht, void (*free_func)(void *))
{
    if (!ht) {
        return;
    }
    for (size_t i = 0; i < ht->size; i++) {
        hashtable_entry_t *entry = ht->buckets[i];
        while (entry) {
            hashtable_entry_t *next = entry->next;
            free(entry->key);
            if (free_func && entry->value) {
                free_func(entry->value);
            }
            free(entry);
            entry = next;
        }
    }
    free(ht->buckets);
    free(ht);
}

static void hashtable_resize(hashtable_t *ht)
{
    size_t new_size = ht->size * 2;
    hashtable_entry_t **new_buckets = calloc(new_size, sizeof(hashtable_entry_t *));
    if (!new_buckets) {
        return;
    }

    for (size_t i = 0; i < ht->size; i++) {
        hashtable_entry_t *entry = ht->buckets[i];
        while (entry) {
            hashtable_entry_t *next = entry->next;
            unsigned int idx = str_hash(entry->key) % new_size;
            entry->next = new_buckets[idx];
            new_buckets[idx] = entry;
            entry = next;
        }
    }

    free(ht->buckets);
    ht->buckets = new_buckets;
    ht->size = new_size;
}

void hashtable_insert(hashtable_t *ht, const char *key, void *value)
{
    if (!ht || !key) {
        return;
    }

    if ((double)ht->count / ht->size > HASHTABLE_LOAD_FACTOR) {
        hashtable_resize(ht);
    }

    unsigned int idx = str_hash(key) % ht->size;

    /* Check for existing key */
    hashtable_entry_t *entry = ht->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }

    /* Insert new entry */
    entry = malloc(sizeof(hashtable_entry_t));
    if (!entry) {
        return;
    }
    entry->key = str_dup(key);
    entry->value = value;
    entry->next = ht->buckets[idx];
    ht->buckets[idx] = entry;
    ht->count++;
}

void *hashtable_lookup(hashtable_t *ht, const char *key)
{
    if (!ht || !key) {
        return NULL;
    }
    unsigned int idx = str_hash(key) % ht->size;
    hashtable_entry_t *entry = ht->buckets[idx];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

void *hashtable_remove(hashtable_t *ht, const char *key)
{
    if (!ht || !key) {
        return NULL;
    }
    unsigned int idx = str_hash(key) % ht->size;
    hashtable_entry_t **pp = &ht->buckets[idx];
    while (*pp) {
        hashtable_entry_t *entry = *pp;
        if (strcmp(entry->key, key) == 0) {
            *pp = entry->next;
            void *value = entry->value;
            free(entry->key);
            free(entry);
            ht->count--;
            return value;
        }
        pp = &entry->next;
    }
    return NULL;
}

bool hashtable_contains(hashtable_t *ht, const char *key)
{
    return hashtable_lookup(ht, key) != NULL;
}

void hashtable_foreach(hashtable_t *ht, hashtable_foreach_fn fn, void *user_data)
{
    if (!ht || !fn) {
        return;
    }
    for (size_t i = 0; i < ht->size; i++) {
        hashtable_entry_t *entry = ht->buckets[i];
        while (entry) {
            fn(entry->key, entry->value, user_data);
            entry = entry->next;
        }
    }
}

/* ========================================================================
 * Dynamic string
 * ======================================================================== */

#define STRING_INITIAL_ALLOC 64

string_t *string_new(const char *init)
{
    return string_new_len(init, init ? strlen(init) : 0);
}

string_t *string_new_len(const char *init, size_t len)
{
    string_t *str = malloc(sizeof(string_t));
    if (!str) {
        return NULL;
    }
    str->alloc = len < STRING_INITIAL_ALLOC ? STRING_INITIAL_ALLOC : len + 1;
    str->str = malloc(str->alloc);
    if (!str->str) {
        free(str);
        return NULL;
    }
    if (init && len > 0) {
        memcpy(str->str, init, len);
    }
    str->str[len] = '\0';
    str->len = len;
    return str;
}

char *string_free(string_t *str, bool free_segment)
{
    if (!str) {
        return NULL;
    }
    char *result = NULL;
    if (free_segment) {
        free(str->str);
    } else {
        result = str->str;
    }
    free(str);
    return result;
}

static void string_ensure_capacity(string_t *str, size_t needed)
{
    if (str->alloc >= needed) {
        return;
    }
    size_t new_alloc = str->alloc * 2;
    while (new_alloc < needed) {
        new_alloc *= 2;
    }
    char *new_str = realloc(str->str, new_alloc);
    if (new_str) {
        str->str = new_str;
        str->alloc = new_alloc;
    }
}

string_t *string_append(string_t *str, const char *val)
{
    if (!str || !val) {
        return str;
    }
    return string_append_len(str, val, strlen(val));
}

string_t *string_append_len(string_t *str, const char *val, size_t len)
{
    if (!str || !val || len == 0) {
        return str;
    }
    string_ensure_capacity(str, str->len + len + 1);
    memcpy(str->str + str->len, val, len);
    str->len += len;
    str->str[str->len] = '\0';
    return str;
}

string_t *string_append_c(string_t *str, char c)
{
    if (!str) {
        return str;
    }
    string_ensure_capacity(str, str->len + 2);
    str->str[str->len++] = c;
    str->str[str->len] = '\0';
    return str;
}

string_t *string_append_printf(string_t *str, const char *format, ...)
{
    if (!str || !format) {
        return str;
    }
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed > 0) {
        string_ensure_capacity(str, str->len + needed + 1);
        vsnprintf(str->str + str->len, needed + 1, format, args);
        str->len += needed;
    }
    va_end(args);
    return str;
}

string_t *string_truncate(string_t *str)
{
    if (str) {
        str->len = 0;
        str->str[0] = '\0';
    }
    return str;
}

/* ========================================================================
 * Dynamic byte buffer
 * ======================================================================== */

#define BYTEBUF_INITIAL_ALLOC 256

bytebuf_t *bytebuf_new(void)
{
    bytebuf_t *buf = malloc(sizeof(bytebuf_t));
    if (!buf) {
        return NULL;
    }
    buf->alloc = BYTEBUF_INITIAL_ALLOC;
    buf->data = malloc(buf->alloc);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    buf->len = 0;
    return buf;
}

void bytebuf_free(bytebuf_t *buf)
{
    if (buf) {
        free(buf->data);
        free(buf);
    }
}

static void bytebuf_ensure_capacity(bytebuf_t *buf, size_t needed)
{
    if (buf->alloc >= needed) {
        return;
    }
    size_t new_alloc = buf->alloc * 2;
    while (new_alloc < needed) {
        new_alloc *= 2;
    }
    uint8_t *new_data = realloc(buf->data, new_alloc);
    if (new_data) {
        buf->data = new_data;
        buf->alloc = new_alloc;
    }
}

void bytebuf_write_u8(bytebuf_t *buf, uint8_t val)
{
    bytebuf_ensure_capacity(buf, buf->len + 1);
    buf->data[buf->len++] = val;
}

void bytebuf_write_u16(bytebuf_t *buf, uint16_t val)
{
    bytebuf_ensure_capacity(buf, buf->len + 2);
    buf->data[buf->len++] = (val >> 8) & 0xFF;
    buf->data[buf->len++] = val & 0xFF;
}

void bytebuf_write_u32(bytebuf_t *buf, uint32_t val)
{
    bytebuf_ensure_capacity(buf, buf->len + 4);
    buf->data[buf->len++] = (val >> 24) & 0xFF;
    buf->data[buf->len++] = (val >> 16) & 0xFF;
    buf->data[buf->len++] = (val >> 8) & 0xFF;
    buf->data[buf->len++] = val & 0xFF;
}

void bytebuf_write_i8(bytebuf_t *buf, int8_t val)
{
    bytebuf_write_u8(buf, (uint8_t)val);
}

void bytebuf_write_i16(bytebuf_t *buf, int16_t val)
{
    bytebuf_write_u16(buf, (uint16_t)val);
}

void bytebuf_write_i32(bytebuf_t *buf, int32_t val)
{
    bytebuf_write_u32(buf, (uint32_t)val);
}

void bytebuf_write_bytes(bytebuf_t *buf, const uint8_t *data, size_t len)
{
    bytebuf_ensure_capacity(buf, buf->len + len);
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
}

void bytebuf_patch_u16(bytebuf_t *buf, size_t offset, uint16_t val)
{
    if (offset + 2 <= buf->len) {
        buf->data[offset] = (val >> 8) & 0xFF;
        buf->data[offset + 1] = val & 0xFF;
    }
}

void bytebuf_patch_u32(bytebuf_t *buf, size_t offset, uint32_t val)
{
    if (offset + 4 <= buf->len) {
        buf->data[offset] = (val >> 24) & 0xFF;
        buf->data[offset + 1] = (val >> 16) & 0xFF;
        buf->data[offset + 2] = (val >> 8) & 0xFF;
        buf->data[offset + 3] = val & 0xFF;
    }
}

/* ========================================================================
 * String utilities
 * ======================================================================== */

char *str_dup(const char *str)
{
    if (!str) {
        return NULL;
    }
    size_t len = strlen(str);
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, str, len + 1);
    }
    return dup;
}

char *str_ndup(const char *str, size_t n)
{
    if (!str) {
        return NULL;
    }
    size_t len = strlen(str);
    if (n < len) {
        len = n;
    }
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, str, len);
        dup[len] = '\0';
    }
    return dup;
}

char **str_split(const char *str, const char *delim, int max_tokens)
{
    if (!str || !delim) {
        return NULL;
    }

    /* Count tokens */
    int count = 1;
    const char *p = str;
    size_t delim_len = strlen(delim);
    while ((p = strstr(p, delim)) != NULL) {
        count++;
        p += delim_len;
        if (max_tokens > 0 && count >= max_tokens) {
            break;
        }
    }

    char **result = malloc((count + 1) * sizeof(char *));
    if (!result) {
        return NULL;
    }

    int i = 0;
    const char *start = str;
    p = str;
    while ((p = strstr(p, delim)) != NULL && (max_tokens <= 0 || i < max_tokens - 1)) {
        result[i++] = str_ndup(start, p - start);
        p += delim_len;
        start = p;
    }
    result[i++] = str_dup(start);
    result[i] = NULL;

    return result;
}

void str_freev(char **str_array)
{
    if (!str_array) {
        return;
    }
    for (int i = 0; str_array[i] != NULL; i++) {
        free(str_array[i]);
    }
    free(str_array);
}

char *str_concat(const char *first, ...)
{
    if (!first) {
        return NULL;
    }

    /* Calculate total length */
    size_t total = strlen(first);
    va_list args;
    va_start(args, first);
    const char *s;
    while ((s = va_arg(args, const char *)) != NULL) {
        total += strlen(s);
    }
    va_end(args);

    /* Allocate and concatenate */
    char *result = malloc(total + 1);
    if (!result) {
        return NULL;
    }

    strcpy(result, first);
    va_start(args, first);
    while ((s = va_arg(args, const char *)) != NULL) {
        strcat(result, s);
    }
    va_end(args);

    return result;
}

char *str_strip(char *str)
{
    if (!str) {
        return NULL;
    }

    /* Skip leading whitespace */
    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }

    /* Find end and trim trailing whitespace */
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) {
        end--;
    }
    *end = '\0';

    /* Shift string if needed */
    if (start != str) {
        memmove(str, start, end - start + 1);
    }

    return str;
}

bool str_has_suffix(const char *str, const char *suffix)
{
    if (!str || !suffix) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return false;
    }
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool str_has_prefix(const char *str, const char *prefix)
{
    if (!str || !prefix) {
        return false;
    }
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/* ========================================================================
 * File utilities
 * ======================================================================== */

bool file_exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

bool file_is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool file_is_regular(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

char *get_current_dir(void)
{
    char *buf = malloc(4096);
    if (!buf) {
        return NULL;
    }
    if (getcwd(buf, 4096) == NULL) {
        free(buf);
        return NULL;
    }
    return buf;
}

char *file_get_contents(const char *filename, size_t *length)
{
    FILE *f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return NULL;
    }

    char *contents = malloc(size + 1);
    if (!contents) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(contents, 1, size, f);
    fclose(f);

    if ((long)read != size) {
        free(contents);
        return NULL;
    }

    contents[size] = '\0';
    if (length) {
        *length = size;
    }
    return contents;
}

bool file_put_contents(const char *filename, const uint8_t *contents, size_t length)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        return false;
    }

    size_t written = fwrite(contents, 1, length, f);
    fclose(f);

    return written == length;
}

bool mkdir_p(const char *path)
{
    char *dup = str_dup(path);
    if (!dup) {
        return false;
    }

    bool result = true;
    char *p = dup;

    /* Skip leading slash */
    if (*p == DIR_SEPARATOR) {
        p++;
    }

    while (*p) {
        /* Find next separator */
        while (*p && *p != DIR_SEPARATOR) {
            p++;
        }

        char saved = *p;
        *p = '\0';

        if (strlen(dup) > 0 && !file_exists(dup)) {
            if (mkdir(dup, 0755) != 0 && errno != EEXIST) {
                result = false;
                break;
            }
        }

        *p = saved;
        if (*p) {
            p++;
        }
    }

    free(dup);
    return result;
}

