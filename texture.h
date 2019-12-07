#ifndef TEXTUREH
#define TEXTUREH

#include "vec3.h"

class texture
{
public:
    virtual vec3 value(float u, float v, const vec3 &p) const = 0;
};

class constant_texture : public texture
{
public:
    constant_texture() {}
    constant_texture(vec3 c) : color(c) {}
    virtual vec3 value(float u, float v, const vec3 &p) const
    {
        return color;
    }
    vec3 color;
};

class checker_texture : public texture
{
public:
    checker_texture() {}
    checker_texture(texture *t0, texture *t1, float scale) : even(t0), odd(t1) {}
    checker_texture(vec3 c0, vec3 c1, float scale)
    {
        odd = new constant_texture(c0);
        even = new constant_texture(c1);
        scale = scale;
    }
    virtual vec3 value(float u, float v, const vec3 &p) const
    {
        float sines = scale * sin(10 * p.x()) * sin(10 * p.y()) * sin(10 * p.z());
        if (sines < 0)
        {
            return odd->value(u, v, p);
        }
        else
        {
            return even->value(u, v, p);
        }
    }
    texture *odd;
    texture *even;
    float scale;
};

#endif
