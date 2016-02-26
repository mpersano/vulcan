#ifndef COLLISION_H_
#define COLLISION_H_

struct ray;
struct model;

int
ray_model_find_collision(float *hit_t, const struct ray *ray,
  const struct model *model);

#endif /* COLLISION_H_ */
