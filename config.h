#pragma once
#include "vec3.h"
#include "camera.h"
#include "thirdparty/json.hpp"

using json = nlohmann::json;

#include <vector>
#include <map>

struct s_film
{
    int width;
    int height;
    long total_pixels;
    float exposure;
    float gamma;
    s_film(){};
    s_film(json film)
    {
        width = film.value("width", 400);
        height = film.value("height", 300);

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

enum IntegratorType
{
    RPT,
    IPT,
    BPT,
    RNEEPT,
    INEEPT,
    BDPT,
    SPPM,
    VCM,
    MLT
};

IntegratorType get_integrator_type_for(std::string type)
{
    static std::map<std::string, IntegratorType> mapping = {
        {"recursive path tracing", RPT},
        {"iterative path tracing", IPT},
        {"branched path tracing", BPT},
        {"recursive nee path tracing", RNEEPT},
        {"iterative nee path tracing", INEEPT},
        {"bidirectional path tracing", BDPT},
        {"stochastic progressive photon mapping", SPPM},
        {"metropolis light transport", MLT},
        {"vertex connection merging", VCM}};
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
    int block_width;
    int block_height;
    float trace_probability;
    RenderType render_type;
    IntegratorType integrator_type;
    int max_bounces;
    int samples;
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
        block_width = jconfig.value("block_width", 64);
        block_height = jconfig.value("block_height", 64);

        render_type = get_render_type_for(jconfig.value("render_type", "progressive"));
        integrator_type = get_integrator_type_for(jconfig.value("integrator_type", "recursive path tracing"));
        max_bounces = jconfig.value("max_bounces", 10);
        samples = jconfig.value("samples", 20);
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
