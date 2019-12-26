#pragma once
#include "vec3.h"
#include "world.h"
#include "ray.h"
#include "helpers.h"
#include "hittable.h"
#include "pdf.h"

vec3 sample_random_light(const ray &r, world *world, const hit_record &rec, const ray &scattered, float scatter_pdf_s, float &light_pdf_s, float &inv_weight_s)
{
    hittable *random_light = world->get_random_light();
    vec3 _color = vec3(0, 0, 0);
    hittable_pdf l_pdf(random_light, rec.p);

    ray light_ray = ray(rec.p, l_pdf.generate(), r.time());

    // pdf of light ray having gone directly towards light
    float light_pdf_l = l_pdf.value(light_ray.direction());
    // pdf of scatter having gone directly towards light
    float scatter_pdf_l = rec.mat_ptr->value(r, rec, light_ray.direction());

    // pdf of light ray having been generated from scatter
    light_pdf_s = l_pdf.value(scattered.direction());
    // pdf of scattered ray having been generated from scatter

    // float mix_l = (scatter_pdf_l + light_pdf_l) / 2.0f;
    // float mix_s = (scatter_pdf_s + light_pdf_s) / 2.0f;

    float weight_l = power_heuristic(1.0f, light_pdf_l, 1.0f, scatter_pdf_l);
    // float inv_weight_l = 1.0f - weight_l;
    // float cos_l = fabs(dot(light_ray.direction(), rec.normal));

    float weight_s = power_heuristic(1.0f, light_pdf_s, 1.0f, scatter_pdf_s);
    inv_weight_s = 1.0f - weight_s;
    // float cos_s = fabs(dot(scattered.direction(), rec.normal));

    // add contribution from next event estimation
    hit_record light_rec;
    // basically a shadow ray
    if (world->hit(light_ray, 0.0, 1.0, light_rec))
    {
        // if (light_rec.mat_ptr->name == "diffuse_light")
        // {
        _color += 1.0f * weight_l / light_pdf_l * light_rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        // }
    }
    return _color;
}

// iterative is more suited for optimization, and possible gpu execution
vec3 iterative_color(ray &r, world *world, int depth, int max_bounces, long *bounce_count, path *_path)
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
            emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
            if (rec.mat_ptr->scatter(r, rec, attenuation))
            {
                assert(!is_nan(r.time()));

                _color += beta * emitted;
                assert(!is_nan(_color));
                float light_pdf_s, inv_weight_s;
                scattered = ray(rec.p, rec.mat_ptr->generate(r, rec), r.time());
                float scatter_pdf_s = rec.mat_ptr->value(r, rec, scattered.direction());
                _color += beta * sample_random_light(r, world, rec, scattered, scatter_pdf_s, light_pdf_s, inv_weight_s);
                assert(!is_nan(_color));
                // flat out skip bounces that would hit the light directly
                if (light_pdf_s > 0)
                {
                    break;
                }
                if (!world->config.value("only_direct_illumination", false))
                {
                    // multiply beta to account for next and future bounces
                    beta *= inv_weight_s / scatter_pdf_s;
                    (*bounce_count)++;
                    beta *= attenuation;
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
