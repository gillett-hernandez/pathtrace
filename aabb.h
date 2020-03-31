
#pragma once
#include <stdlib.h>
#include "ray.h"
#include "transform3.h"

inline float ffmin(float a, float b) { return a < b ? a : b; }
inline float ffmax(float a, float b) { return a > b ? a : b; }

class aabb
{
public:
    aabb() {}
    aabb(const vec3 &a, const vec3 &b, bool skip_check = true)
    {
        if (skip_check)
        {
            _min = a;
            _max = b;
        }
        else
        {
            _min = vec3(ffmin(a.x(), b.x()), ffmin(a.y(), b.y()), ffmin(a.z(), b.z()));
            _max = vec3(ffmax(a.x(), b.x()), ffmax(a.y(), b.y()), ffmax(a.z(), b.z()));
        }
    }

    aabb apply(transform3 transform) const
    {
        return aabb(transform * _min, transform * _max);
    }

    inline aabb extend(vec3 new_point) const
    {
        vec3 small(ffmin(_min.x(), new_point.x()),
                   ffmin(_min.y(), new_point.y()),
                   ffmin(_min.z(), new_point.z()));
        vec3 big(ffmax(_max.x(), new_point.x()),
                 ffmax(_max.y(), new_point.y()),
                 ffmax(_max.z(), new_point.z()));
        return aabb(small, big);
    }

    vec3 min() const { return _min; }
    vec3 max() const { return _max; }

    bool hit(const ray &r, float tmin, float tmax) const;

    vec3 _min;
    vec3 _max;
};

inline bool aabb::hit(const ray &r, float tmin, float tmax) const
{
    for (int a = 0; a < 3; a++)
    {
        float invD = 1.0f / r.direction()[a];
        float t0 = (min()[a] - r.origin()[a]) * invD;
        float t1 = (max()[a] - r.origin()[a]) * invD;
        if (invD < 0.0f)
        {
            std::swap(t0, t1);
        }
        tmin = t0 > tmin ? t0 : tmin;
        tmax = t1 < tmax ? t1 : tmax;
        if (tmax <= tmin)
        {
            return false;
        }
    }
    return true;
}

aabb surrounding_box(aabb box0, aabb box1)
{
    vec3 small(ffmin(box0.min().x(), box1.min().x()),
               ffmin(box0.min().y(), box1.min().y()),
               ffmin(box0.min().z(), box1.min().z()));
    vec3 big(ffmax(box0.max().x(), box1.max().x()),
             ffmax(box0.max().y(), box1.max().y()),
             ffmax(box0.max().z(), box1.max().z()));
    return aabb(small, big);
}
