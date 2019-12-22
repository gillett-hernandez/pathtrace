#ifndef HELPERSH
#define HELPERSH
#include "random.h"
#include "vec3.h"
#include <math.h>

#define PI 3.14159265358979323
#define TAU 2 * PI

inline vec3 random_in_unit_sphere()
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

inline vec3 random_in_unit_disk()
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

inline vec3 random_cosine_direction()
{
    float r1 = random_double();
    float r2 = random_double();
    float z = sqrt(1 - r2);
    float phi = 2 * M_PI * r1;
    float x = cos(phi) * sqrt(r2);
    float y = sin(phi) * sqrt(r2);
    return vec3(x, y, z);
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
        vec3 u() const       { return axis[0]; }
        vec3 v() const       { return axis[1]; }
        vec3 w() const       { return axis[2]; }
        vec3 local(float a, float b, float c) const { return a*u() + b*v() + c*w(); }
        vec3 local(const vec3& a) const { return a.x()*u() + a.y()*v() + a.z()*w(); }
        void build_from_w(const vec3&);
        vec3 axis[3];
};


void onb::build_from_w(const vec3& n) {
    axis[2] = unit_vector(n);
    vec3 a;
    if (fabs(w().x()) > 0.9)
        a = vec3(0, 1, 0);
    else
        a = vec3(1, 0, 0);
    axis[1] = unit_vector(cross(w(), a));
    axis[0] = cross(w(), v());
}
#endif
