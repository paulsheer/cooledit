/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* hashtable.c - binary-keyed hash table with 256 list heads
   Copyright (C) 2024 Paul Sheer
 */

#include "hashtable.h"
#include "stringtools.h"

#include <stdlib.h>
#include <string.h>

#define HT_BUCKETS 256

struct hash_table_entry {
    unsigned long key_type;
    unsigned char *key;
    int length;
    void *value;
    unsigned long extra_data;
    struct hash_table_entry *next;
};

struct hash_table {
    struct hash_table_entry *buckets[HT_BUCKETS];
    hash_table_free_value_t hash_table_free_value_fn;
};

#define ADD_HASH(c) \
        v = (c); \
        r ^= v; \
        r ^= HASH401 (r % 3259);

unsigned long hash_string (const char *s_)
{
    const unsigned char *s;
    unsigned long r = 3182;
    s = (const unsigned char *) s_;
    while (*s) {
        unsigned long v;
        ADD_HASH(*s);
    }
    return r;
}

static unsigned long hash_data (const unsigned char *key, int length)
{
    unsigned long r = 3182;
    int i;
    for (i = 0; i < length; i++) {
        unsigned long v;
        ADD_HASH(key[i]);
    }
    return r;
}

static unsigned long hash_key_type (unsigned long key_type)
{
    unsigned long r = 3182;
    unsigned char *p = (unsigned char *) &key_type;
    int i;
    for (i = 0; i < (int) sizeof (key_type); i++) {
        unsigned long v;
        ADD_HASH(p[i]);
    }
    return r;
}

static unsigned int bucket (unsigned long key_type, const unsigned char *key, int length)
{
    return (hash_key_type (key_type) ^ hash_data (key, length)) % HT_BUCKETS;
}

struct hash_table *hash_table_create (hash_table_free_value_t fn)
{
    struct hash_table *ht = malloc (sizeof (struct hash_table));
    memset (ht, '\0', sizeof (struct hash_table));
    ht->hash_table_free_value_fn = fn;
    return ht;
}

void hash_table_destroy (struct hash_table * ht)
{
    int i;
    if (!ht)
        return;
    for (i = 0; i < HT_BUCKETS; i++) {
        struct hash_table_entry *e = ht->buckets[i];
        while (e) {
            struct hash_table_entry *next = e->next;
            (*ht->hash_table_free_value_fn) (e->value);
            free (e->key);
            free (e);
            e = next;
        }
    }
    free (ht);
}

void *hash_table_insert (struct hash_table * ht, unsigned long key_type, const unsigned char *key, int length, void *value, unsigned long extra_data)
{
    unsigned int b = bucket (key_type, key, length);
    struct hash_table_entry *e = ht->buckets[b];

    while (e) {
        if (e->key_type == key_type && e->length == length && !memcmp (e->key, key, length)) {
            void *old = e->value;
            e->value = value;
            e->extra_data = extra_data;
            return old;
        }
        e = e->next;
    }

    e = malloc (sizeof (struct hash_table_entry));
    e->key_type = key_type;
    e->key = (unsigned char *) malloc (length);
    memcpy (e->key, key, length);
    e->length = length;
    e->value = value;
    e->extra_data = extra_data;
    e->next = ht->buckets[b];
    ht->buckets[b] = e;
    return NULL;
}

void *hash_table_lookup (struct hash_table * ht, unsigned long key_type, const unsigned char *key, int length, unsigned long *extra_data_return)
{
    struct hash_table_entry *e = ht->buckets[bucket (key_type, key, length)];
    while (e) {
        if (e->key_type == key_type && e->length == length && !memcmp (e->key, key, length)) {
            if (extra_data_return)
                *extra_data_return = e->extra_data;
            return e->value;
        }
        e = e->next;
    }
    return NULL;
}

void *hash_table_remove (struct hash_table * ht, unsigned long key_type, const unsigned char *key, int length)
{
    unsigned int b = bucket (key_type, key, length);
    struct hash_table_entry **ep = &ht->buckets[b];

    while (*ep) {
        if ((*ep)->key_type == key_type && (*ep)->length == length && !memcmp ((*ep)->key, key, length)) {
            struct hash_table_entry *gone = *ep;
            void *value = gone->value;
            *ep = gone->next;
            free (gone->key);
            free (gone);
            return value;
        }
        ep = &(*ep)->next;
    }
    return NULL;
}
