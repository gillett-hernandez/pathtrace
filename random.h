#pragma once

#include <functional>
#include <random>
#include "vec3.h"

#define TAU 2 * M_PI

inline double random_double()
{
  static std::uniform_real_distribution<double> distribution(0.0, 1.0);
  static std::mt19937 generator;
  static std::function<double()> rand_generator =
      std::bind(distribution, generator);
  return rand_generator();
}
inline vec3 random_in_unit_sphere()
{
  // vec3 p;
  // do
  // {
  //     p = 2.0 * vec3(random_double(), random_double(), random_double()) - vec3(1, 1, 1);
  // } while (p.squared_length() >= 1.0);
  float u, v, w;
  u = random_double() * TAU;
  v = acos(2 * random_double() - 1);
  w = powf(random_double(), 1.0 / 3.0);
  return vec3(cos(u) * sin(v) * w, cos(v) * w, sin(u) * sin(v) * w);
}

inline vec3 random_in_unit_disk()
{
  // vec3 p;
  // do
  // {
  //     p = 2.0 * vec3(random_double(), random_double(), 0) - vec3(1, 1, 0);
  // } while (dot(p, p) >= 1.0);
  float u, v;
  u = random_double() * TAU;
  v = powf(random_double(), 1.0 / 2.0);
  vec3 p = vec3(cos(u) * v, sin(u) * v, 0);
  return p;
}

inline vec3 random_cosine_direction()
{
  float r1 = random_double();
  float r2 = random_double();
  float z = sqrt(1 - r2);
  float phi = 2 * M_PI * r1;
  float x = cos(phi) * sqrt(r2);
  float y = sin(phi) * sqrt(r2);
  return vec3(x, y, z);
}
