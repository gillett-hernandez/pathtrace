#ifndef PRIMITIVEH
#define PRIMITIVEH
#include "hittable.h"
#include "ray.h"
#include "vec3.h"
#include "transform3.h"
#include "bvh.h"
#include "hittable.h"


class sphere : public hittable
{
public:
    sphere() {}

    sphere(vec3 cen, float r, material *m)
        : center(cen), radius(r), mat_ptr(m){};

    virtual bool hit(const ray &r, float tmin, float tmax, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const;
    vec3 center;
    float radius;
    material *mat_ptr;
};

bool sphere::hit(const ray &r, float t_min, float t_max, hit_record &rec) const
{
    vec3 oc = r.origin() - center;
    float a = dot(r.direction(), r.direction());
    float b = dot(oc, r.direction());
    float c = dot(oc, oc) - radius * radius;
    float discriminant = b * b - a * c;
    if (discriminant > 0)
    {
        float temp = (-b - sqrt(discriminant)) / a;
        if (temp < t_max && temp > t_min)
        {
            rec.t = temp;
            rec.p = r.point_at_parameter(rec.t);
            rec.normal = (rec.p - center) / radius;
            ;
            rec.mat_ptr = mat_ptr;
            return true;
        }
        temp = (-b + sqrt(discriminant)) / a;
        if (temp < t_max && temp > t_min)
        {
            rec.t = temp;
            rec.p = r.point_at_parameter(rec.t);
            rec.normal = (rec.p - center) / radius;
            rec.mat_ptr = mat_ptr;
            return true;
        }
    }
    return false;
}

bool sphere::bounding_box(float t0, float t1, aabb &box) const
{
    box = aabb(center - vec3(radius, radius, radius),
               center + vec3(radius, radius, radius));
    return true;
}

class xy_rect : public hittable
{
public:
    xy_rect() {}
    xy_rect(float _x0, float _x1, float _y0, float _y1, float _k, material *mat)
        : x0(_x0), x1(_x1), y0(_y0), y1(_y1), k(_k), mp(mat){};
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        box = aabb(vec3(x0, y0, k - 0.0001), vec3(x1, y1, k + 0.0001));
        return true;
    }
    material *mp;
    float x0, x1, y0, y1, k;
};

class xz_rect : public hittable
{
public:
    xz_rect() {}
    xz_rect(float _x0, float _x1, float _z0, float _z1, float _k, material *mat)
        : x0(_x0), x1(_x1), z0(_z0), z1(_z1), k(_k), mp(mat){};
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        box = aabb(vec3(x0, k - 0.0001, z0), vec3(x1, k + 0.0001, z1));
        return true;
    }
    material *mp;
    float x0, x1, z0, z1, k;
};

class yz_rect : public hittable
{
public:
    yz_rect() {}
    yz_rect(float _y0, float _y1, float _z0, float _z1, float _k, material *mat)
        : y0(_y0), y1(_y1), z0(_z0), z1(_z1), k(_k), mp(mat){};
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        box = aabb(vec3(k - 0.0001, y0, z0), vec3(k + 0.0001, y1, z1));
        return true;
    }
    material *mp;
    float y0, y1, z0, z1, k;
};

bool xz_rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    float t = (k - r.origin().y()) / r.direction().y();
    if (t < t0 || t > t1)
        return false;
    float x = r.origin().x() + t * r.direction().x();
    float z = r.origin().z() + t * r.direction().z();
    if (x < x0 || x > x1 || z < z0 || z > z1)
        return false;
    rec.u = (x - x0) / (x1 - x0);
    rec.v = (z - z0) / (z1 - z0);
    rec.t = t;
    rec.mat_ptr = mp;
    rec.p = r.point_at_parameter(t);
    rec.normal = vec3(0, 1, 0);
    return true;
}

bool yz_rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    float t = (k - r.origin().x()) / r.direction().x();
    if (t < t0 || t > t1)
        return false;
    float y = r.origin().y() + t * r.direction().y();
    float z = r.origin().z() + t * r.direction().z();
    if (y < y0 || y > y1 || z < z0 || z > z1)
        return false;
    rec.u = (y - y0) / (y1 - y0);
    rec.v = (z - z0) / (z1 - z0);
    rec.t = t;
    rec.mat_ptr = mp;
    rec.p = r.point_at_parameter(t);
    rec.normal = vec3(1, 0, 0);
    return true;
}

bool xy_rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    float t = (k - r.origin().z()) / r.direction().z();
    if (t < t0 || t > t1)
        return false;
    float x = r.origin().x() + t * r.direction().x();
    float y = r.origin().y() + t * r.direction().y();
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return false;
    rec.u = (x - x0) / (x1 - x0);
    rec.v = (y - y0) / (y1 - y0);
    rec.t = t;
    rec.mat_ptr = mp;
    rec.p = r.point_at_parameter(t);
    rec.normal = vec3(0, 0, 1);
    return true;
}

// class instance : public hittable
// {
// public:
//     instance(hittable *p, transform3 transform) : ptr(p), transform(transform) {}
//     virtual bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const
//     {
//         ray local = transform.apply(r);
//         if (ptr->hit(local, t_min, t_max, rec))
//         {
//             return true;
//         }
//         else
//         {
//             return false;
//         }
//     }
//     transform3 transform;
//     hittable * ptr;
// };

class flip_normals : public hittable
{
public:
    flip_normals(hittable *p) : ptr(p) {}

    virtual bool hit(
        const ray &r, float t_min, float t_max, hit_record &rec) const
    {

        if (ptr->hit(r, t_min, t_max, rec))
        {
            rec.normal = -rec.normal;
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        return ptr->bounding_box(t0, t1, box);
    }

    hittable *ptr;
};

#endif
