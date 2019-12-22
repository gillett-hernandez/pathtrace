#pragma once
#include "helpers.h"
#include "hittable.h"
#include "texture.h"
#include "vec3.h"

class material
{
public:
    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation,
        ray &scattered, float &pdf) const
    {
        return false;
    }
    virtual float scattering_pdf(const ray &r_in, const hit_record &rec,
                                 const ray &scattered) const
    {
        return 0;
    }
    virtual vec3 emitted(float u, float v, const vec3 &p) const
    {
        return vec3(0, 0, 0);
    }
};

class lambertian : public material
{
public:
    lambertian(texture *a) : albedo(a) {}
    lambertian(vec3 v)
    {
        albedo = new constant_texture(v);
    }
    bool scatter(const ray &r_in,
                 const hit_record &rec, vec3 &alb, ray &scattered, float &pdf) const
    {
        // vec3 target = rec.p + rec.normal + random_in_unit_sphere();
        // scattered = ray(rec.p, unit_vector(target - rec.p), r_in.time());
        // vec3 direction;
        // do
        // {
        //     direction = random_in_unit_sphere();
        // } while (dot(direction, rec.normal) < 0);

        // scattered = ray(rec.p, unit_vector(direction), r_in.time());

        onb uvw;
        uvw.build_from_w(rec.normal);
        vec3 direction = uvw.local(random_cosine_direction());
        scattered = ray(rec.p, unit_vector(direction), r_in.time());
        alb = albedo->value(rec.u, rec.v, rec.p);
        pdf = dot(rec.normal, scattered.direction()) / M_PI;
        return true;
    }
    float scattering_pdf(const ray &r_in,
                         const hit_record &rec, const ray &scattered) const
    {
        float cosine = dot(rec.normal, unit_vector(scattered.direction()));
        if (cosine < 0)
            return 0;
        return cosine / M_PI;
    }

    texture *albedo;
};

class metal : public material
{
public:
    metal(const vec3 &a, float f) : albedo(a)
    {
        if (f < 1)
        {
            fuzz = f;
        }
        else
        {
            fuzz = 1;
        }
    }
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation, ray &scattered) const
    {
        vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.normal) > 0);
    }
    vec3 albedo;
    float fuzz;
};

class dielectric : public material
{
public:
    dielectric(float ri) : ref_idx(ri) {}
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation, ray &scattered) const
    {
        vec3 outward_normal;
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        float ni_over_nt;
        attenuation = vec3(1.0, 1.0, 1.0);
        vec3 refracted;

        float reflect_prob;
        float cosine;

        if (dot(r_in.direction(), rec.normal) > 0)
        {
            outward_normal = -rec.normal;
            ni_over_nt = ref_idx;
            cosine = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
        }
        else
        {
            outward_normal = rec.normal;
            ni_over_nt = 1.0 / ref_idx;
            cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
        }

        if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted))
        {
            reflect_prob = schlick(cosine, ref_idx);
        }
        else
        {
            reflect_prob = 1.0;
        }

        if (random_double() < reflect_prob)
        {
            scattered = ray(rec.p, reflected);
        }
        else
        {
            scattered = ray(rec.p, refracted);
        }

        return true;
    }

    float ref_idx;
};

class diffuse_light : public material
{
public:
    diffuse_light(texture *a) : emit(a) {}
    diffuse_light(vec3 &a)
    {
        emit = new constant_texture(a);
    }
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation, ray &scattered) const { return false; }
    virtual vec3 emitted(float u, float v, const vec3 &p) const
    {
        return emit->value(u, v, p);
    }
    texture *emit;
};
