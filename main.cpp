#include "bvh.h"
#include "camera.h"
#include "example_scenes.h"
#include "float.h"
#include "helpers.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "random.h"
#include "scene_parser.h"
#include "texture.h"
#include "thirdparty/json.hpp"
#include "tonemap.h"
#include "world.h"
#include "types.h"
#include "integrator.h"
#include "renderer.h"

using json = nlohmann::json;

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

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

void print_out_progress(long num_samples_done, long num_samples_left, std::chrono::high_resolution_clock::time_point start_time)
{
    auto intermediate = std::chrono::high_resolution_clock::now();
    // rate was in rays per nanosecond
    // first multiply by 1 billion to get rays per second
    assert(num_samples_left >= 0);
    assert(num_samples_done >= 0);
    float rate = 1000000000 * num_samples_done / (intermediate - start_time).count();
    assert(rate > 0);

    std::cout << "samples left" << std::setw(20) << num_samples_left
              << " rate " << std::setw(10) << rate
              << " time left " << std::setw(5) << num_samples_left / rate
              << "                                           " << '\r' << std::flush;
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
    World *world = build_scene(scene);
    world->config = config;
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = t2 - t1;
    std::cout << "time taken to build bvh " << elapsed_seconds.count() << std::endl;

    // camera setup

    json camera_json = scene["camera"];
    camera cam = setup_camera(camera_json, float(width) / float(height));

    // end camera setup

    // before we compute everything, open the file
    std::ofstream output(config.value("ppm_output_path", "out.ppm"));

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

    Integrator *integrator = new RecursivePT(MAX_BOUNCES);
    Renderer *renderer = new progressive(framebuffer, framebuffer_lock, integrator, N_THREADS, bounce_counts, samples_done, remaining_samples, MAX_BOUNCES, trace_probability, array_of_paths);
    renderer->start_render();

    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = t3 - t2;
    std::cout << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl;
    std::cout << "joining threads\n";

    float total_bounces = 0;

    long num_samples_done;
    long num_samples_left = 1;
    using namespace std::chrono_literals;
    float avg_luminance, max_luminance, total_luminance;
    calculate_luminance(framebuffer, width, height, num_samples_done / (width * height), width * height, max_luminance, total_luminance, avg_luminance);

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

    std::cout << '\n';
    for (int thread_id = 0; thread_id < N_THREADS; thread_id++)
    {
        threads[thread_id].join();
        total_bounces += (float)bounce_counts[thread_id];
        std::cout << ' ' << thread_id << ':' << bounce_counts[thread_id] << "bounces, ";
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
    assert(added_paths > 0 || !should_trace_paths);
    std::cout << "added " << added_paths << " paths" << std::endl;

    calculate_luminance(framebuffer, width, height, num_samples_done / (width * height), width * height, max_luminance, total_luminance, avg_luminance);
    std::cout << "avg lum " << avg_luminance << std::endl;
    std::cout << "max lum " << max_luminance << std::endl;

    output_to_file(output, framebuffer, width, height, n_samples, max_luminance, exposure, gamma);
}
