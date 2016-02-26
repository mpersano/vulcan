/* plane.h -- part of vulcan
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

#ifndef PLANE_H_
#define PLANE_H_

#include "vector.h"

struct plane {
	struct vector normal;
	struct vector pt;
};

enum rpos {
	RP_FRONT,
	RP_BACK,
	RP_ON,
	RP_SPANNING,
};

enum rpos
plane_classify_point(const struct plane *p, const struct vector *a);

float
plane_point_distance(const struct plane *p, const struct vector *a);

void
plane_intersect(struct vector *r, const struct plane *p, const struct vector *a,
  const struct vector *b);

#endif /* PLANE_H_ */
