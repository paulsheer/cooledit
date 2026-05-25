/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-2-Clause) */
/* hashtable.h - binary-keyed hash table with 256 list heads
   Copyright (C) 2024 Paul Sheer
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

struct hash_table;

typedef void (*hash_table_free_value_t) (void *);

struct hash_table *hash_table_create (hash_table_free_value_t fn);
void hash_table_destroy (struct hash_table *ht);
void *hash_table_insert (struct hash_table *ht, unsigned long key_type, const unsigned char *key, int length, void *value, unsigned long extra_data);
void *hash_table_lookup (struct hash_table *ht, unsigned long key_type, const unsigned char *key, int length, unsigned long *extra_data_return);
void *hash_table_remove (struct hash_table *ht, unsigned long key_type, const unsigned char *key, int length);

#endif
