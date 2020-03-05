#pragma once
#include "random.h"
#include "vec3.h"
#include "types.h"
#include <math.h>
#include <vector>
#include <float.h>


inline int max(int a, int b)
{
    return a > b ? a : b;
}

inline int min(int a, int b)
{
    return a < b ? a : b;
}

vec3 reflect(const vec3 &v, const vec3 &n)
{
    return v - 2 * dot(v, n) * n;
}

bool refract(const vec3 &v, const vec3 &n, float ni_over_nt, vec3 &refracted)
{
    vec3 uv = unit_vector(v);
    float dt = dot(uv, n);
    float discriminant = 1.0 - ni_over_nt * ni_over_nt * (1 - dt * dt);
    if (discriminant > 0)
    {
        refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
        return true;
    }
    else
        return false;
}

float schlick(float cosine, float ref_idx)
{
    float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}

inline bool is_nan(const float x)
{
    return !(x == x);
}

inline bool is_nan(const vec3 v)
{
    return is_nan(v.x()) || is_nan(v.y()) || is_nan(v.z());
}

inline bool isinf(const vec3 v)
{
    return isinf(v[0]) || isinf(v[1]) || isinf(v[2]);
}

inline vec3 de_nan(const vec3 &c)
{
    vec3 temp = c;
    if (is_nan(temp[0]))
    {
        temp[0] = 0;
    }
    if (is_nan(temp[1]))
    {
        temp[1] = 0;
    }
    if (is_nan(temp[2]))
    {
        temp[2] = 0;
    }
    return temp;
}

float to_srgb(float color)
{
    if (color < 0.0031308)
    {
        return 323 * color / 25;
    }
    else
    {
        return (211 * powf(color, 5.0 / 12) - 11) / 200;
    }
}

vec3 to_srgb(vec3 color)
{
    return vec3(to_srgb(color.x()), to_srgb(color.y()), to_srgb(color.z()));
}

template <class T>
T clamp(T x, T l, T r)
{
    if (x > r)
    {
        return r;
    }
    else if (x < l)
    {
        return l;
    }
    else
    {
        return x;
    }
}

class onb
{
public:
    onb() {}
    inline vec3 operator[](int i) const { return axis[i]; }
    vec3 u() const { return axis[0]; }
    vec3 v() const { return axis[1]; }
    vec3 w() const { return axis[2]; }
    vec3 local(float a, float b, float c) const { return a * u() + b * v() + c * w(); }
    vec3 local(const vec3 &a) const { return a.x() * u() + a.y() * v() + a.z() * w(); }
    void build_from_w(const vec3 &);
    vec3 axis[3];
};

void onb::build_from_w(const vec3 &n)
{
    axis[2] = unit_vector(n);
    vec3 a;
    if (fabs(w().x()) > 0.9)
        a = vec3(0, 1, 0);
    else
        a = vec3(1, 0, 0);
    axis[1] = unit_vector(cross(w(), a));
    axis[0] = cross(w(), v());
}

inline float power_heuristic(int nf, float fPdf, int ng, float gPdf, float pow = 2.0f)
{
    float f = nf * fPdf, g = ng * gPdf;
    // return (f * f) / (f * f + g * g);
    float fpow = powf(f, pow);
    return fpow / (fpow + powf(g, pow));
}

void calculate_luminance(vec3 **framebuffer, int width, int height, int n_samples, long total_pixels, float &max_luminance, float &total_luminance, float &avg_luminance)
{
    max_luminance = -FLT_MAX;
    assert(n_samples > 0);
    total_luminance = 0.0;
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = de_nan(framebuffer[j][i]);
            col /= float(n_samples);
            float f = abs(col.length());
            total_luminance += f;
            assert(!is_nan(col));
            assert(!is_nan(total_luminance));
            if (f > max_luminance)
            {
                max_luminance = f;
            }
        }
    }
    avg_luminance = total_luminance / ((float)total_pixels * (float)n_samples);
}
