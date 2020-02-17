#pragma once
#include "vec3.h"
#include "ray.h"
#include "hittable.h"
#include "world.h"
#include "types.h"
#include "material.h"

class Integrator
{
    virtual vec3 color(const ray &r, World *world, int depth, int max_bounces, long *bounce_count, path *_path) = 0;
};

class RecursivePT : public Integrator
{
public:
    // reuse this for branched path tracing
    vec3 color(const ray &r, World *world, int depth, int max_bounces, long *bounce_count, path *_path)
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
            vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
            if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            {
                (*bounce_count)++;
                return emitted + attenuation * this->color(scattered, world, depth + 1, max_bounces, bounce_count, _path);
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
            // world background color here
            // vec3 unit_direction = unit_vector(r.direction());
            // float t = 0.5 * (unit_direction.y() + 1.0);
            // return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.2, 0.1, 0.7);

            // generate world u v and then sample world texture?
            // return vec3(0, 0, 0);
            vec3 unit_direction = unit_vector(r.direction());
            float u = unit_direction.x();
            float v = unit_direction.y();
            // TODO: replace u and v with angle l->r and angle d->u;
            return world->value(u, v, unit_direction);
        }
    }
};

class IterativePT : public Integrator
{
public:
    vec3 color(ray &r, World *world, int depth, int max_bounces, long *bounce_count, path *_path)
    {
        hit_record rec;
        vec3 _color = vec3(0, 0, 0);
        ray scattered;
        vec3 attenuation = vec3(0, 0, 0);
        vec3 emitted = vec3(0, 0, 0);

        vec3 beta = vec3(1.0, 1.0, 1.0);
        for (int i = 0; i < max_bounces; i++)
        {
            if (_path != nullptr)
            {
                _path->push_back(rec.p);
            }
            if (world->hit(r, 0.001, MAXFLOAT, rec))
            {
                emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
                if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
                {
                    (*bounce_count)++;
                    _color += beta * emitted;
                    beta *= attenuation;
                    r = scattered;
                }
                else
                {
                    _color += beta * emitted;
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
                float u = unit_direction.x();
                float v = unit_direction.y();
                // TODO: replace u and v with angle l->r and angle d->u;
                _color += beta * world->value(u, v, unit_direction);
                break;
            }
        }
        return _color;
    }
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
