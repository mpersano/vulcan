/* body.h -- part of vulcan
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

#ifndef BODY_H_
#define BODY_H_

#include "vector.h"
#include "matrix.h"

struct mesh;

struct sphere {
	struct vector center;
	float radius;
};

struct body {
	struct mesh *mesh;
	struct bsp_tree *bsp_tree;
	struct matrix transform;	/* rotation and translation */
	struct sphere bounding_sphere;	/* center is in body coordinates */
};

void
body_find_bounding_sphere(struct body *body);

#endif /* BODY_H_ */
