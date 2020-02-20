#pragma once
#include "camera.h"
#include "ray.h"
#include "thirdparty/json.hpp"
#include "vec3.h"
#include "types.h"
#include "integrator.h"
#include "config.h"
#include <thread>
#include <mutex>

using json = nlohmann::json;

class Renderer
{
public:
    // Renderer() {};
    void initialize(Integrator *integrator, s_film film, camera cam, World *world)
    {
        film = film;
        cam = cam;
        integrator = integrator;
        world = world;

        // create framebuffer
        framebuffer = new vec3 *[film.height];
        for (int j = film.height - 1; j >= 0; j--)
        {
            // std::cout << "computed row " << j << std::endl;
            framebuffer[j] = new vec3[film.width];
            for (int i = 0; i < film.width; i++)
            {
                framebuffer[j][i] = vec3(0, 0, 0);
            }
        }
    };
    virtual void preprocess() = 0;
    virtual void start_render() = 0;
    virtual std::vector<std::string> progress() = 0;
    virtual void next_pixel_and_ray(int thread_id, ray &ray, int x, int y) = 0;
    virtual void sync_progress() = 0;
    virtual bool is_done() = 0;
    virtual void compute(int thread_id, long *ray_ct, int *completed_samples, vec3 **buffer, int samples, int max_bounces, float trace_probability, paths *array_of_paths) = 0;
    virtual void finalize() = 0;
    vec3 **framebuffer;
    std::mutex framebuffer_lock;
    Integrator *integrator;
    std::chrono::high_resolution_clock::time_point render_start_time;
    s_film film;
    camera cam;
    World *world;
};

class Progressive : public Renderer
{
public:
    Progressive(Integrator *integrator, camera cam, World *world){

    };
    void preprocess(){};
    void start_render(long n_samples, float trace_probability, int *bounce_counts, int MAX_BOUNCES, std::chrono::high_resolution_clock::time_point program_start_time)
    {
        threads = new std::thread[N_THREADS];
        int min_samples = n_samples / N_THREADS;
        int remaining_samples = n_samples % N_THREADS;

        std::cout << "samples per thread " << min_samples << std::endl;
        std::cout << "leftover samples to be allocated " << remaining_samples << std::endl;
        long *bounce_counts = new long[N_THREADS];
        int *samples_done = new int[N_THREADS];

        for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
        {
            bounce_counts[thread_id] = 0;
            samples_done[thread_id] = 0;
        }

        // create N_THREADS buckets to dump paths into.

        paths *array_of_paths = new paths[N_THREADS];
        if (trace_probability > 0.0)
        {
            int avg_number_of_paths = config.value("avg_number_of_paths", 100);
            for (int t = 0; t < N_THREADS; t++)
            {
                array_of_paths[t] = paths();
                array_of_paths[t].reserve(avg_number_of_paths / N_THREADS);
                for (int i = 0; i < avg_number_of_paths / N_THREADS; i++)
                {
                    array_of_paths[t].push_back(new path());
                    // for (int j = 0; j < MAX_BOUNCES/2; j++) {
                    //     array_of_paths[t].back()->push_back(vec3(0, 0, 0));
                    // }
                    array_of_paths[t].back()->reserve(MAX_BOUNCES / 2);
                }
            }
        }

        std::cout << "spawning threads";
        for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
        {
            int samples = min_samples + (int)(thread_id < remaining_samples);
            std::cout << ' ' << samples;
            // pass the address of bounce_counts[t] so that it can be modified. this should be thread safe though, and thus shouldn't require a lock
            // wrap framebuffer in a std::ref since it needs to be modified

            threads[thread_id] = std::thread(this->compute, thread_id, &bounce_counts[thread_id], samples_done, std::ref(framebuffer), samples, trace_probability, array_of_paths);
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

        float total_bounces = 0;

        long num_samples_done;
        long num_samples_left = 1;
        using namespace std::chrono_literals;
        float avg_luminance, max_luminance, total_luminance;
        calculate_luminance(framebuffer, film.width, film.height, num_samples_done / (width * height), width * height, max_luminance, total_luminance, avg_luminance);

        while (num_samples_left > 0)
        {
            num_samples_done = 0;
            num_samples_left = 0;
            for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
            {
                assert(samples_done[thread_id] >= 0);
                num_samples_done += samples_done[thread_id];
            }
            num_samples_left = min_camera_rays - num_samples_done;
            print_out_progress(num_samples_done, num_samples_left, t3);
            calculate_luminance(framebuffer, width, height, num_samples_done / (width * height), width * height, max_luminance, total_luminance, avg_luminance);
            output_to_file(output, framebuffer, width, height, num_samples_done / (width * height), 10.0, exposure, gamma);
            std::this_thread::sleep_for(0.5s);
        }
    };
    void compute(int thread_id, long *ray_ct, int *completed_samples, int samples, int max_bounces, float trace_probability, paths *array_of_paths)
    {
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
                    col += de_nan(integrator->color(r, world, 0, max_bounces, count, _path));
                    if (_path != nullptr)
                    {
                        // std::cout << "traced _path, size is " << _path->size() << std::endl;
                        if (traces > array_of_paths[thread_id].size())
                        {
                            array_of_paths[thread_id].push_back(_path);
                        }
                    }

                    framebuffer_lock.lock();
                    framebuffer[j][i] += col;
                    *ray_ct += *count;
                    completed_samples[thread_id] += 1;
                    framebuffer_lock.unlock();
                }
            }
        }
        // std::cout << "total length of traced paths : " << paths[thread_id].size() << std::endl;
    }

    int N_THREADS;
    std::thread *threads;
    json config;
    Integrator *integrator;
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
//             for (int i = 0; i < width; i++)
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
