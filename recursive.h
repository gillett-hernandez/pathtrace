#pragma once
#include "vec3.h"
#include "helpers.h"
#include "world.h"
#include "ray.h"
#include "pdf.h"

// reuse this for branched path tracing
vec3 recursive_color(const ray &r, world *world, int depth, int max_bounces, long *bounce_count, path *_path)
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
        vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation))
        {
            // if (rec.mat_ptr->name == "diffuse_light")
            // {
            //     return vec3(0, 0, 0);
            // }
            // assert non-nan time
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
            ray scattered = ray(rec.p, rec.mat_ptr->generate(r, rec), r.time());
            // float weight;
            vec3 _color = emitted;
            // vec3 _color = vec3(0.0f, 0.0f, 0.0f);
            // pdf of light ray having gone directly towards light
            float light_pdf_l = l_pdf.value(light_ray.direction());
            // pdf of scatter having gone directly towards light
            float scatter_pdf_l = rec.mat_ptr->value(r, rec, light_ray.direction());

            // pdf of light ray having been generated from scatter
            float light_pdf_s = l_pdf.value(scattered.direction());
            // pdf of scattered ray having been generated from scatter
            float scatter_pdf_s = rec.mat_ptr->value(r, rec, scattered.direction());

            float mix_l = (scatter_pdf_l + light_pdf_l) / 2.0f;
            float mix_s = (scatter_pdf_s + light_pdf_s) / 2.0f;

            float weight_l = power_heuristic(1.0f, light_pdf_l, 1.0f, scatter_pdf_l);
            float inv_weight_l = 1.0f - weight_l;
            float cos_l = fabs(dot(light_ray.direction(), rec.normal));

            float weight_s = power_heuristic(1.0f, light_pdf_s, 1.0f, scatter_pdf_s);
            float inv_weight_s = 1.0f - weight_s;
            float cos_s = fabs(dot(scattered.direction(), rec.normal));

            // add contribution from next event estimation
            _color += 1.0f * attenuation * weight_l / light_pdf_l * recursive_color(light_ray, world, depth + 1, 0, bounce_count, nullptr);

            // flat out skip bounces that would hit the light directly
            if (light_pdf_s > 0)
            {
                return _color;
            }
            if (!world->config.value("only_direct_illumination", false))
            {
                // add contribution from next and future bounces
                _color += 1.0f * attenuation * inv_weight_s / scatter_pdf_s * recursive_color(scattered, world, depth + 1, max_bounces, bounce_count, _path);
            }

            return _color;
        }
        else
        {
            // if (max_bounces == 0)
            // {
            return emitted;
            // }
            // else
            // {
            // return vec3(0, 0, 0);
            // }
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
        return world->value(u, v, unit_direction);
    }
}
