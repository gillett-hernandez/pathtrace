#pragma once
#include <iostream>
#include "types.h"
#include "vec3.h"
#include "texture.h"

class image_texture : public texture
{
public:
    image_texture(int width, int height) : width(width), height(height)
    {
        data = array_2d<vec3>(width, height);
        alpha_mask = array_2d<float>(width, height);
    }
    vec3 value(float u, float v, const vec3 &p) const
    {
        // assert(0 <= u && u <= 1 && 0 <= v && v <= 1);
        v -= int(v);
        if (v < 0)
        {
            v += 1;
        }
        u -= int(u);
        if (u < 0)
        {
            u += 1;
        }
        int y = int(v * height);
        int x = int(u * width);
        return data[y][x];
    }
    float alpha(float u, float v, const vec3 &p) const
    {
        v -= int(v);
        if (v < 0)
        {
            v += 1;
        }
        u -= int(u);
        if (u < 0)
        {
            u += 1;
        }
        int y = int(v * height);
        int x = int(u * width);
        return alpha_mask[y][x];
    }
    int width, height;
    vec3 **data;
    float **alpha_mask;
};

image_texture *from_4byte_vector(std::vector<unsigned char> image, int width, int height)
{
    image_texture *it = new image_texture(width, height);
    for (int y = height - 1; y >= 0; y--)
    {
        for (int x = 0; x < width; x++)
        {
            float r = image[4 * (y * width + x) + 0] / 255.0;
            float g = image[4 * (y * width + x) + 1] / 255.0;
            float b = image[4 * (y * width + x) + 2] / 255.0;
            it->alpha_mask[y][x] = image[4 * (y * width + x) + 3] / 255.0;
            it->data[y][x] = vec3(r, g, b);
            // std::cout << it->alpha_mask[y][x];
        }
        // std::cout << '\n';
    }
    return it;
}