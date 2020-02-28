#pragma once
#include "vec3.h"

class texture
{
public:
    virtual vec3 value(float u, float v, const vec3 &p) const = 0;
    virtual float alpha(float u, float v, const vec3 &p) const
    {
        return 1.0;
    }
};

class constant_texture : public texture
{
public:
    constant_texture() { a = 1.0; }
    constant_texture(vec3 c) : color(c) { a = 1.0; }
    constant_texture(vec3 c, float a) : color(c), a(a) {}
    virtual vec3 value(float u, float v, const vec3 &p) const
    {
        return color;
    }
    virtual float alpha(float u, float v, const vec3 &p) const
    {
        return a;
    }

    vec3 color;
    float a;
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
        float sines = sin(scale * p.x()) * sin(scale * p.y()) * sin(scale * p.z());
        if (sines < 0)
        {
            return odd->value(u, v, p);
        }
        else
        {
            return even->value(u, v, p);
        }
    }
    virtual float alpha(float u, float v, const vec3 &p) const
    {
        // the alpha at the specified uv value
        return 1.0;
    }
    texture *odd;
    texture *even;
    float scale;
};

// class perlin_texture : public texture
// {
// public:
//     perlin_texture() {}
// };
