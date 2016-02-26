/* hash_table.h -- part of vulcan
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

#ifndef MAP_H_
#define MAP_H_

struct hash_table;

struct hash_table *
hash_table_make(int nbuckets, unsigned (*key_hash)(const void *),
  int (*key_equals)(const void *, const void *));

void
hash_table_put(struct hash_table *hash_table, void *key, void *value);

void *
hash_table_get(struct hash_table *hash_table, const void *key);

void
hash_table_free(struct hash_table *hash_table, void (*free_key)(void *), void (*free_data)(void *));

#endif /* MAP_H_ */
