#include "float.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "random.h"
#include "helpers.h"
#include "bvh.h"
#include "material.h"
#include "thirdparty/json.hpp"

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
        if (depth < max_bounces && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
        {
            return attenuation * color(scattered, world, depth + 1, max_bounces);
        }
        else
        {
            return vec3(0, 0, 0);
        }
    }
    else
    {
        // world background color here
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);
    }
}

hittable *random_scene()
{
    int n = 500;
    hittable **list = new hittable *[n + 1];
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(vec3(0.5, 0.5, 0.5)));
    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float choose_mat = random_double();
            vec3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - vec3(4, 0.2, 0)).length() > 0.9)
            {
                if (choose_mat < 0.8)
                { // diffuse
                    list[i++] = new sphere(center, 0.2,
                                           new lambertian(vec3(random_double() * random_double(),
                                                               random_double() * random_double(),
                                                               random_double() * random_double())));
                }
                else if (choose_mat < 0.95)
                { // metal
                    list[i++] = new sphere(center, 0.2,
                                           new metal(vec3(0.5 * (1 + random_double()),
                                                          0.5 * (1 + random_double()),
                                                          0.5 * (1 + random_double())),
                                                     0.5 * random_double()));
                }
                else
                { // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.1)));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));
    std::cerr << i << std::endl;
    return new bvh_node(list, i, 0.0f, 0.0f);
}

std::mutex framebuffer_lock;

void compute_rays(vec3 **buffer, json config, camera cam, hittable *world)
{

    for (int j = config["height"].get<int>() - 1; j >= 0; j--)
    {
        // std::cerr << "computing row " << j << std::endl;
        for (int i = 0; i < config["width"].get<int>(); i++)
        {
            // std::cerr << "computing column " << i << std::endl
            vec3 col = vec3(0, 0, 0);
            for (int s = 0; s < config["samples"].get<int>(); s++)
            {
                float u = float(i + random_double()) / float(config["width"].get<int>());
                float v = float(j + random_double()) / float(config["height"].get<int>());
                ray r = cam.get_ray(u, v);
                col += color(r, world, 0, config["max_bounces"].get<int>());
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
    std::ifstream input("config.json");
    json j;
    input >> j;
    int width, height, n_samples, N_THREADS, MAX_BOUNCES;
    // int width, height, n_samples, N_THREADS, MAX_BOUNCES;
    // default_assign(j, width, "width", 1920/10);
    // int width = j.value("width", 1920/10);
    default_assign(j, width, "width", 1920 / 10);
    default_assign(j, height, "height", 1080 / 10);
    default_assign(j, n_samples, "samples", 20);
    default_assign(j, N_THREADS, "threads", 4);
    default_assign(j, MAX_BOUNCES, "max_bounces", 10);

    // round up to nearest multiple of N_THREADS
    n_samples += N_THREADS;
    n_samples -= n_samples % N_THREADS;
    vec3 **framebuffer = new vec3 *[height];
    for (int j = height - 1; j >= 0; j--)
    {
        // std::cerr << "computed row " << j << std::endl;
        framebuffer[j] = new vec3[width];
        for (int i = 0; i < width; i++)
        {
            framebuffer[j][i] = vec3(0, 0, 0);
        }
    }

    // x,y,z
    // y is up.
    auto t1 = std::chrono::high_resolution_clock::now();
    // hittable *list[3];
    // int i = 0;
    // list[i++] = new sphere(vec3(0,-100,0), 100, new lambertian(vec3(0.2, 0.2, 0.2)));
    // list[i++] = new sphere(vec3(1,1,0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    // list[i++] = new sphere(vec3(-1,1,0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    // hittable *world = new bvh_node(list, i, 0.0f, 0.0f);
    hittable *world = random_scene();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = t2 - t1;
    std::cerr << "time taken to build bvh " << elapsed_seconds.count() << std::endl;
    vec3 lookfrom(0, 1, 10);
    vec3 lookat(0, 1, 0);
    float dist_to_focus = (lookfrom - lookat).length();
    float aperture = 0.05;
    int fov = 40;

    camera cam(lookfrom, lookat, vec3(0, 1, 0), fov,
               float(width) / float(height), aperture, dist_to_focus, 0.0f, 0.0f);
    int pixels = 0;
    int total_pixels = width * height;
    std::thread threads[N_THREADS];

    for (int t = 0; t < N_THREADS; t++)
    {
        std::cerr << "spawning thread " << t << std::endl;
        threads[t] = std::thread(compute_rays, std::ref(framebuffer), j, cam, world);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = t3 - t2;
    std::cerr << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl;
    for (int t = 0; t < N_THREADS; t++)
    {
        std::cerr << "joining thread " << t << std::endl;
        threads[t].join();
        std::cerr << "joined thread " << t << std::endl;
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds3 = t4 - t3;
    std::cerr << "time taken to compute " << elapsed_seconds3.count() << std::endl;

    std::cerr << "computed " << total_pixels * n_samples << " rays in " << elapsed_seconds3.count() << "seconds, at " << total_pixels * n_samples / elapsed_seconds3.count() << " rays per second" << std::endl;

    std::cout << "P6\n"
              << width << " " << height << "\n255\n";
    for (int j = height - 1; j >= 0; j--)
    {
        for (int i = 0; i < width; i++)
        {
            vec3 col = framebuffer[j][i];
            col /= float(n_samples);

            col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

            char ir = int(255.99 * col[0]);
            char ig = int(255.99 * col[1]);
            char ib = int(255.99 * col[2]);
            std::cout << ir << ig << ib;
        }
    }
}
