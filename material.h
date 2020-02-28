#pragma once
#include "hittable.h"
#include "texture.h"
#include "vec3.h"

class material
{
public:
    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation,
        ray &scattered) const = 0;
    virtual vec3 emitted(const ray &r_in, const hit_record &rec, float u, float v, const vec3 &p) const
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
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation, ray &scattered) const
    {
        vec3 target = rec.p + rec.normal + random_in_unit_sphere();
        scattered = ray(rec.p, target - rec.p);
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
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
    diffuse_light(texture *a, float power = 1.0) : emit(a), power(power) {}
    diffuse_light(vec3 &a, float power = 1.0) : power(power)
    {
        emit = new constant_texture(a);
    }
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation, ray &scattered) const
    {
        float alpha = emit->alpha(rec.u, rec.v, rec.p);
        if (alpha == 0.0 || random_double() > alpha || dot(rec.normal, r_in.direction()) > 0)
        {
            // passthrough ray,
            // because the random was above the alpha threshold,
            // we were at 0 alpha,
            // or the hit occurred from behind the light

            // set ray passthrough to right after the ray intersection point and in original ray direction.
            scattered = ray(rec.p + r_in.direction() * 0.01, r_in.direction());
            // set attenuation to 1 (meaning no change)
            attenuation = vec3(1.0, 1.0, 1.0); //emit->value(rec.u, rec.v, rec.p);
            return true;
        }
        return false;
    }
    virtual vec3 emitted(const ray &r_in, const hit_record &rec, float u, float v, const vec3 &p) const
    {
        if (dot(rec.normal, r_in.direction()) < 0)
        {
            // if normal and r_in.direction are in opposite directions
            // i.e. if hit on correct side of light
            return power * emit->value(u, v, p) * emit->alpha(u, v, p);
        }
        else
        {
            // normal and r_in.direction are aligned, pointing the same way
            // this means that we hit from the wrong side
            return vec3(0, 0, 0);
        }
    }
    texture *emit;
    float power;
};
