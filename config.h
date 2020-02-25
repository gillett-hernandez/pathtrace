#pragma once
#include "vec3.h"
#include "thirdparty/json.hpp"

using json = nlohmann::json;

#include <vector>
#include <map>

struct s_film
{
    size_t width;
    size_t height;
    size_t total_pixels;
    float exposure;
    float gamma;
    s_film(){};
    s_film(json film)
    {
        width = (size_t)film.value("width", 400);
        height = (size_t)film.value("height", 300);

        exposure = (float)film.value("gamma", 2.2);
        gamma = (float)film.value("exposure", 0.0);
        total_pixels = width * height;
    };
};

enum RenderType
{
    NAIVE,
    PROGRESSIVE,
    TILED
};

RenderType get_render_type_for(std::string type)
{
    static std::map<std::string, RenderType> mapping = {
        {"naive", NAIVE},
        {"progressive", PROGRESSIVE},
        {"tiled", TILED}};
    return mapping[type];
}

struct Config
{
    s_film film;
    std::string ppm_output_path;
    std::string png_output_path;
    std::string traced_paths_output_path;
    std::string traced_paths_2d_output_path;
    std::string scene_path;
    bool should_trace_paths;
    float avg_number_of_paths;
    float trace_probability;
    RenderType render_type;
    size_t max_bounces;
    size_t samples;
    uint16_t threads;
    Config(){};

    Config(json jconfig)
    {
        film = s_film(jconfig["film"]);

        ppm_output_path = jconfig.value("ppm_output_path", "out.ppm");
        png_output_path = jconfig.value("png_output_path", "out.png");
        traced_paths_output_path = jconfig["traced_paths_output_path"].get<std::string>();
        traced_paths_2d_output_path = jconfig["traced_paths_2d_output_path"].get<std::string>();
        scene_path = jconfig.value("scene", "scenes/scene.json");
        should_trace_paths = jconfig.value("should_trace_paths", false);
        avg_number_of_paths = jconfig.value("avg_number_of_paths", 100.0f);

        render_type = get_render_type_for(jconfig.value("render_type", "progressive"));
        max_bounces = (size_t)jconfig.value("max_bounces", 10);
        samples = (size_t)jconfig.value("samples", 20);
        threads = (uint16_t)jconfig.value("threads", 1);

        long min_camera_rays = samples * film.total_pixels;

        if (should_trace_paths)
        {
            trace_probability = jconfig.value("avg_number_of_paths", 100.0) / min_camera_rays;
        }
        else
        {
            trace_probability = 0.0;
        }
    };
};

// Config parse_config(json jconfig)
// {
//     // film setup

// }
