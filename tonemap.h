#pragma once
#include "vec3.h"

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;

template <class T>
T Uncharted2TonemapFunc(T x)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 tonemap_uncharted(vec3 color, float white)
{
    vec3 temp_color = Uncharted2TonemapFunc<vec3>(color) / Uncharted2TonemapFunc<float>(white);

    return vec3(clamp(temp_color.x(), 0.0f, 1.0f),
                clamp(temp_color.y(), 0.0f, 1.0f),
                clamp(temp_color.z(), 0.0f, 1.0f));
}

vec3 tonemap_1(vec3 color)
{
    // float lum = col.length();
    float lum = std::max({color[0], color[1], color[2]});
    // float new_lum = lum / (1 + lum);
    float new_lum = 1.0 - expf(-0.2 * lum);
    float factor = new_lum / lum;
    // col = vec3(factor * col[0], factor * col[1], factor * col[2]);
    color = color * factor;
    // put gamma and exposure here
    return color;
}

vec3 tonemap_2(vec3 color)
{
    // float lum = col.length();
    float lum = std::max({color[0], color[1], color[2]});
    // float new_lum = lum / (1 + lum);
    float new_lum = 1.0 - expf(-0.2 * lum);
    float factor = new_lum / lum;
    // col = vec3(factor * col[0], factor * col[1], factor * col[2]);
    color = color * factor;
    // put gamma and exposure here
    return color;
}
