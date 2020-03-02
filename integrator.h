#pragma once
#include "vec3.h"
#include "ray.h"
#include "hittable.h"
#include "world.h"
#include "types.h"
#include "material.h"
#include "helpers.h"
#include "pdf.h"

class Integrator
{
public:
    virtual vec3 color(ray &r, int depth, long *bounce_count, path *_path, bool skip_light_hit = false) = 0;
    int max_bounces;
    World *world;
};

class RecursivePT : public Integrator
{
public:
    RecursivePT(int max_bounces, World *world) : max_bounces(max_bounces), world(world)
    {
        assert(this->max_bounces > 0);
        std::cout << "complex constructor called for recursivePT" << std::endl;
    };
    // reuse this for branched path tracing
    vec3 color(ray &r, int depth, long *bounce_count, path *_path, bool skip_light_hit)
    {
        hit_record rec;
        if (world->hit(r, 0.001, MAXFLOAT, rec))
        {
            if (_path != nullptr)
            {
                _path->push_back(rec.p);
            }
            ray scattered;
            vec3 attenuation;
            vec3 emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
            // if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation))
            {
                scattered = ray(rec.p, rec.mat_ptr->generate(r, rec));
                (*bounce_count)++;
                vec3 subcall = this->color(scattered, depth + 1, bounce_count, _path, skip_light_hit);
                assert(!is_nan(subcall));
                assert(!is_nan(emitted));
                assert(!is_nan(attenuation));
                return emitted + attenuation * subcall;
            }
            else
            {
                return emitted;
            }
        }
        else
        {
            if (_path != nullptr)
            {
                _path->push_back(rec.p);
            }
            // generate world u v and then sample world texture?
            // return vec3(0, 0, 0);
            vec3 unit_direction = unit_vector(r.direction());
            // get phi and theta values for that direction, then convert to UV values for an environment map.
            float u = (M_PI + atan2(unit_direction.y(), unit_direction.x())) / TAU;
            float v = acos(unit_direction.z()) / M_PI;

            return world->value(u, v, unit_direction);
        }
    }
    int max_bounces;
    World *world;
};

/* class IterativePT : public Integrator
// {
// public:
//     vec3 color(ray &r,  int depth, long *bounce_count, path *_path)
//     {
//         hit_record rec;
//         vec3 _color = vec3(0, 0, 0);
//         ray scattered;
//         vec3 attenuation = vec3(0, 0, 0);
//         vec3 emitted = vec3(0, 0, 0);

//         vec3 beta = vec3(1.0, 1.0, 1.0);
//         for (int i = 0; i < max_bounces; i++)
//         {
//             if (_path != nullptr)
//             {
//                 _path->push_back(rec.p);
//             }
//             if (world->hit(r, 0.001, MAXFLOAT, rec))
//             {
//                 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
//                 if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
//                 {
//                     (*bounce_count)++;
//                     _color += beta * emitted;
//                     beta *= attenuation;
//                     r = scattered;
//                 }
//                 else
//                 {
//                     _color += beta * emitted;
//                     break;
//                 }
//             }
//             else
//             {
//                 // world background color here
//                 // vec3 unit_direction = unit_vector(r.direction());
//                 // float t = 0.5 * (unit_direction.y() + 1.0);
//                 // return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.2, 0.1, 0.7);

//                 // generate world u v and then sample world texture?
//                 // return vec3(0, 0, 0);
//                 vec3 unit_direction = unit_vector(r.direction());
//                 float u = unit_direction.x();
//                 float v = unit_direction.y();
//                 // TODO: replace u and v with angle l->r and angle d->u;
//                 _color += beta * world->value(u, v, unit_direction);
//                 break;
//             }
//         }
//         return _color;
//     }
// }; */

class NEERecursive : public Integrator
{
public:
    NEERecursive(int max_bounces, World *world) : max_bounces(max_bounces), world(world){};
    vec3 color(ray &r, int depth, long *bounce_count, path *_path, bool skip_light_hit = false)
    {
        hit_record rec;
        // assert non-nan time
        assert(!is_nan(r.time()));

        if (_path != nullptr)
        {
            _path->push_back(rec.p);
        }
        if (world->hit(r, 0.001f, MAXFLOAT, rec))
        {
            vec3 attenuation;
            vec3 emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
            if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation))
            {
                if (rec.mat_ptr->name == "diffuse_light" && skip_light_hit)
                {
                    return vec3(0, 0, 0);
                }
                // assert non-nan time
                assert(!is_nan(r.time()));
                (*bounce_count)++;
                hittable *random_light = world->get_random_light();
                hittable_pdf l_pdf(random_light, rec.p);
                // pdf scatter_pdf;

                ray light_ray = ray(rec.p, l_pdf.generate(), r.time());
                ray scattered = ray(rec.p + 0.001 * rec.normal, rec.mat_ptr->generate(r, rec), r.time());
                // float weight;
                vec3 _color = vec3(0, 0, 0);
                // vec3 _color = vec3(0.0f, 0.0f, 0.0f);
                // cosine of incoming ray
                float cos_i = fabs(dot(r.direction(), rec.normal));
                // pdf of light ray having gone directly towards light
                float light_pdf_l = l_pdf.value(light_ray.direction());
                // pdf of scatter having gone directly towards light
                float scatter_pdf_l = rec.mat_ptr->value(r, rec, light_ray.direction());

                // pdf of light ray having been generated from scatter
                // float light_pdf_s = l_pdf.value(scattered.direction());
                // pdf of scattered ray having been generated from scatter
                // float scatter_pdf_s = rec.mat_ptr->value(r, rec, scattered.direction());

                float weight_l = power_heuristic(1.0f, light_pdf_l, 1.0f, scatter_pdf_l);
                float inv_weight_l = 1.0f - weight_l;
                float cos_l = fabs(dot(light_ray.direction(), rec.normal));

                // cosine direction
                float cos_s = fabs(dot(scattered.direction(), rec.normal));

                // MIS weighted contribtuion of
                // add contribution from next event estimation

                if (!world->config.only_direct_illumination)
                {
                    // add contribution from next and future bounces
                    vec3 fac = inv_weight_l * attenuation / scatter_pdf_l;
                    vec3 next_and_future_bounces = this->color(scattered, depth + 1, bounce_count, _path, true);
                    _color += fac * next_and_future_bounces;
                }

                vec3 light_hit = this->color(light_ray, depth + 1, bounce_count, nullptr, false);
                _color += weight_l * attenuation / light_pdf_l * light_hit;

                return _color;
            }
            else
            {
                if (skip_light_hit && rec.mat_ptr->name == "diffuse_light")
                {
                    return vec3(0, 0, 0);
                }
                return emitted;
            }
        }
        else
        {
            vec3 unit_direction = unit_vector(r.direction());
            float u = atan(unit_direction.z() / unit_direction.x());
            float v = acos(unit_direction.y());
            return world->value(u, v, unit_direction);
        }
    }
    int max_bounces;
    World *world;
};

class NEEIterative : public Integrator
{
public:
    NEEIterative(int max_bounces, World *world) : max_bounces(max_bounces), world(world){};
    // iterative is more suited for optimization, and possible gpu execution
    vec3 color(ray &r, int depth, long *bounce_count, path *_path, bool skip_light_hit = false)
    {
        hit_record rec;
        vec3 _color = vec3(0, 0, 0);
        ray scattered;
        vec3 attenuation = vec3(0, 0, 0);
        vec3 emitted = vec3(0, 0, 0);
        if (_path != nullptr)
        {
            _path->push_back(rec.p);
        }
        vec3 beta = vec3(1.0, 1.0, 1.0);
        for (int i = 0; i < max_bounces; i++)
        {
            if (beta.squared_length() < 0.00001)
            {
                // terminate iteration of beta becomes too small
                break;
            }
            assert(!is_nan(_color));
            if (world->hit(r, 0.001, MAXFLOAT, rec))
            {
                emitted = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
                if (rec.mat_ptr->scatter(r, rec, attenuation))
                {
                    assert(!is_nan(r.time()));

                    _color += beta * emitted;
                    assert(!is_nan(r.time()));
                    (*bounce_count)++;
                    hittable *random_light = world->get_random_light();
                    hittable_pdf l_pdf(random_light, rec.p);
                    // pdf scatter_pdf;

                    // rec.mat_ptr->get_pdf(*scatter_pdf, r, rec);
                    // mixture_pdf p(&p0, &p1);
                    // hittable_pdf p = p0;
                    // cosine_pdf p = p1;
                    // mixture_pdf p(&p0, rec.mat_ptr->pdf);
                    ray light_ray = ray(rec.p, l_pdf.generate(), r.time());
                    ray scattered = ray(rec.p + 0.001 * rec.normal, rec.mat_ptr->generate(r, rec), r.time());
                    // float weight;
                    vec3 _color = vec3(0, 0, 0);
                    // vec3 _color = vec3(0.0f, 0.0f, 0.0f);
                    // cosine of incoming ray
                    float cos_i = fabs(dot(r.direction(), rec.normal));
                    // pdf of light ray having gone directly towards light
                    float light_pdf_l = l_pdf.value(light_ray.direction());
                    // pdf of scatter having gone directly towards light
                    float scatter_pdf_l = rec.mat_ptr->value(r, rec, light_ray.direction());

                    // pdf of light ray having been generated from scatter
                    // float light_pdf_s = l_pdf.value(scattered.direction());
                    // pdf of scattered ray having been generated from scatter
                    // float scatter_pdf_s = rec.mat_ptr->value(r, rec, scattered.direction());

                    float weight_l = power_heuristic(1.0f, light_pdf_l, 1.0f, scatter_pdf_l);
                    float inv_weight_l = 1.0f - weight_l;
                    float cos_l = fabs(dot(light_ray.direction(), rec.normal));

                    // cosine direction
                    float cos_s = fabs(dot(scattered.direction(), rec.normal));

                    // MIS weighted contribtuion of
                    // add contribution from next event estimation

                    vec3 light_hit = this->color(light_ray, depth + 1, bounce_count, nullptr, false);
                    _color += weight_l * attenuation / light_pdf_l * light_hit;

                    // flat out skip bounces that would hit the light directly
                    if (!world->config.only_direct_illumination)
                    {
                        // multiply beta to account for next and future bounces
                        beta *= inv_weight_l * attenuation / scatter_pdf_l;
                        (*bounce_count)++;
                        // reassign r to continue bouncing.
                        r = scattered;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    _color += beta * emitted;
                    assert(!is_nan(_color));
                    break;
                }
            }
            else
            {
                // world background color here
                // vec3 unit_direction = unit_vector(r.direction());
                // float t = 0.5 * (unit_direction.y() + 1.0);
                // return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.2, 0.1, 0.7);

                // generate world u v and then sample world texture?
                // return vec3(0, 0, 0);
                vec3 unit_direction = unit_vector(r.direction());
                float u = atan(unit_direction.z() / unit_direction.x());
                float v = acos(unit_direction.y());
                // TODO: replace u and v with angle l->r and angle d->u;
                _color += beta * world->value(u, v, unit_direction);
                assert(!is_nan(_color));
                break;
            }
        }
        return _color;
    }
    int max_bounces;
    World *world;
};

// class BPT : public Integrator {

// };

// class BDPT : public Integrator {

// };

// class SPPM : public Integrator {

// };

// class VCM : public Integrator {

// };

// class MLT : public Integrator {

// };
