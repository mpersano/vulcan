/* bsptree.h -- part of vulcan
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

#ifndef BSPTREE_H_
#define BSPTREE_H_

#include "plane.h"

struct bsp_tree {
	int num_children;
	struct plane plane;
	struct bsp_tree *front, *back;
};

struct mesh;

struct bsp_tree *
bsp_tree_build_from_mesh(const struct mesh *mesh);

#endif /* BSPTREE_H_ */
