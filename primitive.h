#pragma once
#include "bvh.h"
#include "scene.h"
#include "hittable.h"
#include "hittable_list.h"
#include "ray.h"
#include "transform3.h"
#include "vec3.h"
#include <float.h>

enum plane_enum
{
    XY,
    XZ,
    YZ
};

plane_enum plane_enum_mapping(std::string alignment)
{
    static std::map<std::string, plane_enum> mapping = {
        {"xy", XY},
        {"xz", XZ},
        {"yz", YZ}};
    return mapping[alignment];
}

class sphere : public hittable
{
public:
    sphere() {}

    sphere(vec3 cen, float r, material *m)
        : center(cen), radius(r), mat_ptr(m){};

    virtual bool hit(const ray &r, float tmin, float tmax, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const;
    virtual float pdf_value(const vec3 &o, const vec3 &v) const
    {
        hit_record rec;
        if (this->hit(ray(o, v), 0.001, FLT_MAX, rec))
        {
            float cos_theta_max = sqrt(1 - radius * radius / (center - o).squared_length());
            float solid_angle = 2 * M_PI * (1 - cos_theta_max);
            return 1 / solid_angle;
        }
        else
        {
            return 0;
        }
    }
    virtual vec3 random(const vec3 &o) const
    {
        vec3 direction = center - o;
        float distance_squared = direction.squared_length();
        onb uvw;
        uvw.build_from_w(direction);
        return uvw.local(random_to_sphere(radius, distance_squared));
    }
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
            rec.mat_ptr = mat_ptr;
            // rec.primitive = (hittable *)this;
            return true;
        }
        temp = (-b + sqrt(discriminant)) / a;
        if (temp < t_max && temp > t_min)
        {
            rec.t = temp;
            rec.p = r.point_at_parameter(rec.t);
            rec.normal = (rec.p - center) / radius;
            rec.mat_ptr = mat_ptr;
            // rec.primitive = (hittable *)this;
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

inline vec3 shuffle(vec3 v, plane_enum style)
{
    switch (style)
    {
    case XY:
    {
        return vec3(v.x(), v.z(), v.y());
    }
    case YZ:
    {
        return vec3(v.y(), v.x(), v.z());
    }
    default:
    {
        return v;
    }
    }
}



class rect : public hittable
{
public:
    rect() {}
    rect(float _x0, float _z0, float _x1, float _z1, float _y, material *mat, plane_enum type = XZ, bool flipped = false)
        : x0(_x0), z0(_z0), x1(_x1), z1(_z1), y(_y), mp(mat), type(type), normal(!flipped)
    {
        // std::cout << "constructor called with " << type << '\n';
    }
    rect(float x, float z, material *mat, plane_enum type = XZ, bool flipped = false)
        : rect(-x / 2.0, -z / 2.0, x / 2.0, z / 2.0, 0.0, mat, type, flipped)
    {
        // std::cout << "constructor called with " << type << '\n';
    }

    rect(float x, float z, float _y, material *mat, plane_enum type = XZ, bool flipped = false)
        : rect(x, z, mat, type, flipped)
    {
        y = _y;
        // std::cout << "constructor called with " << type << '\n';
    }
    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        assert(x0 < x1);
        assert(z0 < z1);
        vec3 a = vec3(x0, y - 0.001, z0);
        vec3 b = vec3(x1, y + 0.001, z1);
        // switch some variables around for axis alignment
        box = aabb(shuffle(a, type), shuffle(b, type));
        return true;
    }

    virtual float pdf_value(const vec3 &o, const vec3 &v) const
    {
        hit_record rec;
        if (this->hit(ray(o, v), 0.001, FLT_MAX, rec))
        {
            float area = (x1 - x0) * (z1 - z0);
            float vlen = v.length();
            float distance_squared = powf(rec.t * vlen, 2.0);
            float cosine = fabs(dot(v, rec.normal) / vlen);
            return distance_squared / (cosine * area);
        }
        else
        {
            return 0;
        }
    }
    virtual vec3 random(const vec3 &o) const
    {
        vec3 random_point = shuffle(vec3(x0 + random_double() * (x1 - x0), y,
                                         z0 + random_double() * (z1 - z0)),
                                    type);
        return random_point - o;
    }
    material *mp;
    bool normal;
    float x0, z0, x1, z1, y;
    plane_enum type;
};

bool rect::hit(const ray &r, float t0, float t1, hit_record &rec) const
{
    // float rox, roy, roz, rdx, rdy, rdz;
    // conversion to simulated/transformed space
    vec3 temp_o = shuffle(r.origin(), type);
    vec3 temp_d = shuffle(r.direction(), type);

    float t = (y - temp_o.y()) / temp_d.y();
    // time value of hit outside of bounds?
    if (t < t0 || t > t1)
    {
        return false;
    }
    float xh = temp_o.x() + t * temp_d.x();
    float zh = temp_o.z() + t * temp_d.z();
    // coordinates of hit outside of bounds?
    if (xh < x0 || xh > x1 || zh < z0 || zh > z1)
    {
        return false;
    }
    rec.u = (xh - x0) / (x1 - x0);
    rec.v = (zh - x0) / (z1 - z0);
    rec.t = t;
    rec.mat_ptr = mp;

    rec.p = r.point_at_parameter(t);
    rec.normal = shuffle(vec3(0, 2 * normal - 1, 0), type);
    // rec.primitive = (hittable *)this;
    return true;
}

class box : public hittable
{
public:
    box(float width, float height, float depth, material *mat) : box(vec3(-width / 2, -height / 2, -depth / 2), vec3(width / 2, height / 2, depth / 2), mat) {}
    box(const vec3 p0, const vec3 p1, material *mat) : p0(p0), p1(p1)
    {
        hittable **sides = new hittable *[6];
        sides[0] = new rect(p0.x(), p0.y(), p1.x(), p1.y(), p0.z(), mat, XY, true);
        sides[1] = new rect(p0.x(), p0.y(), p1.x(), p1.y(), p1.z(), mat, XY);
        sides[2] = new rect(p0.y(), p0.z(), p1.y(), p1.z(), p0.x(), mat, YZ, true);
        sides[3] = new rect(p0.y(), p0.z(), p1.y(), p1.z(), p1.x(), mat, YZ);
        sides[4] = new rect(p0.x(), p0.z(), p1.x(), p1.z(), p0.y(), mat, XZ, true);
        sides[5] = new rect(p0.x(), p0.z(), p1.x(), p1.z(), p1.y(), mat, XZ);
        group = new hittable_list(sides, 6);
    }

    virtual bool hit(const ray &r, float t0, float t1, hit_record &rec) const
    {
        return group->hit(r, t0, t1, rec);
    }
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        // box = aabb(p0, p1);
        group->bounding_box(t0, t1, box);
        return true;
    }

    hittable *group;
    vec3 p0, p1;
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
    virtual float pdf_value(const vec3 &o, const vec3 &v) const
    {
        // hit_record rec;
        // if (this->hit(ray(o, v), 0.001, FLT_MAX, rec))
        // {
        //     float area = (x1 - x0) * (z1 - z0);
        //     float vlen = v.length();
        //     float distance_squared = powf(rec.t * vlen, 2.0);
        //     float cosine = fabs(dot(v, rec.normal) / vlen);
        //     return distance_squared / (cosine * area);
        // }
        // else
        // {
        //     return 0;
        // }

        // inverse transform to local space
        return ptr->pdf_value(transform.inverse() * o, transform.inverse().apply_linear(v));
    }
    virtual vec3 random(const vec3 &o) const
    {
        // inverse transform
        return transform.apply_linear(ptr->random(transform.inverse() * o));
    }

    transform3 transform;
    aabb bbox;
    bool hasbbox;
    hittable *ptr;
};
