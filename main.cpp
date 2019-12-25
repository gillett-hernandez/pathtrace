#include "bvh.h"
#include "camera.h"
#include "example_scenes.h"
#include "float.h"
#include "helpers.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "random.h"
#include "scene.h"
#include "texture.h"
#include "world.h"
#include "tonemap.h"
#include "thirdparty/json.hpp"

using json = nlohmann::json;

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

vec3 color(const ray &r, world *world, int depth, int max_bounces, long *bounce_count, std::vector<vec3> *path)
{
    hit_record rec;
    if (world->hit(r, 0.001, MAXFLOAT, rec))
    {
        if (path != nullptr)
        {
            path->push_back(rec.p);
        }
        ray scattered;
        vec3 attenuation;
        vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            (*bounce_count)++;
            return emitted + attenuation * color(scattered, world, depth + 1, max_bounces, bounce_count, path);
        }
        else
        {
            return emitted;
        }
    }
    else
    {
        if (path != nullptr)
        {
            path->push_back(rec.p);
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

vec3 color2(ray &r, world *world, int depth, int max_bounces, long *bounce_count, std::vector<vec3> *path)
{
    hit_record rec;
    vec3 _color = vec3(0, 0, 0);
    ray scattered;
    vec3 attenuation = vec3(0, 0, 0);
    vec3 emitted = vec3(0, 0, 0);

    vec3 beta = vec3(1.0, 1.0, 1.0);
    for (int i = 0; i < max_bounces; i++)
    {
        if (path != nullptr)
        {
            path->push_back(rec.p);
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

camera setup_camera(json camera_json, float aspect_ratio, vec3 vup = vec3(0, 1, 0))
{
    vec3 lookfrom(
        camera_json["look_from"].at(0),
        camera_json["look_from"].at(1),
        camera_json["look_from"].at(2));

    vec3 lookat(
        camera_json["look_at"].at(0),
        camera_json["look_at"].at(1),
        camera_json["look_at"].at(2));

    float vfov = camera_json.value("fov", 30.0);
    float aperture = camera_json.value("aperture", 0.0);
    float dist_to_focus = camera_json.value("dist_to_focus", 10.0);

    return camera(lookfrom, lookat, vup, vfov, aspect_ratio,
                  aperture, dist_to_focus, 0.0, 1.0);
}

void output_to_file(std::ofstream &output, vec3 **buffer, int width, int height, int samples, float max_luminance, float exposure, float gamma)
{
    output.seekp(0);
    // output file
    output << "P6\n"
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

            col = to_srgb(tonemap_uncharted(col, max_luminance));
            // put gamma and exposure here

            char ir = int(255.99 * powf(col[0], gamma));
            char ig = int(255.99 * powf(col[1], gamma));
            char ib = int(255.99 * powf(col[2], gamma));
            output << ir << ig << ib;
        }
    }
    output.flush();
}

std::mutex framebuffer_lock;

void compute_rays_single_pass(int thread_id, long *ray_ct, int *completed_samples, vec3 **buffer, int width, int height, int samples, int max_bounces, camera cam, world *world, float trace_probability, std::vector<std::vector<vec3> *> *paths)
{
    if (samples == 0)
    {
        return;
    }
    for (int j = height - 1; j >= 0; j--)
    {
        // std::cout << "computing row " << j << std::endl;
        for (int i = 0; i < width; i++)
        {
            // std::cout << "computing column " << i << std::endl;
            vec3 col = vec3(0, 0, 0);
            long *count = new long(0);

            for (int s = 0; s < samples; s++)
            {
                float u = float(i + random_double()) / float(width);
                float v = float(j + random_double()) / float(height);
                ray r = cam.get_ray(u, v);
                std::vector<vec3> *path = nullptr;
                if (random_double() < trace_probability)
                {
                    // std::cout << "creating path to trace" << std::endl;
                    path = new std::vector<vec3>();
                }
                col += de_nan(color2(r, world, 0, max_bounces, count, path));
                // col += de_nan(color(r, world, 0, max_bounces, count, path));
                if (path != nullptr)
                {
                    // std::cout << "traced path, size is " << path->size() << std::endl;
                    paths[thread_id].push_back(path);
                }
            }

            framebuffer_lock.lock();
            buffer[j][i] += col;
            *ray_ct += *count;
            completed_samples[thread_id] += samples;
            framebuffer_lock.unlock();
        }
    }
    // std::cout << "total length of traced paths : " << paths[thread_id].size() << std::endl;
}

void compute_rays_progressive(int thread_id, long *ray_ct, int *completed_samples, vec3 **buffer, int width, int height, int samples, int max_bounces, camera cam, world *world, float trace_probability, std::vector<std::vector<vec3> *> *paths)
{
    if (samples == 0)
    {
        return;
    }
    for (int s = 0; s < samples; s++)
    {
        for (int j = height - 1; j >= 0; j--)
        {
            // std::cout << "computing row " << j << std::endl;
            for (int i = 0; i < width; i++)
            {
                // std::cout << "computing column " << i << std::endl;
                vec3 col = vec3(0, 0, 0);
                long *count = new long(0);

                float u = float(i + random_double()) / float(width);
                float v = float(j + random_double()) / float(height);
                ray r = cam.get_ray(u, v);
                std::vector<vec3> *path = nullptr;
                if (random_double() < trace_probability)
                {
                    // std::cout << "creating path to trace" << std::endl;
                    path = new std::vector<vec3>();
                }
                col += de_nan(color2(r, world, 0, max_bounces, count, path));
                // col += de_nan(color(r, world, 0, max_bounces, count, path));
                if (path != nullptr)
                {
                    // std::cout << "traced path, size is " << path->size() << std::endl;
                    paths[thread_id].push_back(path);
                }

                framebuffer_lock.lock();
                buffer[j][i] += col;
                *ray_ct += *count;
                completed_samples[thread_id] += 1;
                framebuffer_lock.unlock();
            }
        }
    }
    // std::cout << "total length of traced paths : " << paths[thread_id].size() << std::endl;
}

int main(int argc, char *argv[])
{
    std::ifstream config_file("config.json");
    json config;
    config_file >> config;

    // film setup

    int width = config["film"].value("width", 400);
    int height = config["film"].value("height", 300);
    float gamma = config["film"].value("gamma", 2.2);
    float exposure = config["film"].value("exposure", 0.0);

    // end film setup

    int pixels = 0;
    long total_pixels = width * height;

    // other config
    std::string scene_path = config.value("scene", "scenes/scene.json");
    int n_samples = config.value("samples", 20);
    int N_THREADS = config.value("threads", 4);
    int MAX_BOUNCES = config.value("max_bounces", 10);
    float progressive_proportion = config.value("progressive_render_proportion", 0.5);
    bool should_trace_paths = config.value("should_trace_paths", false);
    float trace_probability;
    long min_camera_rays = n_samples * total_pixels;

    if (should_trace_paths)
    {
        trace_probability = config.value("avg_number_of_paths", 100.0) / min_camera_rays;
    }
    else
    {
        trace_probability = 0.0;
    }
    std::cout << "trace probability is " << trace_probability << std::endl;

    std::cout << std::endl;
    std::cout << "config read complete, rendering image at " << width << "x" << height << "==" << total_pixels << "px^2" << std::endl;
    std::cout << "with " << n_samples << " samples per pixel, that sets a minimum number of camera rays at " << min_camera_rays << "\n\n";
    std::cout << "using " << N_THREADS << " threads\n";

    // create framebuffer
    vec3 **framebuffer = new vec3 *[height];
    for (int j = height - 1; j >= 0; j--)
    {
        // std::cout << "computed row " << j << std::endl;
        framebuffer[j] = new vec3[width];
        for (int i = 0; i < width; i++)
        {
            framebuffer[j][i] = vec3(0, 0, 0);
        }
    }

    // x,y,z
    // y is up.
    std::cout << "reading scene data" << std::endl;

    json scene;
    std::ifstream scene_file(scene_path);
    scene_file >> scene;

    auto t1 = std::chrono::high_resolution_clock::now();
    // hittable *world = cornell_box();
    world *world = build_scene(scene);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = t2 - t1;
    std::cout << "time taken to build bvh " << elapsed_seconds.count() << std::endl;

    // camera setup

    json camera_json = scene["camera"];
    camera cam = setup_camera(camera_json, float(width) / float(height));

    // end camera setup

    // before we compute everything, open the file
    std::ofstream output(config.value("output_path", "out.ppm"));

    // start thread setup
    std::thread threads[N_THREADS];
    int min_samples = n_samples / N_THREADS;
    int remaining_samples = n_samples % N_THREADS;

    std::cout << "samples per thread " << min_samples << std::endl;
    std::cout << "leftover samples to be allocated " << remaining_samples << std::endl;
    long bounce_counts[N_THREADS];
    int *samples_done = new int[N_THREADS];
    // create N_THREADS buckets to dump paths into.

    typedef std::vector<vec3> path;
    typedef std::vector<path *> paths;
    paths *array_of_paths = new paths[N_THREADS];

    std::cout << "spawning threads";
    for (int t = 0; t < N_THREADS; t++)
    {
        bounce_counts[t] = 0;
        int samples = min_samples + (int)(t < remaining_samples);
        bounce_counts[t] = 0;
        samples_done[t] = 0;
        std::cout << ' ' << samples;
        if (random_double() < progressive_proportion)
        {
            threads[t] = std::thread(compute_rays_progressive, t, &bounce_counts[t], samples_done, std::ref(framebuffer), width, height, samples, MAX_BOUNCES, cam, world, trace_probability, array_of_paths);
        }
        else
        {
            threads[t] = std::thread(compute_rays_single_pass, t, &bounce_counts[t], samples_done, std::ref(framebuffer), width, height, samples, MAX_BOUNCES, cam, world, trace_probability, array_of_paths);
        }
    }
    std::cout << " done.\n";
    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = t3 - t2;
    std::cout << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl;
    std::cout << "joining threads";

    float total_bounces = 0;

    long num_samples_done;
    long num_samples_left = 1;
    using namespace std::chrono_literals;
    while (num_samples_left > 0)
    {
        num_samples_done = 0;
        num_samples_left = 0;
        for (int t = 0; t < N_THREADS; t++)
        {
            num_samples_done += samples_done[t];
        }
        num_samples_left = min_camera_rays - num_samples_done;
        auto intermediate = std::chrono::high_resolution_clock::now();
        // rate was in rays per nanosecond
        // first multiply by 1 billion to get rays per second
        float rate = 1000000000 * num_samples_done / (intermediate - t3).count();

        std::cout << "samples left " << num_samples_left << '\t'
                  << "rate " << rate << "\t"
                  << "time left " << num_samples_left / rate << '\n';
        output_to_file(output, framebuffer, width, height, num_samples_done / (width * height), 10.0, exposure, gamma);
        std::this_thread::sleep_for(0.5s);
    }

    for (int t = 0; t < N_THREADS; t++)
    {
        threads[t].join();
        total_bounces += (float)bounce_counts[t];
        std::cout << ' ' << t << ':' << bounce_counts[t] << "bounces, ";
    }

    std::cout << " done\n";
    auto t4 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds3 = t4 - t3;
    std::cout << "time taken to compute " << elapsed_seconds3.count() << std::endl;
    float rate1 = total_pixels * n_samples / elapsed_seconds3.count();
    float rate2 = total_bounces / elapsed_seconds3.count();
    std::cout << "computed " << total_pixels * n_samples << " camera rays in " << elapsed_seconds3.count() << "s, at " << rate1 << " rays per second, or " << rate1 / N_THREADS << "per thread" << std::endl;
    std::cout << "computed " << total_bounces << " rays, at " << rate2 << " rays per second, or " << rate2 / N_THREADS << " per thread" << std::endl;

    int added_paths = 0;
    if (should_trace_paths)
    {
        std::ofstream traced_paths_output(config.value("traced_paths_output", "paths.txt"));
        std::ofstream traced_paths_output2d(config.value("traced_paths_2d_output", "paths2d.txt"));
        for (int t = 0; t < N_THREADS; t++)
        {
            auto paths = array_of_paths[t];

            added_paths += paths.size();

            for (auto &path : paths)
            {
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
    assert(added_paths > 0 || !should_trace_paths);
    std::cout << "added " << added_paths << " paths" << std::endl;

    float max_luminance = -FLT_MAX;
    float total_luminance = 0.0;
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = framebuffer[j][i];
            col /= float(n_samples);
            float f = abs(col.length());
            total_luminance += f;
            if (f > max_luminance)
            {
                max_luminance = f;
            }
        }
    }
    float avg_luminance = total_luminance / ((float)total_pixels * (float)n_samples);

    std::cout << "avg lum " << avg_luminance << std::endl;
    std::cout << "max lum " << max_luminance << std::endl;

    output_to_file(output, framebuffer, width, height, n_samples, max_luminance, exposure, gamma);
}
