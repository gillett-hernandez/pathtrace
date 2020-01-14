#pragma once

#include "camera.h"
#include "world.h"
#include "ray.h"
#include "thirdparty/json.hpp"
#include "vec3.h"
#include <thread>
#include <mutex>

using json = nlohmann::json;

class manager
{
public:
    // // should return the next ray to fire into the scene
    // virtual ray next() = 0;
    // // should record color data onto film
    // virtual void record(vec3 color) = 0;
    // // should advance internal data and return true if done
    // virtual bool advance() = 0;
    // report_progress should return a true when done.
    virtual void start() = 0;
    virtual bool report_progress() = 0;
    virtual ray get_ray(int i, int j)
    {
        float u = float(i + random_double()) / float(width);
        float v = float(j + random_double()) / float(height);
        ray r = cam->get_ray(u, v);
        return r;
    }
    world *world;
    camera *cam;
    int width, height;
    json config;
    vec3 **framebuffer;
};

class progressive : public manager
{
public:
    progressive(world *world, camera *cam, vec3 **framebuffer, json config, int width, int height, int samples) : world(world), cam(cam), framebuffer(framebuffer), config(config), width(width), height(height), samples(samples)
    {
        // j = height - 1;
        // i = 0;
        // s = samples;
    }

    void start()
    {
        std::cout << "spawning threads";
        for (int t = 0; t < N_THREADS; t++)
        {
            bounce_counts[t] = 0;
            // int samples = min_samples + (int)(t < remaining_samples);
            bounce_counts[t] = 0;
            samples_done[t] = 0;
            std::cout << ' ' << samples;
            threads[t] = std::thread(this->execute, t, &bounce_counts[t], samples_done, std::ref(framebuffer), width, height, MAX_BOUNCES, cam, world, trace_probability, array_of_paths);
        }
        std::cout << " done.\n";
    }
    bool report_progress()
    {
    }
    void execute(int thread_id, long *ray_ct, int *completed_samples, vec3 **buffer, int max_bounces, world *world, float trace_probability, paths *array_of_paths)
    {
        if (samples == 0)
        {
            return;
        }
        int traces = 0;
        for (int j = height - 1; j >= 0; j--)
        {
            // std::cout << "computing row " << j << std::endl;
            for (int i = 0; i < width; i++)
            {
                // std::cout << "computing column " << i << std::endl;
                vec3 col = vec3(0, 0, 0);
                long *count = new long(0);

                ray r = this->get_ray(i, j);
                std::vector<vec3> *_path = nullptr;
                if (random_double() < trace_probability)
                {
                    // std::cout << "creating path to trace" << std::endl;
                    // path = new std::vector<vec3>();
                    if (traces < array_of_paths[thread_id].size())
                    {
                        _path = array_of_paths[thread_id][traces];
                    }
                    else
                    {
                        _path = new path();
                    }
                    traces++;
                }
                else
                {
                    _path = nullptr;
                }
                col += de_nan(iterative_color(r, world, 0, max_bounces, count, _path));
                if (_path != nullptr)
                {
                    // std::cout << "traced _path, size is " << _path->size() << std::endl;
                    if (traces > array_of_paths[thread_id].size())
                    {
                        array_of_paths[thread_id].push_back(_path);
                    }
                }

                framebuffer_lock.lock();
                buffer[j][i] += col;
                *ray_ct += *count;
                completed_samples[thread_id] += 1;
                framebuffer_lock.unlock();
            }
        }
    }
    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
};

class naive : public manager
{
public:
    naive(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
    {
    }
    bool advance()
    {
    }
    void record(vec3 color)
    {
        if (s > 0)
        {
            color_buffer += color;
        }
        else
        {
            framebuffer[j][i] += color_buffer;
        }
    }
    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
};

class tiled : public manager
{
public:
    tiled(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
    {
    }

    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
};
