#ifndef RAYH
#define RAYH

#include "vec3.h"
#include "transform3.h"

class ray
{
public:
    ray() {}
    ray(const vec3 &a, const vec3 &b, float ti = 0.0)
    {
        A = a;
        B = b;
        assert(ti == ti);
        _time = ti;
    }
    inline vec3 origin() const { return A; }
    inline vec3 direction() const { return B; }
    inline float time() const { return _time; }
    inline vec3 point_at_parameter(float t) const { return A + t * B; }
    ray apply(transform3 transform) const
    {
        // return ray(transform * A, transform * B, _time);
        return ray(transform * A, transform.apply_linear(B), _time);
    }

    vec3 A;
    vec3 B;
    float _time;
};

#endif
