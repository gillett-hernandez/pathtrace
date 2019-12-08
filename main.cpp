#include "float.h"
#include "hittable_list.h"
#include "camera.h"
#include "random.h"
#include "helpers.h"
#include "bvh.h"
#include "texture.h"
#include "material.h"
#include "primitive.h"
#include "thirdparty/json.hpp"
#include "example_scenes.h"
#include "scene.h"

using json = nlohmann::json;

#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <chrono>

vec3 color(const ray &r, hittable *world, int depth, int max_bounces)
{
    hit_record rec;
    if (world->hit(r, 0.001, MAXFLOAT, rec))
    {
        ray scattered;
        vec3 attenuation;
        vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
        if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            return emitted + attenuation * color(scattered, world, depth + 1, max_bounces);
        }
        else
        {
            return emitted;
        }
    }
    else
    {
        // world background color here
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.2, 0.1, 0.7);

        // return vec3(0, 0, 0);
    }
}

std::mutex framebuffer_lock;

void compute_rays(vec3 **buffer, int width, int height, int samples, int max_bounces, camera cam, hittable *world)
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
            // std::cout << "computing column " << i << std::endl
            vec3 col = vec3(0, 0, 0);
            for (int s = 0; s < samples; s++)
            {
                float u = float(i + random_double()) / float(width);
                float v = float(j + random_double()) / float(height);
                ray r = cam.get_ray(u, v);
                col += color(r, world, 0, max_bounces);
            }

            framebuffer_lock.lock();
            buffer[j][i] += col;
            framebuffer_lock.unlock();
        }
    }
}

template <class T>
void default_assign(json j, T &var, std::string key, T _default)
{
    if (j.find(key) != j.end())
    {
        var = j[key];
    }
    else
    {
        var = _default;
    }
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

    // other config
    int n_samples = config.value("samples", 20);
    int N_THREADS = config.value("threads", 4);
    int MAX_BOUNCES = config.value("max_bounces", 10);

    // round up to nearest multiple of N_THREADS

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
    std::ifstream scene_file("scene.json");
    scene_file >> scene;

    auto t1 = std::chrono::high_resolution_clock::now();
    hittable *world = cornell_box();
    // hittable *world = build_scene(scene);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = t2 - t1;
    std::cout << "time taken to build bvh " << elapsed_seconds.count() << std::endl;

    // camera setup

    json camera_json = scene["camera"];
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

    std::cout << lookfrom << ' ' << lookat << '\n';

    camera cam(lookfrom, lookat, vec3(0, 1, 0), vfov, float(width) / float(height),
               aperture, dist_to_focus, 0.0, 1.0);

    // end camera setup

    int pixels = 0;
    int total_pixels = width * height;
    // before we compute everything, open the file
    std::ofstream output(config.value("output_path", "out.ppm"));
    std::thread threads[N_THREADS];
    int min_samples = n_samples / N_THREADS;
    int remaining_samples = n_samples % N_THREADS;

    std::cout << min_samples << '\n';
    std::cout << remaining_samples << '\n';

    for (int t = 0; t < N_THREADS; t++)
    {
        int samples = min_samples + (int)(t < remaining_samples);
        std::cout << "spawning thread " << t << " with " << samples << " samples" << std::endl;
        threads[t] = std::thread(compute_rays, std::ref(framebuffer), width, height, samples, MAX_BOUNCES, cam, world);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = t3 - t2;
    std::cout << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl;
    for (int t = 0; t < N_THREADS; t++)
    {
        std::cout << "joining thread " << t << std::endl;
        threads[t].join();
        std::cout << "joined thread " << t << std::endl;
    }

    auto t4 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds3 = t4 - t3;
    std::cout << "time taken to compute " << elapsed_seconds3.count() << std::endl;
    std::cout << "computed " << total_pixels * n_samples << " rays in " << elapsed_seconds3.count() << "seconds, at " << total_pixels * n_samples / elapsed_seconds3.count() << " rays per second" << std::endl;

    // output file
    float max_luminance;
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = framebuffer[j][i];
            col /= float(n_samples);
            float f = col.length();
            if (f > max_luminance)
            {
                max_luminance = f;
            }
        }
    }

    std::cout << "max lum " << max_luminance << '\n';
    // |p|^(1/g) == 1
    // |p| == 1
    // float calculated_gamma = ;

    output << "P6\n"
           << width << " " << height << "\n255\n";
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = framebuffer[j][i];
            col /= float(n_samples);
            // color space interpolation here
            // first color mapping
            float lum = col.length();
            float new_lum = lum / (1 + lum);
            // float new_lum = 1.0 - expf(-0.1 * lum);
            float factor = new_lum / lum;
            col = vec3(factor * col[0], factor * col[1], factor * col[2]);
            // put gamma and exposure here

            char ir = int(255.99 * powf(col[0], gamma));
            char ig = int(255.99 * powf(col[1], gamma));
            char ib = int(255.99 * powf(col[2], gamma));
            output << ir << ig << ib;
        }
    }
    output.close();
}
