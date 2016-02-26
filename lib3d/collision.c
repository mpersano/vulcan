#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "vector.h"
#include "matrix.h"
#include "ray.h"
#include "mesh.h"
#include "model.h"
#include "bsptree.h"
#include "collision.h"

static float
plane_intersect_ray_seg(const struct plane *plane, const struct vector *from,
  const struct vector *dir)
{
	struct vector u;

	vec_sub(&u, &plane->pt, from);

	return vec_dot_product(&u, &plane->normal) /
	  vec_dot_product(dir, &plane->normal);
}


static int
ray_seg_inside_volume(float *hit_t, struct bsp_tree *root,
  const struct vector *from, const struct vector *dir,
  const float t0, const float t1)
{
	enum rpos rp, c0, c1;
	struct vector p0, p1;
	float tm, t_front_0, t_front_1, t_back_0, t_back_1;

	assert(t0 < t1 && t0 >= 0.f && t0 <= 1.f && t1 >= 0.f && t1 <= 1.f);

	vec_scalar_mul_copy(&p0, dir, t0);
	vec_add_to(&p0, from);

	vec_scalar_mul_copy(&p1, dir, t1);
	vec_add_to(&p1, from);

  retry:
	c0 = plane_classify_point(&root->plane, &p0);
	c1 = plane_classify_point(&root->plane, &p1);

	if (c0 == RP_ON) {
		rp = c1;
	} else if (c1 == RP_ON) {
		rp = c0;
	} else if (c0 != c1) {
		rp = RP_SPANNING;
	} else {
		assert(c0 == c1);
		rp = c0;
	}

	switch (rp) {
		case RP_ON:
		case RP_FRONT:
			if (root->front == NULL)
				return 0;
			root = root->front;
			goto retry;

		case RP_BACK:
			if (root->back == NULL) {
				if (*hit_t < 0.f || t0 < *hit_t)
					*hit_t = t0;
				return 1;
			}
			root = root->back;
			goto retry;

		case RP_SPANNING:
			assert(c0 != RP_ON && c1 != RP_ON);

			tm = plane_intersect_ray_seg(&root->plane, from, dir);

			if (root->front == NULL && root->back == NULL) {
				if (*hit_t < 0.f || tm < *hit_t)
					*hit_t = tm;
				return 1;
			}

			if (c0 == RP_BACK) {
				t_back_0 = t0; t_back_1 = tm;
				t_front_0 = tm; t_front_1 = t1;
			} else {
				t_front_0 = t0; t_front_1 = tm;
				t_back_0 = tm; t_back_1 = t1;
			}

			if (root->front == NULL) {
				return ray_seg_inside_volume(hit_t, root, from,
				  dir, t_back_0, t_back_1);
			} else if (root->back == NULL) {
				return ray_seg_inside_volume(hit_t, root, from,
				  dir, t_front_0, t_front_1);
			} else {
				return ray_seg_inside_volume(hit_t, root, from,
				  dir, t_back_0, t_back_1) ||
				ray_seg_inside_volume(hit_t, root, from,
				  dir, t_front_0, t_front_1);
			}

			break;
	}

	/* NOTREACHED */

	assert(0);

	return 0;
}

static int
intersect_sphere(const struct ray *ray, const struct sphere *sphere)
{
	struct vector dir, vc;
	float a, b, c, det, sq_det, t0, t1;

	vec_sub(&dir, &ray->to, &ray->from);
	vec_sub(&vc, &ray->from, &sphere->center);

	a = vec_dot_product(&dir, &dir);
	b = 2.f*vec_dot_product(&dir, &vc);
	c = vec_dot_product(&vc, &vc) - sphere->radius*sphere->radius;
	
	det = b*b - 4.f*a*c;

	if (det < 0.f)
		return 0;

	sq_det = sqrt(det);

	t0 = (-b - sq_det)/(2.f*a);
	t1 = (-b + sq_det)/(2.f*a);

	return (t0 >= 0.f && t0 <= 1.f) || (t1 >= 0.f && t1 <= 1.f);
}


int
ray_model_find_collision(float *hit_t, const struct ray *ray,
  const struct model *model)
{
	struct matrix inv_model_transform;
	struct ray ray_model;
	struct vector dir;

	/* transform ray to model coordinates */

	mat_invert_copy(&inv_model_transform, &model->transform);
	mat_transform_ray(&ray_model, &inv_model_transform, ray);

	/* ray hits bounding sphere? */

	if (!intersect_sphere(&ray_model, &model->bounding_sphere))
		return 0;

	/* if it hits sphere, do expensive test with model bsp tree */

	vec_sub(&dir, &ray_model.to, &ray_model.from);

	return ray_seg_inside_volume(hit_t, model->bsp_tree, &ray_model.from,
	  &dir, 0.f, 1.f);
}
