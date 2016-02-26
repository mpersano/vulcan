/* list.h -- part of vulcan
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

#ifndef LIST_H_
#define LIST_H_

struct list_node {
	struct list_node *next;
	void *data;
};

struct list {
	struct list_node *first;
	struct list_node **last;
	int length;
};

struct list *
list_make(void);

struct list *
list_append(struct list *l, void *data);

struct list *
list_prepend(struct list *l, void *data);

void
list_free(struct list *l, void(*free_data)(void *));

void *
list_element_at(const struct list *l, int n);

#endif /* LIST_H_ */
