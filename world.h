#pragma once

#include "hittable.h"
#include "texture.h"
#include "thirdparty/json.hpp"

using json = nlohmann::json;

class World : public hittable
{
public:
    World(bvh_node *ptr, texture *background, std::vector<hittable *> lights) : ptr(ptr), background(background), lights(lights)
    {
        // search through bvh and find lights
        // ptr->find_lights(&lights);
    }
    virtual bool hit(const ray &r, float tmin, float tmax, hit_record &rec) const
    {
        return ptr->hit(r, tmin, tmax, rec);
    }
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        return ptr->bounding_box(t0, t1, box);
    }

    vec3 value(float u, float v, vec3 &p)
    {
        return background->value(u, v, p);
    }

    hittable *get_random_light()
    {
        int idx = (int)(random_double() * lights.size());
        return lights[idx];
    }

    json config;
    bvh_node *ptr;
    std::vector<hittable *> lights;
    texture *background;
};
