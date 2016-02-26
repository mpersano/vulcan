/* model.h -- part of vulcan
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

#ifndef MODEL_H_
#define MODEL_H_

#include "vector.h"
#include "matrix.h"

struct mesh;
struct bsp_tree;

struct sphere {
	struct vector center;
	float radius;
};

struct model {
	struct mesh *mesh;
	struct bsp_tree *bsp_tree;
	struct matrix transform;	/* rotation and translation */
	struct sphere bounding_sphere;	/* center is in model coordinates */
};

void
model_find_bounding_sphere(struct model *model);

#endif /* MODEL_H_ */
