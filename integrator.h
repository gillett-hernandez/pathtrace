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
    Config config;
};

class RecursivePT : public Integrator
{
public:
    RecursivePT(int max_bounces, World *world) : max_bounces(max_bounces), world(world), config(world->config)
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
    Config config;
};

class NEERecursive : public Integrator
{
public:
    NEERecursive(int max_bounces, World *world) : max_bounces(max_bounces), world(world), config(world->config){};
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
                vec3 sum = vec3(0, 0, 0);
                // vec3 sum = vec3(0.0f, 0.0f, 0.0f);
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

                if (!config.only_direct_illumination)
                {
                    // add contribution from next and future bounces
                    vec3 fac = inv_weight_l * attenuation / scatter_pdf_l;
                    vec3 next_and_future_bounces = this->color(scattered, depth + 1, bounce_count, _path, true);
                    sum += fac * next_and_future_bounces;
                }

                vec3 light_hit = this->color(light_ray, depth + 1, bounce_count, nullptr, false);
                sum += weight_l * attenuation / light_pdf_l * light_hit;

                return sum;
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
    Config config;
};

class NEEIterative : public Integrator
{
public:
    NEEIterative(int max_bounces, World *world) : max_bounces(max_bounces), world(world), config(world->config){};
    // iterative is more suited for optimization, and possible gpu execution
    vec3 color(ray &r, int depth, long *bounce_count, path *_path, bool skip_light_hit = false)
    {
        hit_record rec;
        vec3 sum = vec3(0, 0, 0);
        ray scattered;
        vec3 attenuation = vec3(0, 0, 0);
        vec3 hit_emission = vec3(0, 0, 0);
        float last_bsdf_pdf = -1;

        vec3 beta = vec3(1.0, 1.0, 1.0);
        for (int i = 0; i < max_bounces; i++)
        {
            // do russian roulette path termination here? by checking beta?

            ASSERT(!is_nan(sum), "sum had nan components");
            ASSERT(!isinf(beta), "beta was inf");
            (*bounce_count)++;
            if (world->hit(r, 0.001, MAXFLOAT, rec))
            {
                if (_path != nullptr)
                {
                    _path->push_back(rec.p);
                }
                bool did_scatter = rec.mat_ptr->scatter(r, rec, attenuation);
                assert(!is_nan(r.time()));
                float cos_i = fabs(dot(r.direction().normalized(), rec.normal.normalized()));

                hit_emission = rec.mat_ptr->emitted(r, rec, rec.u, rec.v, rec.p);
                // if hit emission is greater than some small value
                if (hit_emission.squared_length() > 0.000001)
                {
                    if (last_bsdf_pdf <= 0)
                    {
                        sum += beta * hit_emission;
                    }
                    else
                    {
                        hittable_pdf this_pdf(rec.primitive, r.origin());
                        float weight = power_heuristic(1.0, last_bsdf_pdf, 1.0, this_pdf.value(rec.p));
                        sum += beta * hit_emission * weight;
                        ASSERT(!is_nan(sum), "sum had nan components");
                    }
                }

                vec3 light_contribution = vec3(0, 0, 0);
                for (int i = 0; i < config.light_samples; i++)
                {
                    hittable *random_light = world->get_random_light();
                    float pick_pdf = world->lights.size();
                    hittable_pdf l_pdf(random_light, rec.p);
                    // pdf scatter_pdf;

                    // rec.mat_ptr->get_pdf(*scatter_pdf, r, rec);
                    // mixture_pdf p(&p0, &p1);
                    // hittable_pdf p = p0;
                    // cosine_pdf p = p1;
                    // mixture_pdf p(&p0, rec.mat_ptr->pdf);
                    // cosine of incoming ray

                    ray light_ray = ray(rec.p, l_pdf.generate(), r.time());
                    float cos_l = dot(light_ray.direction().normalized(), rec.normal.normalized());

                    // vec3 sum = vec3(0.0f, 0.0f, 0.0f);
                    // pdf of light ray having gone directly towards light
                    float light_pdf_l = l_pdf.value(light_ray.direction());
                    float scatter_pdf_l = rec.mat_ptr->value(r, rec, light_ray.direction());
                    float weight_l = power_heuristic(1.0f, light_pdf_l, 1.0f, scatter_pdf_l);
                    float inv_weight_l = 1.0f - weight_l;

                    hit_record light_rec;
                    bool did_light_hit = world->hit(light_ray, 0.001, MAXFLOAT, light_rec);
                    (*bounce_count)++;
                    if (did_light_hit && attenuation.length() > 0.0001)
                    {
                        if (true || light_rec.primitive == random_light)
                        {
                            vec3 light_emission = light_rec.mat_ptr->emitted(light_ray, light_rec, light_rec.u, light_rec.v, light_rec.p);
                            float dropoff = fmax(cos_l, 0.0);
                            vec3 contribution = attenuation * beta * weight_l / light_pdf_l * dropoff * light_emission / pick_pdf;
                            if (is_nan(contribution))
                            {
                                // likely nan because what was hit by `r` was the same object as what was hit by light_ray
                            }
                            else
                            {
                                light_contribution += contribution;
                            }
                            ASSERT(!is_nan(sum), "sum had nan components");
                        }
                    }
                }

                sum += light_contribution / config.light_samples;
                ASSERT(!is_nan(sum), "sum had nan components: " << sum << ", and light contrib: " << light_contribution);

                if (did_scatter)
                {

                    ray scattered = ray(rec.p + config.normal_offset * rec.normal, rec.mat_ptr->generate(r, rec), r.time());

                    // float light_pdf_s = l_pdf.value(scattered.direction());
                    // pdf of scattered ray having been generated from scatter
                    float scatter_pdf_s = rec.mat_ptr->value(r, rec, scattered.direction());

                    // cosine direction
                    // float cos_s = fabs(dot(scattered.direction(), rec.normal)) / scattered.direction().length();

                    // MIS weighted contribtuion of
                    // add contribution from next event estimation

                    float p = std::max(beta.x(), std::max(beta.y(), beta.z()));
                    if (config.russian_roulette && p <= 1 && 0.001 < p)
                    {
                        if (random_double() > p)
                        {
                            break;
                        }

                        // Add the energy we 'lose' by randomly terminating paths
                        beta *= 1 / p;
                        ASSERT(!isinf(beta), "beta was inf");
                    }

                    if (!config.only_direct_illumination)
                    {
                        if (scatter_pdf_s < 0.0000001)
                        {
                            break;
                        }
                        beta *= attenuation * fabs(cos_i) / scatter_pdf_s;
                        ASSERT(!isinf(beta), "beta was inf " << beta << "  " << attenuation << "  " << cos_i << "  " << scatter_pdf_s);
                        ASSERT(!is_nan(beta), beta << " " << attenuation << " " << cos_i << " " << scatter_pdf_s);
                        last_bsdf_pdf = scatter_pdf_s;
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
                    sum += beta * hit_emission;
                    ASSERT(!is_nan(sum), "sum had nan components, beta was " << beta << " and sum was " << sum);

                    break;
                }
            }
            else
            {
                vec3 unit_direction = unit_vector(r.direction());
                // get phi and theta values for that direction, then convert to UV values for an environment map.
                float u = (M_PI + atan2(unit_direction.y(), unit_direction.x())) / TAU;
                float v = acos(unit_direction.z()) / M_PI;

                sum += beta * world->value(u, v, unit_direction);
                ASSERT(!is_nan(sum), "sum had nan components, beta was " << beta << ", sum was " << sum << ", and world value was " << world->value(u, v, unit_direction));

                break;
            }
        }
        return sum;
    }
    int max_bounces;
    World *world;
    Config config;
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
