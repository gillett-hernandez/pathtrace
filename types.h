#pragma once
#include "vec3.h"
#include <vector>

#define ASSERT(condition, message)                                             \
    do                                                                         \
    {                                                                          \
        if (!(condition))                                                      \
        {                                                                      \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__   \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate();                                                  \
        }                                                                      \
    } while (false)

typedef std::vector<vec3> path;
typedef std::vector<path *> paths;

template <typename T>
T **array_2d(int width, int height)
{
    T **data = new T *[height];
    for (int i = 0; i < height; i++)
    {
        data[i] = new T[width];
        for (int j = 0; j < width; j++)
        {
            data[i][j] = T();
        }
    }
    return data;
}
