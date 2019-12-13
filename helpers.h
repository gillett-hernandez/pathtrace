#ifndef HELPERSH
#define HELPERSH
#include "vec3.h"
#include "random.h"
#include <math.h>

#define PI 3.14159265358979323
#define TAU 2 * PI

vec3 random_in_unit_sphere()
{
    // vec3 p;
    // do
    // {
    //     p = 2.0 * vec3(random_double(), random_double(), random_double()) - vec3(1, 1, 1);
    // } while (p.squared_length() >= 1.0);
    float u, v, w;
    u = random_double() * TAU;
    v = acos(2 * random_double() - 1);
    w = powf(random_double(), 1.0 / 3.0);
    return vec3(cos(u) * sin(v) * w, cos(v) * w, sin(u) * sin(v) * w);
}

vec3 random_in_unit_disk()
{
    // vec3 p;
    // do
    // {
    //     p = 2.0 * vec3(random_double(), random_double(), 0) - vec3(1, 1, 0);
    // } while (dot(p, p) >= 1.0);
    float u, v;
    u = random_double() * TAU;
    v = powf(random_double(), 1.0 / 2.0);
    vec3 p = vec3(cos(u) * v, sin(u) * v, 0);
    return p;
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

inline vec3 de_nan(const vec3 &c)
{
    vec3 temp = c;
    if (!(temp[0] == temp[0]))
        temp[0] = 0;
    if (!(temp[1] == temp[1]))
        temp[1] = 0;
    if (!(temp[2] == temp[2]))
        temp[2] = 0;
    return temp;
}

#endif
