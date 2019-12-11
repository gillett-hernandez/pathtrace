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
    rect(float _x0, float _z0, float _x1, float _z1, float _y, material *mat, bool flipped = false) : x0(_x0), z0(_z0), x1(_x1), z1(_z1), y(_y), mp(mat) {}
    rect(float x, float z, material *mat, bool flipped = false)
        : mp(mat), flipped(flipped)
    {
        x0 = -x / 2.0;
        z0 = -z / 2.0;
        x1 = x / 2.0;
        z1 = z / 2.0;
        y = 0.0;
    };
    rect(float x, float z, float _y, material *mat, bool flipped = false)
        : mp(mat), flipped(flipped)
    {
        x0 = -x / 2.0;
        z0 = -z / 2.0;
        x1 = x / 2.0;
        z1 = z / 2.0;
        y = _y;
    };
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        assert(x0 < x1);
        assert(z0 < z1);

        box = aabb(vec3(x0, y - 0.001, z0), vec3(x1, y + 0.001, z1));
        return true;
    }
    material *mp;
    bool flipped;
    float x0, z0, x1, z1, y;
};

bool rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    float t = (y - r.origin().y()) / r.direction().y();
    if (t < t0 || t > t1)
    {
        return false;
    }
    float xh = r.origin().x() + t * r.direction().x();
    float zh = r.origin().z() + t * r.direction().z();
    if (xh < x0 || xh > x1 || zh < z0 || zh > z1)
    {
        return false;
    }
    rec.u = (xh - x0) / (x1 - x0);
    rec.v = (zh - x0) / (z1 - z0);
    rec.t = t;
    rec.mat_ptr = mp;
    rec.p = r.point_at_parameter(t);
    rec.normal = vec3(0, 1, 0);
    return true;
}

// class  xyrect : public rect {
//     ;
// }

class box : public hittable
{
public:
    box(float width, float height, float depth, material *mat)
    {
        sides[0] = new rect(width, height, -depth, mat);
        sides[1] = new rect(width, height, depth, mat);
        sides[2] = new rect(height, depth, -width, mat);
        sides[3] = new rect(height, depth, width, mat);
        sides[4] = new rect(depth, width, -height, mat);
        sides[5] = new rect(depth, width, height, mat);
    }
    box(const vec3 p0, const vec3 p1, material *mat){
        float width = p0.x;
    }

    hittable *sides[6];
    // float width, height, depth;
};

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
