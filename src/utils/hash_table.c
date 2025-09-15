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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
//#include <stdio.h>

#include "hash_table.h"

#define MAX(a, b) ((a)>(b)?(a):(b))

static const unsigned tombstone_key_value;
static const void *tombstone_key = &tombstone_key_value;

bool hash_table_init(struct hash_table *ht, unsigned order, phat_hash_fn hash_cb, phat_equals_fn eq_cb, phat_copy_fn copy_cb, phat_dispose_fn dispose_cb) {
	if (!ht || !hash_cb || !eq_cb)
		return false;
	ht->table_size = 1<<order;
	ht->hash_f = hash_cb;
	ht->eq_f = eq_cb;
	ht->copy_f = copy_cb;
	ht->dispose_f = dispose_cb;
	ht->table_entries = 0;
	ht->table = calloc(ht->table_size, sizeof(*ht->table));
	if (!ht->table)
		return false;

	return true;
}

void hash_table_fini(struct hash_table *ht) {
	if (!ht) return;
	if (ht->dispose_f) {
		for (size_t i = 0; i < ht->table_size; ++i) {
			if (ht->table[i].key != NULL && ht->table[i].key != tombstone_key)
				ht->dispose_f(ht->table[i]);
		}
	}
	free(ht->table);
	ht->table_entries = 0;
}

struct hash_table *hash_table_create(size_t order, phat_hash_fn hash_cb, phat_equals_fn eq_cb, phat_copy_fn copy_cb, phat_dispose_fn dispose_cb) {
	struct hash_table *ht = calloc(1, sizeof(struct hash_table));
	return hash_table_init(ht, order, hash_cb, eq_cb, copy_cb, dispose_cb) ? ht : NULL;
}

void hash_table_destroy(struct hash_table *ht) {
	hash_table_fini(ht);
	free(ht);
}

static bool _picco_hash_table_expand(struct hash_table *ht) {
	struct hash_table newht = *ht;
	newht.table = NULL;
	newht.table_size = ht->table_size<<1;
	newht.table_entries = 0;

	if (newht.table_size < ht->table_size)
		return false; // No more addressable memory
	newht.table = calloc(newht.table_size, sizeof(*ht->table));
	if (!newht.table)
		return false; // Out of memory

	for (size_t i = 0; (newht.table_entries != ht->table_entries) && (i < ht->table_size); ++i) {
		if (ht->table[i].key && ht->table[i].key != tombstone_key)
			hash_table_insert_entry(&newht, ht->table[i], false);
	}

	assert(newht.table_entries == ht->table_entries); // Something went horribly wrong

	if (ht->table)
		free(ht->table);
	*ht = newht;
	return true;
}

static inline void *hash_table_insert_internal(struct hash_table *ht, uint32_t hash, void *key, void *data, bool replace, bool copy) {
	struct ht_entry *ins = NULL;

	if (MAX(ht->table_entries+1, ht->table_entries*10) >= ht->table_size)
		_picco_hash_table_expand(ht);

	for (size_t i = 0; i < ht->table_size; ++i) {
		size_t try = (i + hash) & (ht->table_size-1);
		if (ht->table[try].key == NULL) {
			if (!ins) ins = &ht->table[try];
			break;
		} else if (ht->table[try].key == tombstone_key) {
			if (!ins) ins = &ht->table[try];
		} else if (ht->eq_f(ht->table[try].key, key)) {
			if (replace) {
				if (copy && ht->copy_f) ht->copy_f(&ht->table[try], key, data);
				else {ins->key = key; ins->data = data;}
			}
			return ht->table[try].data;
		}
	}

	if (ins) {
		*ins = (struct ht_entry){.hash = hash};
		if (copy && ht->copy_f) ht->copy_f(ins, key, data);
		else {ins->key = key; ins->data = data;}
		ht->table_entries++;
	}

	return ins->data;
}

void *hash_table_insert_entry(struct hash_table *ht, struct ht_entry elem, bool replace) {
	return hash_table_insert_internal(ht, elem.hash, elem.key, elem.data, replace, false);
}

void *hash_table_insert(struct hash_table *ht, void *key, void *data, bool replace) {
	return hash_table_insert_internal(ht, ht->hash_f(key), key, data, replace, true);
}

bool hash_table_lookup(struct hash_table *ht, const void *key, void **data) {
	uint32_t hash = ht->hash_f(key);

	for (size_t i = 0; i < ht->table_size; ++i) {
		size_t try = (i + hash) & (ht->table_size-1);
		//fprintf(stderr, "DEBUG: hash_table_find(%s): try(%zu):%p\n", (char*)elem, try, ht->table[try]);
		if (ht->table[try].key == NULL) {
			return false; // Not here
		} else if (ht->table[try].key == tombstone_key) {
			continue;
		} else if (ht->eq_f(ht->table[try].key, key)) {
			if (data) *data = ht->table[try].data;
			return true;
		}
	}

	return false;
}

bool hash_table_remove(struct hash_table *ht, const void *key) {
	uint32_t hash = ht->hash_f(key);

	for (size_t i = 0; i < ht->table_size; ++i) {
		size_t try = (i + hash) & (ht->table_size-1);
		//fprintf(stderr, "DEBUG: hash_table_find(%s): try(%zu):%p\n", (char*)elem, try, ht->table[try]);
		if (ht->table[try].key == NULL) {
			return false; // Not here
		} else if (ht->table[try].key == tombstone_key) {
			continue;
		} else if (ht->eq_f(ht->table[try].key, key)) {
			if (ht->dispose_f) ht->dispose_f(ht->table[try]);
			ht->table[try].key = (void*)tombstone_key;
			--ht->table_entries;
			return true;
		}
	}

	return false;
}

size_t hash_table_length(struct hash_table *ht) {
	return ht->table_entries;
}

void hash_table_forall(struct hash_table *ht, void (*entry_callback)(struct ht_entry elem)) {
	if (ht && entry_callback) {
		for (size_t i = 0; i < ht->table_size; ++i) {
			if (ht->table[i].key && ht->table[i].key != tombstone_key) {
				entry_callback(ht->table[i]);
			}
		}
	}
}

bool hash_key_equals_str(const void *in1, const void *in2) {
	return strcmp(in1, in2) == 0;
}
/*
void hash_table_print(struct hash_table *ht) {
	if (ht) {
		for (size_t i = 0; i < ht->table_size; ++i) {
			if (!ht->table[i].key)
				fprintf(stderr, "\t%zu\t---\n", i);
			else if (ht->table[i].key == tombstone_key)
				fprintf(stderr, "\t%zu\t-<DELETED>-\n", i);
			if (ht->table[i].key && ht->table[i].key != tombstone_key)
				fprintf(stderr, "\t%zu\t%s --\n", i, (char*)ht->table[i].key);
		}
	}
}*/
