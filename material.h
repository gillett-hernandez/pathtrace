#pragma once
#include "helpers.h"
#include "hittable.h"
#include "texture.h"
#include "vec3.h"
#include "pdf.h"

class material
{
public:
    // material()
    // {
    //     assert(false);
    //     std::cout << "material should not be instantiated\n";
    // }
    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation) const
    {
        return false;
    }
    virtual vec3 generate(const ray &r, const hit_record &rec) = 0;
    virtual float value(const ray &r, const hit_record &rec, const vec3 &direction) = 0;
    // virtual float scattering_pdf(const ray &r_in, const hit_record &rec,
    //                              const ray &scattered) const
    // {
    //     return 0;
    // }
    virtual vec3 emitted(float u, float v, const vec3 &p) const
    {
        return vec3(0, 0, 0);
    }
    std::string name;
};

class lambertian : public material
{
public:
    lambertian(texture *a, std::string name = "lambertian") : albedo(a), name(name) {}
    lambertian(vec3 v, std::string name = "lambertian") : name(name)
    {
        albedo = new constant_texture(v);
    }
    bool scatter(const ray &r_in, const hit_record &rec, vec3 &alb) const
    // bool scatter(const ray &r_in, const hit_record &rec, vec3 &alb) const
    {

        // onb uvw;
        // uvw.build_from_w(rec.normal);
        // vec3 direction = uvw.local(random_cosine_direction());
        // scattered = ray(rec.p, unit_vector(direction), r_in.time());
        // cosine_pdf pdf = cosine_pdf(rec.normal);
        // scattered = ray(rec.p, pdf.generate(), r_in.time());
        alb = albedo->value(rec.u, rec.v, rec.p) / M_PI;
        return true;
    }
    // float scattering_pdf(const ray &r_in,
    //                      const hit_record &rec, const ray &scattered) const
    // {
    //     float cosine = dot(rec.normal, unit_vector(scattered.direction()));
    //     if (cosine < 0)
    //     {
    //         return 0;
    //     }
    //     return cosine / M_PI;
    // }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        return cosine_pdf(rec.normal).generate();
    };
    float value(const ray &r, const hit_record &rec, const vec3 &direction)
    {
        return cosine_pdf(rec.normal).value(direction);
    }

    texture *albedo;
    std::string name;
};

class metal : public material
{
public:
    metal(const vec3 &a, float f, std::string name = "metal") : albedo(a), name(name)
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
                         vec3 &attenuation) const
    {
        // vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        // scattered = ray(rec.p, reflected + fuzz * random_in_unit_sphere());
        attenuation = albedo;
        // return (dot(scattered.direction(), rec.normal) > 0);
        return true;
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        // for now use cosine.
        // in the future, program a microfaced brdf
        return cosine_pdf(rec.normal).generate();
    };
    float value(const ray &r, const hit_record &rec, const vec3 &direction)
    {
        return cosine_pdf(rec.normal).value(direction);
    }
    vec3 albedo;
    float fuzz;
    std::string name;
};

class dielectric : public material
{
public:
    dielectric(float ri, std::string name = "dielectric") : ref_idx(ri), name(name) {}
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation) const
    {
        // vec3 outward_normal;
        // vec3 reflected = reflect(r_in.direction(), rec.normal);
        // float ni_over_nt;
        // attenuation = vec3(1.0, 1.0, 1.0);
        // vec3 refracted;

        // float reflect_prob;
        // float cosine;

        // if (dot(r_in.direction(), rec.normal) > 0)
        // {
        //     outward_normal = -rec.normal;
        //     ni_over_nt = ref_idx;
        //     cosine = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
        // }
        // else
        // {
        //     outward_normal = rec.normal;
        //     ni_over_nt = 1.0 / ref_idx;
        //     cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
        // }

        // if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted))
        // {
        //     reflect_prob = schlick(cosine, ref_idx);
        // }
        // else
        // {
        //     reflect_prob = 1.0;
        // }

        // if (random_double() < reflect_prob)
        // {
        //     scattered = ray(rec.p, reflected);
        // }
        // else
        // {
        //     scattered = ray(rec.p, refracted);
        // }

        return true;
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        return void_pdf().generate();
    };
    float value(const ray &r, const hit_record &rec, const vec3 &direction)
    {
        return void_pdf().value(direction);
    }

    float ref_idx;
    std::string name;
};

class diffuse_light : public material
{
public:
    diffuse_light(texture *a, std::string name = "diffuse_light") : emit(a), name(name) {}
    diffuse_light(vec3 &a, std::string name = "diffuse_light") : name(name)
    {
        emit = new constant_texture(a);
    }
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation) const { return false; }
    virtual vec3 emitted(float u, float v, const vec3 &p) const
    {
        return emit->value(u, v, p);
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        return void_pdf().generate();
    };
    float value(const ray &r, const hit_record &rec, const vec3 &direction)
    {
        return void_pdf().value(direction);
    }
    texture *emit;
    std::string name;
};
class isotropic : public material
{
public:
    isotropic(texture *a, vec3 emission = vec3(0, 0, 0), std::string name = "dielectric") : albedo(a), emission(emission), name(name) {}
    isotropic(vec3 a, vec3 emission = vec3(0, 0, 0), std::string name = "dielectric") : albedo(new constant_texture(a)), emission(emission), name(name) {}
    virtual bool scatter(
        const ray &r_in,
        const hit_record &rec,
        vec3 &attenuation) const
    {

        // scattered = ray(rec.p, random_in_unit_sphere());
        attenuation = albedo->value(rec.u, rec.v, rec.p);
        return true;
    }
    virtual vec3 emitted(float u, float v, const vec3 &p) const
    {
        return emission;
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        return random_pdf().generate();
    };
    float value(const ray &r, const hit_record &rec, const vec3 &direction)
    {
        return random_pdf().value(direction);
    }
    texture *albedo;
    vec3 emission;
    std::string name;
};
