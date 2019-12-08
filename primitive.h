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

class rect : public hittable
{
public:
    rect() {}
    rect(float _x, float _z, material *mat, bool dual_sided = false)
        : x(_x), z(_z), mp(mat), dual_sided(dual_sided){};
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        box = aabb(vec3(-x / 2.0, -0.001, -z / 2.0), vec3(x / 2.0, 0.001, z / 2.0));
        return true;
    }
    material *mp;
    bool dual_sided;
    float x, z;
};

bool rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    float t = (-r.origin().y()) / r.direction().y();
    bool flipped = false;
    if (t < 0 && dual_sided)
    {
        t = -t;
        std::cout << "blahblah" << t << '\n';
        flipped = true;
    }
    if (t < t0 || t > t1)
    {
        return false;
    }
    float xh = r.origin().x() + t * r.direction().x();
    float zh = r.origin().z() + t * r.direction().z();
    if (xh < (-x / 2.0) || xh > (x / 2.0) || zh < (-z / 2.0) || zh > (z / 2.0))
    {
        return false;
    }
    rec.u = (xh + x / 2.0) / x;
    rec.v = (zh + z / 2.0) / z;
    rec.t = t;
    rec.mat_ptr = mp;
    rec.p = r.point_at_parameter(t);
    if (!flipped)
    {
        rec.normal = vec3(0, 1, 0);
    }
    else
    {
        rec.normal = vec3(0, -1, 0);
    }
    return true;
}
class instance : public hittable
{
public:
    instance(hittable *p) : ptr(p)
    {
        transform = transform3();
        hasbbox = ptr->bounding_box(0, 1, bbox);
    }
    instance(hittable *p, transform3 transform) : ptr(p), transform(transform)
    {
        vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
        vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        hasbbox = ptr->bounding_box(0, 1, bbox);
        // narrow down the bounding box based on the transform
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    float x = i * bbox.max().x() + (1 - i) * bbox.min().x();
                    float y = j * bbox.max().y() + (1 - j) * bbox.min().y();
                    float z = k * bbox.max().z() + (1 - k) * bbox.min().z();
                    vec3 tester = transform * vec3(x, y, z);
                    for (int c = 0; c < 3; c++)
                    {
                        if (tester[c] > max[c])
                        {
                            max[c] = tester[c];
                        }
                        if (tester[c] < min[c])
                        {
                            min[c] = tester[c];
                        }
                    }
                }
            }
        }
        bbox = aabb(min, max);
    }
    virtual bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const
    {
        const ray local = r.apply(transform.inverse());
        if (ptr->hit(local, t_min, t_max, rec))
        {
            rec.p = transform * rec.p;
            rec.normal = transform.apply_normal(rec.normal);
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        box = bbox;
        return hasbbox;
    }

    transform3 transform;
    aabb bbox;
    bool hasbbox;
    hittable *ptr;
};

#endif
