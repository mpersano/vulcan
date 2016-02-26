#include "vector.h"
#include "matrix.h"
#include "ray.h"

void
ray_set(struct ray *r, const struct vector *from, const struct vector *to)
{
	vec_copy(&r->from, from);
	vec_copy(&r->to, to);
}


void
mat_transform_ray(struct ray *r, const struct matrix *m, const struct ray *o)
{
	mat_transform_copy(&r->from, m, &o->from);
	mat_transform_copy(&r->to, m, &o->to);
}
