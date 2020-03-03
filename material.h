#pragma once
#include "hittable.h"
#include "texture.h"
#include "vec3.h"
#include "pdf.h"

class material
{
public:
    virtual bool scatter(
        const ray &r_in, const hit_record &rec, vec3 &attenuation) const
    {
        // mauve
        std::cout << "WARNING! material::scatter being called" << std::endl;
        attenuation = vec3(0.8, 0.0, 0.8);
        return true;
    }
    virtual vec3 generate(const ray &r, const hit_record &rec) = 0;
    virtual float value(const ray &r, const hit_record &rec, const vec3 &direction) = 0;
    virtual vec3 emitted(const ray &r_in, const hit_record &rec, float u, float v, const vec3 &p) const
    {
        return vec3(0, 0, 0);
    }
    std::string name = "error";
};

class lambertian : public material
{
public:
    lambertian()
    {
        // bsdf_pdf = new cosine_pdf();
    }
    lambertian(texture *a, std::string name = "lambertian") : albedo(a), name(name) {}
    lambertian(vec3 v, std::string name = "lambertian") : name(name)
    {
        albedo = new constant_texture(v);
    }
    bool scatter(const ray &r_in, const hit_record &rec, vec3 &attenuation) const
    {

        attenuation = albedo->value(rec.u, rec.v, rec.p) / M_PI;

        // scattered = ray(rec.p, target - rec.p);
        return true;
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        /*float alpha = albedo->alpha(rec.u, rec.v, rec.p);
                if (alpha == 0.0 || random_double() > alpha)
        {
            // passthrough
            target = rec.p + r_in.direction();
            attenuation = vec3(1.0, 1.0, 1.0);
        }
        else
        {*/
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
        attenuation = albedo / M_PI;
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
        attenuation = vec3(1.0, 1.0, 1.0); // change this to something else to have a tinted glass material

        return true;
    }
    vec3 generate(const ray &r_in, const hit_record &rec)
    {
        vec3 outward_normal;
        vec3 reflected = reflect(r_in.direction(), rec.normal);
        float ni_over_nt;
        vec3 scattered_direction;
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
            scattered_direction = reflected;
        }
        else
        {
            scattered_direction = refracted;
        }
        return scattered_direction;
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
    diffuse_light(texture *a, float power = 1.0, bool two_sided = true) : emit(a), power(power), two_sided(two_sided)
    {
    }
    diffuse_light(vec3 &a, float power = 1.0, bool two_sided = true) : power(power), two_sided(two_sided)
    {
        emit = new constant_texture(a);
    }
    virtual bool scatter(const ray &r_in, const hit_record &rec,
                         vec3 &attenuation) const
    {
        return false;
    }
    // virtual vec3 emitted(float u, float v, const vec3 &p) const
    //                      vec3 &attenuation, ray &scattered) const
    // {
    // float alpha = emit->alpha(rec.u, rec.v, rec.p);
    // bool aligned = dot(rec.normal, r_in.direction()) > 0;
    // // this if condition handles alpha and one-sided visibility
    // if (alpha == 0.0 || random_double() > alpha || (aligned && !two_sided))
    // {
    //     // passthrough ray,
    //     // because the random was above the alpha threshold,
    //     // we were at 0 alpha,
    //     // or the hit occurred from behind the light

    //     // set ray passthrough to right after the ray intersection point and in original ray direction.
    //     scattered = ray(rec.p + r_in.direction() * 0.01, r_in.direction());
    //     // set attenuation to 1 (meaning no change)
    //     attenuation = vec3(1.0, 1.0, 1.0); //emit->value(rec.u, rec.v, rec.p);
    //     return true;
    // }
    // return false;
    // }
    virtual vec3 emitted(const ray &r_in, const hit_record &rec, float u, float v, const vec3 &p) const
    {
        // this if condition handles one-sidedness
        bool aligned = dot(rec.normal, r_in.direction()) > 0;
        if (!aligned || two_sided)
        {
            // if normal and r_in.direction are in opposite directions
            // i.e. if hit on correct side of light
            return power * emit->value(u, v, p) * emit->alpha(u, v, p);
        }
        else
        {
            // aligned && !two_sided
            // opposite is !(aligned && !two_sided)
            //           = !aligned || two_sided
            // normal and r_in.direction are aligned, pointing the same way
            // this means that we hit from the wrong side and that we're not two sided
            return vec3(0, 0, 0);
        }
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
    float power;
    bool two_sided;
};

class isotropic : public material
{
public:
    isotropic(texture *a, vec3 emission = vec3(0, 0, 0), std::string name = "isotropic") : albedo(a), emission(emission), name(name) {}
    isotropic(vec3 a, vec3 emission = vec3(0, 0, 0), std::string name = "isotropic") : albedo(new constant_texture(a)), emission(emission), name(name) {}
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
