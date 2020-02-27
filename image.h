#pragma once

#include "vec3.h"

template<typename T>
class image {
public:
    image(int width, int height) {
        data = new T*[height];
        for (int i = 0; i < height; i++) {
            data[i] = new T[width];
            for (int j = 0; j < width; j++) {
                data[i][j] = new T();
            }
        }
    }
    T** data;
};