#pragma once

#include "aabb.h"
#include "helpers.h"
#include "ray.h"


class material;
// class hittable;

struct hit_record
{
    float t;
    vec3 p;
    vec3 normal;
    // hittable *primitive;
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
    virtual float pdf_value(const vec3 &o, const vec3 &v) const { return 0.0; }
    virtual vec3 random(const vec3 &o) const { return vec3(1, 0, 0); }
};
