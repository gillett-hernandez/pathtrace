#pragma once
#include "vec3.h"
#include <vector>
#include <map>

struct s_film
{
    size_t width;
    size_t height;
    size_t total_pixels;
    float exposure;
    float gamma;
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
};