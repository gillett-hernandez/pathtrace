#ifndef WORLDH
#define WORLDH

#include "hittable.h"
#include "texture.h"

class world : public hittable {
public:
    world(hittable* ptr, texture* background): ptr(ptr), background(background) {}
    virtual bool hit(const ray &r, float tmin, float tmax, hit_record &rec) const
    {
        return ptr->hit(r, tmin, tmax, rec);
    }
    virtual bool bounding_box(float t0, float t1, aabb &box) const {
        return ptr->bounding_box(t0, t1, box);
    }

    vec3 value(float u, float v, vec3 &p) {
        return background->value(u, v, p);
    }

    hittable* ptr;
    texture* background;
};

#endif