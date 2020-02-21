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

config parse_config(json jconfig)
{

    // film setup

    s_film film = {
        jconfig["film"].value("width", 400),
        jconfig["film"].value("height", 300),
        0,
        jconfig["film"].value("gamma", 2.2),
        jconfig["film"].value("exposure", 0.0)};

    config config = {
        film,
        jconfig.value("ppm_output_path", "out.ppm"),
        jconfig["png_output_path"],
        jconfig["traced_paths_output_path"],
        jconfig["traced_paths_2d_output_path"],
        jconfig["scene_path"],
        jconfig.get<bool>("should_trace_paths", false),
        jconfig.get<float>("avg_number_of_paths", 0.0f),
        0.0,
        get_render_type_for(jconfig["render_type"]),
        jconfig["max_bounces"],
        jconfig["samples"],
        jconfig["threads"]};

    std::string scene_path = jconfig.value("scene", "scenes/scene.json");
    int n_samples = jconfig.value("samples", 20);
    int N_THREADS = jconfig.value("threads", 4);
    int MAX_BOUNCES = jconfig.value("max_bounces", 10);
    float progressive_proportion = jconfig.value("progressive_render_proportion", 0.5);
    bool should_trace_paths = jconfig.value("should_trace_paths", false);
    float trace_probability;

    long total_pixels = width * height;
    long min_camera_rays = n_samples * total_pixels;

    if (should_trace_paths)
    {
        config.trace_probability = jconfig.value("avg_number_of_paths", 100.0) / min_camera_rays;
    }
    else
    {
        config.trace_probability = 0.0;
    }
    return config;
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
    json jconfig;
    config_file >> jconfig;

    config config = parse_config(jconfig);
    s_film film = config.film;

    // end film setup

    int pixels = 0;

    // other config
    std::cout << "trace probability is " << config.trace_probability << std::endl;

    std::cout << std::endl;
    std::cout << "config read complete, rendering image at " << film.width << "x" << film.height << "==" << film.total_pixels << "px^2" << std::endl;
    std::cout << "with " << config.n_samples << " samples per pixel, that sets a minimum number of camera rays at " << min_camera_rays << "\n\n";
    std::cout << "using " << config.threads << " threads\n";

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
    std::ofstream output(config.ppm_output_path);

    Integrator *integrator = new RecursivePT();
    Renderer *renderer = new Progressive(integrator, cam, world, output);
    renderer->start_render(t2);

    while (!renderer->is_done())
    {
        renderer->sync_progress();
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.5s);
    }

    std::cout << '\n';

    std::cout << " done\n";

    renderer->finalize();
}
