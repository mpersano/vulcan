/* vrml_tree.h -- part of vulcan
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

#ifndef VRML_H_
#define VRML_H_

union vrml_node;

enum vrml_node_type {
	VRML_NODE_SEPARATOR,
	VRML_NODE_COORDINATE3,
	VRML_NODE_INDEXED_FACE_SET,
	VRML_NODE_INFO,
};

struct vrml_node_common {
	enum vrml_node_type type;
	void (*dump_fn)(union vrml_node *node, int ident);
	void (*free_fn)(union vrml_node *node);
	char *name;
};

struct vrml_node_separator {
	struct vrml_node_common common;
	struct list *children;
};

struct vrml_node_coordinate3 {
	struct vrml_node_common common;
	struct list *points;
};

struct vrml_node_indexed_face_set {
	struct vrml_node_common common;
	struct list *indexes;
};

struct vrml_node_info {
	struct vrml_node_common common;
	char *info_str;
};

union vrml_node {
	struct vrml_node_common common;
	struct vrml_node_separator separator;
	struct vrml_node_coordinate3 coordinate3;
	struct vrml_node_indexed_face_set indexed_face_set;
	struct vrml_node_info info;
};

struct vrml_vector {
	float x, y, z;
};

struct vrml_vector *
vrml_vector_make(float x, float y, float z);

union vrml_node *
vrml_node_separator_make(struct list *children);

union vrml_node *
vrml_node_coordinate3_make(struct list *points);

union vrml_node *
vrml_node_indexed_face_set_make(struct list *indexes);

union vrml_node *
vrml_node_info_make(char *info_str);

void
vrml_node_free(union vrml_node *node);

struct list *
vrml_parse_file(const char *file_name);

#endif /* VRML_H_ */
