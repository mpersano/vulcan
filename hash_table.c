/* hash_table.c -- part of vulcan
 *
 * This program is copyright (C) 2006 Mauro Persano, and is free
 * software which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#include <stdlib.h>
#include "hash_table.h"

struct hash_table_entry {
	void *key;
	void *value;
	struct hash_table_entry *next;
};

struct hash_table {
	int nbuckets;
	struct hash_table_entry **buckets;
	unsigned (*key_hash)(const void *key);
	int (*key_equals)(const void *key1, const void *key2);
};

struct hash_table *
hash_table_make(int nbuckets, unsigned (*key_hash)(const void *key),
  int (*key_equals)(const void *key1, const void *key2))
{
	struct hash_table *hash_table = malloc(sizeof *hash_table);

	hash_table->nbuckets = nbuckets;
	hash_table->buckets = calloc(hash_table->nbuckets,
	  sizeof *hash_table->buckets);
	hash_table->key_hash = key_hash;
	hash_table->key_equals = key_equals;

	return hash_table;
}

static struct hash_table_entry *
hash_table_entry_make(void *key, void *value, struct hash_table_entry *next)
{
	struct hash_table_entry *p = malloc(sizeof *p);

	p->key = key;
	p->value = value;
	p->next = next;

	return p;
}

static int
bucket_idx(struct hash_table *hash_table, const void *key)
{
	return hash_table->key_hash(key)%hash_table->nbuckets;
}

void
hash_table_put(struct hash_table *hash_table, void *key, void *value)
{
	int idx = bucket_idx(hash_table, key);

	hash_table->buckets[idx] = hash_table_entry_make(key, value,
	  hash_table->buckets[idx]);
}

void *
hash_table_get(struct hash_table *hash_table, const void *key)
{
	struct hash_table_entry *p;

	for (p = hash_table->buckets[bucket_idx(hash_table, key)]; p;
	  p = p->next) {
		if (key == p->key || hash_table->key_equals(key, p->key))
			return p->value;
	}

	return NULL;
}

void
hash_table_free(struct hash_table *hash_table, void (*free_key)(void *), void (*free_data)(void *))
{
	int i;

	for (i = 0; i < hash_table->nbuckets; i++) {
		struct hash_table_entry *p, *n;

		for (p = hash_table->buckets[i]; p; p = n) {
			n = p->next;

			if (free_key)
				free_key(p->key);

			if (free_data)
				free_data(p->value);

			free(p);
		}
	}
}
