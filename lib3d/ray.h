#ifndef RAY_H_
#define RAY_H_

#include "vector.h"

struct matrix;

struct ray {
	struct vector from;
	struct vector to;
};

void
ray_set(struct ray *r, const struct vector *from, const struct vector *to);

void
mat_transform_ray(struct ray *r, const struct matrix *m, const struct ray *o);

#endif /* RAY_H_ */
