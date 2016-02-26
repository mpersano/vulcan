/* dict.h -- part of vulcan
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

#ifndef DICT_H_
#define DICT_H_

struct dict;

struct dict *
dict_make(void);

void
dict_put(struct dict *dict, const char *name, void *value);

void *
dict_get(struct dict *dict, const char *name);

void
dict_free(struct dict *dict, void (*free_data)(void *));

#endif /* DICT_H_ */
