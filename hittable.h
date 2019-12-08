
#include "ray.h"
#include "helpers.h"
#include "aabb.h"

#ifndef HITTABLEH
#define HITTABLEH

class material;

struct hit_record
{
    float t;
    vec3 p;
    vec3 normal;
    float u;
    float v;
    material *mat_ptr;
};

class hittable
{
public:
    virtual bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const = 0;
    virtual bool bounding_box(float t0, float t1, aabb &box) const = 0;
    // virtual bool bounding_box(float t0, float t1, aabb& box) const = 0;
};
#endif
