#pragma once
#include "camera.h"
#include "ray.h"
#include "thirdparty/json.hpp"
#include "vec3.h"
#include "types.h"
#include "integrator.h"
#include "config.h"
#include "tonemap.h"
#include <chrono>
#include <thread>
#include <fstream>
#include <mutex>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

void output_to_file(std::shared_ptr<std::ofstream> output, vec3 **buffer, int width, int height, int samples, float max_luminance, float exposure, float gamma)
{
    output->seekp(0);
    // output file
    (*output) << "P6\n"
              << width << " " << height << "\n255\n";
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = buffer[j][i];
            col /= float(samples);

            col *= 16 + exposure;
            // color space interpolation here
            // first color mapping

            // col = to_srgb(tonemap_uncharted(col, max_luminance));
            col = to_srgb(tonemap_2(col));
            // put gamma and exposure here

            char ir = int(255.99 * powf(col[0], gamma));
            char ig = int(255.99 * powf(col[1], gamma));
            char ib = int(255.99 * powf(col[2], gamma));
            (*output) << ir << ig << ib;
        }
    }
    output->flush();
}

void print_out_progress(long num_samples_done, long num_samples_left, std::chrono::high_resolution_clock::time_point start_time)
{
    auto intermediate = std::chrono::high_resolution_clock::now();
    // rate was in rays per nanosecond
    // first multiply by 1 billion to get rays per second
    assert(num_samples_left >= 0);
    assert(num_samples_done >= 0);
    float rate = 1000000000 * num_samples_done / (intermediate - start_time).count();
    assert(rate >= 0);

    std::cout << "samples left" << std::setw(20) << num_samples_left
              << " rate " << std::setw(10) << rate
              << " time left " << std::setw(5) << num_samples_left / rate
              << "                                           " << '\r' << std::flush;
}

class Renderer
{
public:
    Renderer()
    {
        this->output = std::make_shared<std::ofstream>(std::ofstream("trash.txt"));
    };
    Renderer(Integrator *integrator, camera cam, World *world)
    {
        std::cout << "complex constructor called" << std::endl;
        this->config = world->config;
        this->film = this->config.film;
        this->cam = cam;
        this->integrator = integrator;
        this->world = world;
        this->output = std::make_shared<std::ofstream>(std::ofstream(config.ppm_output_path));
        ;

        // create framebuffer
        framebuffer = new vec3 *[film.height];
        for (int j = film.height - 1; j >= 0; j--)
        {
            framebuffer[j] = new vec3[film.width];
            for (int i = 0; i < film.width; i++)
            {
                framebuffer[j][i] = vec3(0, 0, 0);
            }
        }
    };
    virtual void preprocess() = 0;
    virtual void start_render(std::chrono::high_resolution_clock::time_point) = 0;
    virtual void next_pixel_and_ray(int thread_id, ray &ray, int x, int y) = 0;
    virtual void sync_progress() = 0;
    virtual bool is_done() = 0;
    virtual void compute(int thread_id, int samples) = 0;
    virtual void finalize() = 0;
    vec3 **framebuffer;
    std::mutex framebuffer_lock;
    std::chrono::high_resolution_clock::time_point render_start_time;
    bool completed;
    Integrator *integrator;
    camera cam;
    World *world;
    Config config;
    s_film film;
    std::shared_ptr<std::ofstream> output;
};

class Progressive : public Renderer
{
public:
    Progressive(Integrator *integrator, camera cam, World *world) : Renderer{integrator, cam, world}
    {
        trace_probability = config.trace_probability;
        N_THREADS = config.threads;
        completed = false;
    };
    void preprocess(){};
    void start_render(std::chrono::high_resolution_clock::time_point program_start_time)
    {
        threads = new std::thread[N_THREADS];
        int min_samples = config.samples / N_THREADS;
        int remaining_samples = config.samples % N_THREADS;

        std::cout << "samples per thread " << min_samples << std::endl;
        std::cout << "leftover samples to be allocated " << remaining_samples << std::endl;
        bounce_counts = new long[N_THREADS];
        samples_done = new int[N_THREADS];

        for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
        {
            bounce_counts[thread_id] = 0;
            samples_done[thread_id] = 0;
        }

        // create N_THREAD buckets to dump paths into.

        array_of_paths = new paths[N_THREADS];
        if (trace_probability > 0.0)
        {
            int avg_number_of_paths = config.avg_number_of_paths;
            for (int t = 0; t < N_THREADS; t++)
            {
                array_of_paths[t] = paths();
                array_of_paths[t].reserve(avg_number_of_paths / N_THREADS);
                for (int i = 0; i < avg_number_of_paths / N_THREADS; i++)
                {
                    array_of_paths[t].push_back(new path());
                    array_of_paths[t].back()->reserve(config.max_bounces / 2);
                }
            }
        }

        std::cout << "spawning threads";
        for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
        {
            int samples = min_samples + (int)(thread_id < remaining_samples);
            std::cout << '.';
            // wrap framebuffer in a std::ref since it needs to be modified

            threads[thread_id] = std::thread([this](int thread_id, int samples) { compute(thread_id, samples); }, thread_id, samples); //, std::ref(framebuffer));
            // threads[thread_id] = std::thread(this->compute, thread_id, std::ref(framebuffer));
        }
        std::cout << " done.\n";
        render_start_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds2 = render_start_time - program_start_time;
        std::cout << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl;
        std::cout << "joining threads\n";
    };
    void next_pixel_and_ray(int thread_id, ray &ray, int x, int y){};
    void sync_progress() override
    {
        long num_samples_done = 0;
        long min_camera_rays = config.samples * film.total_pixels;
        for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
        {
            assert(samples_done[thread_id] >= 0);
            num_samples_done += samples_done[thread_id];
        }
        long num_samples_left = min_camera_rays - num_samples_done;
        print_out_progress(num_samples_done, num_samples_left, render_start_time);
        float avg_luminance, max_luminance, total_luminance;
        calculate_luminance(framebuffer, film.width, film.height, num_samples_done / (film.width * film.height), film.width * film.height, max_luminance, total_luminance, avg_luminance);
        output_to_file(output, framebuffer, film.width, film.height, num_samples_done / (film.width * film.height), max_luminance, film.exposure, film.gamma);
        completed = num_samples_left <= 0;
    };

    bool is_done()
    {
        return this->completed;
    }
    void compute(int thread_id, int samples)
    {
        // start of multithreaded code.
        if (samples == 0)
        {
            return;
        }
        int traces = 0;
        for (int s = 0; s < samples; s++)
        {
            for (int j = film.height - 1; j >= 0; j--)
            {
                // std::cout << "computing row " << j << std::endl;
                for (int i = 0; i < film.width; i++)
                {
                    // std::cout << "computing column " << i << std::endl;
                    vec3 col = vec3(0, 0, 0);
                    long *count = new long(0);

                    float u = float(i + random_double()) / float(film.width);
                    float v = float(j + random_double()) / float(film.height);
                    ray r = cam.get_ray(u, v);
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
                    col += de_nan(integrator->color(r, world, 0, count, _path));
                    if (_path != nullptr)
                    {
                        // std::cout << "traced _path, size is " << _path->size() << std::endl;
                        if (traces > array_of_paths[thread_id].size())
                        {
                            array_of_paths[thread_id].push_back(_path);
                        }
                    }
                    // framebuffer accesses need to be guarded with a lock so that multiple threads don't write to the same pixel at the same time.
                    framebuffer_lock.lock();
                    framebuffer[j][i] += col;
                    bounce_counts[thread_id] += *count;
                    samples_done[thread_id] += 1;
                    framebuffer_lock.unlock();
                }
            }
        }
        // std::cout << "total length of traced paths : " << paths[thread_id].size() << std::endl;
    }

    void finalize()
    {
        long total_bounces = 0;
        for (int thread_id = 0; thread_id < config.threads; thread_id++)
        {
            total_bounces += bounce_counts[thread_id];
        }
        auto t4 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds3 = t4 - render_start_time;
        std::cout << "time taken to compute " << elapsed_seconds3.count() << std::endl;
        float rate1 = film.total_pixels * config.samples / elapsed_seconds3.count();
        float rate2 = total_bounces / elapsed_seconds3.count();
        std::cout << "computed " << film.total_pixels * config.samples << " camera rays in " << elapsed_seconds3.count() << "s, at " << rate1 << " rays per second, or " << rate1 / N_THREADS << "per thread" << std::endl;
        std::cout << "computed " << total_bounces << " rays, at " << rate2 << " rays per second, or " << rate2 / N_THREADS << " per thread" << std::endl;

        int added_paths = 0;
        if (config.should_trace_paths)
        {
            std::ofstream traced_paths_output(config.traced_paths_output_path);
            std::ofstream traced_paths_output2d(config.traced_paths_2d_output_path);
            for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
            {
                auto paths = array_of_paths[thread_id];

                // added_paths += paths.size();

                for (auto &path : paths)
                {
                    if (path->size() == 0)
                    {
                        continue;
                    }
                    added_paths++;
                    for (auto &point : *path)
                    {
                        traced_paths_output << point.x() << ',' << point.y() << ',' << point.z() << '\n';

                        float x = 0;
                        float y = 0;
                        bool hit_scene = cam.project(point, x, y);
                        if (0.0 < x && 0.0 < y && x <= 1.0 && y <= 1.0 && hit_scene)
                        {
                            traced_paths_output2d << x << ',' << y << std::endl;
                        }
                        if (!hit_scene)
                        {
                            traced_paths_output2d << x << ',' << y << '!' << std::endl;
                        }
                    }
                    traced_paths_output << std::endl;
                    traced_paths_output2d << std::endl;
                }
            }
            traced_paths_output.close();
            traced_paths_output2d.close();
        }
        // assert(added_paths > 0 || !config.should_trace_paths);
        if (added_paths == 0 && config.should_trace_paths)
        {
            std::cout << "should have added some amount of paths but didn't" << std::endl;
        }
        std::cout << "added " << added_paths << " paths" << std::endl;

        float max_luminance, avg_luminance, total_luminance;
        calculate_luminance(framebuffer, film.width, film.height, config.samples, film.width * film.height, max_luminance, total_luminance, avg_luminance);
        std::cout << "avg lum " << avg_luminance << std::endl;
        std::cout << "max lum " << max_luminance << std::endl;

        output_to_file(output, framebuffer, film.width, film.height, config.samples, max_luminance, film.exposure, film.gamma);
    }

    int N_THREADS;
    paths *array_of_paths;
    std::thread *threads;
    long *bounce_counts;
    int *samples_done;
    float trace_probability;
};

// class naive : public Renderer
// {
//     void preprocess(){};
//     void start_render(){};
//     void next_pixel_and_ray(int thread_id, ray &ray, int x, int y){};
//     void compute(int thread_id, long *ray_ct, int *completed_samples, vec3 **buffer, int samples, int max_bounces, float trace_probability, paths *array_of_paths)
//     {
//         if (samples == 0)
//         {
//             return;
//         }
//         int traces = 0;
//         for (int j = height - 1; j >= 0; j--)
//         {
//             // std::cout << "computing row " << j << std::endl;
//             for (int i = 0; i < film.width; i++)
//             {
//                 // std::cout << "computing column " << i << std::endl;
//                 vec3 col = vec3(0, 0, 0);
//                 long *count = new long(0);

//                 for (int s = 0; s < samples; s++)
//                 {
//                     float u = float(i + random_double()) / float(width);
//                     float v = float(j + random_double()) / float(height);
//                     ray r = cam.get_ray(u, v);
//                     std::vector<vec3> *_path = nullptr;
//                     if (random_double() < trace_probability)
//                     {
//                         // std::cout << "creating path to trace" << std::endl;
//                         // path = new std::vector<vec3>();
//                         if (traces < array_of_paths[thread_id].size())
//                         {
//                             _path = array_of_paths[thread_id][traces];
//                             assert(_path->size() == 0);
//                         }
//                         else
//                         {
//                             _path = new path();
//                         }
//                         traces++;
//                     }
//                     else
//                     {
//                         _path = nullptr;
//                     }
//                     col += de_nan(integrator->color(r, world, 0, max_bounces, count, _path));
//                     if (_path != nullptr)
//                     {
//                         // std::cout << "traced _path, size is " << _path->size() << std::endl;
//                         if (traces > array_of_paths[thread_id].size())
//                         {
//                             array_of_paths[thread_id].push_back(_path);
//                         }
//                     }
//                 }

//                 framebuffer_lock.lock();
//                 buffer[j][i] += col;
//                 *ray_ct += *count;
//                 completed_samples[thread_id] += samples;
//                 framebuffer_lock.unlock();
//             }
//         }
//     }
// };

// class tiled : public renderer
// {
// public:
//     tiled(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
//     {
//         j = height - 1;
//         i = 0;
//         s = samples;
//     }
//     ray get_ray()
//     {
//         float u = float(i + random_double()) / float(width);
//         float v = float(j + random_double()) / float(height);
//         ray r = cam->get_ray(u, v);
//         return r;
//     }
//     bool advance()
//     {

//         s--;
//         if (s < 0)
//         {
//             s = samples;
//             i++;
//         }
//         if (i >= width)
//         {
//             i = 0;
//             j--;
//         }
//         if (j < 0)
//         {
//             j = height - 1;
//         }
//     }
//     void record(vec3 color)
//     {
//         if (s > 0)
//         {
//             color_buffer += color;
//         }
//         else
//         {
//             framebuffer[j][i] += color_buffer;
//         }
//     }
//     camera *cam;
//     int width, height, samples;

// private:
//     // the framebuffer for the screen
//     vec3 **framebuffer;
//     // the color buffer, a single pixel's value is stored here before being inserted into the framebuffer
//     vec3 color_buffer;
//     int i, j, s;
// };
