/*
** Copyright (c) 2025 oreo639
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to
** deal in the Software without restriction, including without limitation the
** rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
** sell copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
*/
#ifndef PICCO_SRC_UTIL_HASH_TABLE_H_
#define PICCO_SRC_UTIL_HASH_TABLE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
#define hash_table_create _picco_hash_table_create
#define hash_table_destroy _picco_hash_table_destroy
#define hash_table_init _picco_hash_table_init
#define hash_table_fini _picco_hash_table_fini
#define hash_table_forall _picco_hash_table_forall
#define hash_table_insert _picco_hash_table_insert
#define hash_table_insert_entry _picco_hash_table_insert_entry
#define hash_table_lookup _picco_hash_table_lookup
#define hash_table_remove _picco_hash_table_remove
#define hash_table_length _picco_hash_table_length
#define hash_key_equals_str _picco_hash_key_equals_str
#define hash_table_get_keys _picco_hash_table_get_keys
*/

struct dlist;
struct hash_table;
struct ht_entry {
	uint32_t hash;
	void *key;
	void *data;
};

typedef uint32_t (*phat_hash_fn)(const void *key);
typedef bool (*phat_equals_fn)(const void *key1, const void *key2);
typedef void (*phat_copy_fn)(struct ht_entry *ent, void *key, void *data);
typedef void (*phat_dispose_fn)(struct ht_entry elem);

struct hash_table {
	struct ht_entry *table;
	size_t table_size;
	size_t table_entries;
	phat_hash_fn hash_f;
	phat_equals_fn eq_f;
	phat_copy_fn copy_f;
	phat_dispose_fn dispose_f;
};

static inline uint32_t hash_fnv1a32_str(const void *key) {
	const char *str = key;
	uint32_t hash = 2166136261;
	for (; *str; ++str) {
		hash ^= *str;
		hash *= 16777619;
	}
	return hash;
}
bool hash_key_equals_str(const void *in1, const void *in2);

struct hash_table *hash_table_create(size_t order, phat_hash_fn hash_cb, phat_equals_fn eq_cb, phat_copy_fn copy_cb, phat_dispose_fn dispose_cb);
void hash_table_destroy(struct hash_table *ht);
bool hash_table_init(struct hash_table *ht, unsigned order, phat_hash_fn hash_cb, phat_equals_fn eq_cb, phat_copy_fn copy_cb, phat_dispose_fn dispose_cb);
void hash_table_fini(struct hash_table *ht);
void hash_table_forall(struct hash_table *ht, void (*entry_callback)(struct ht_entry elem));

size_t hash_table_length(struct hash_table *ht);

void *hash_table_insert(struct hash_table *ht, void *key, void *value, bool replace);
void *hash_table_insert_entry(struct hash_table *ht, struct ht_entry elem, bool replace);
bool hash_table_lookup(struct hash_table *ht, const void *key, void **data);
bool hash_table_remove(struct hash_table *ht, const void *key);
struct dlist *hash_table_get_keys(struct hash_table *ht);

static inline void *hash_table_get(struct hash_table *ht, const void *key) {
	void *ret = NULL;
	return (hash_table_lookup(ht, key, &ret),ret);
}

#endif
