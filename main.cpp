#include "bvh.h"
#include "camera.h"
#include "example_scenes.h"
#include "float.h"
#include "helpers.h"
#include "hittable_list.h"
#include "material.h"
#include "pdf.h"
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
#include "config.h"

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

std::mutex framebuffer_lock;

int main(int argc, char *argv[])
{
    std::ifstream config_file("config.json");
    json jconfig;
    config_file >> jconfig;

    Config config = Config(jconfig);
    s_film film = config.film;

    // end film setup

    // other config
    long min_camera_rays = config.samples * film.total_pixels;
    std::cout << "trace probability is " << config.trace_probability << std::endl;

    std::cout << std::endl;
    std::cout << "config read complete, rendering image at " << film.width << "x" << film.height << "==" << film.total_pixels << "px^2" << std::endl;
    std::cout << "with " << config.samples << " samples per pixel, that sets a minimum number of camera rays at " << min_camera_rays << "\n\n";
    std::cout << "using " << config.threads << " threads\n";

    // x,y,z
    // y is up.
    std::cout << "reading scene data" << std::endl;

    json scene;
    std::ifstream scene_file(config.scene_path);
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
    camera cam = setup_camera(camera_json, float(film.width) / float(film.height));

    // end camera setup

    // before we compute everything, open the file

    Integrator *integrator = new RecursivePT(config.max_bounces, world);
    Renderer *renderer = new Progressive(integrator, cam, config);
    renderer->start_render(t2);

    while (!renderer->is_done())
    {
        renderer->sync_progress();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.5s);
    }

    std::cout << " done\n";

    renderer->finalize();
}
