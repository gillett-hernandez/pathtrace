#pragma once
#include "vec3.h"
#include <vector>
#include <map>

struct s_film {
    size_t width;
    size_t height;
    float exposure;
    float gamma;
};


enum render_type
{
    NAIVE,
    PROGRESSIVE,
    TILED
};

render_type get_render_type_for(std::string type)
{
    static std::map<std::string, render_type> mapping = {
        {"naive", NAIVE},
        {"progressive", PROGRESSIVE},
        {"tiled", TILED}};        
    return mapping[type];
}


struct config {
    s_film film;
    std::string ppm_output_path;
    std::string png_output_path;
    std::string traced_paths_output_path;
    std::string traced_paths_2d_output_path;
    std::string scene_path;
    bool should_trace_paths;
    float avg_number_of_paths;
    render_type e_render_type;
    size_t max_bounces;
    size_t samples;
    uint16_t threads;
};