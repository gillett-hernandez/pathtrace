#pragma once

#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"

hittable *random_scene1()
{
    int n = 500;
    hittable **list = new hittable *[n + 1];
    // list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(new constant_texture(vec3(0.5, 0.5, 0.5))));
    texture *checker = new checker_texture(
        new constant_texture(vec3(0.2, 0.3, 0.1)),
        new constant_texture(vec3(0.9, 0.9, 0.9)),
        2.0);
    list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker));
    int i = 1;
    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            float choose_mat = random_double();
            vec3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());
            if ((center - vec3(4, 0.2, 0)).length() > 0.9)
            {
                if (choose_mat < 0.8)
                { // diffuse
                    list[i++] = new sphere(
                        center,
                        0.2,
                        new lambertian(
                            new constant_texture(
                                vec3(random_double() * random_double(),
                                     random_double() * random_double(),
                                     random_double() * random_double()))));
                }
                else if (choose_mat < 0.95)
                { // metal
                    list[i++] = new sphere(center, 0.2,
                                           new metal(vec3(0.5 * (1 + random_double()),
                                                          0.5 * (1 + random_double()),
                                                          0.5 * (1 + random_double())),
                                                     0.5 * random_double()));
                }
                else
                { // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4, 0.2, 0.1))));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.9, 0.9, 0.9), 0.0));
    list[i++] = new sphere(vec3(0, 8, 0), 5.0, new diffuse_light(new constant_texture(vec3(1.0, 1.0, 1.0))));
    std::cout << i << std::endl;
    return new bvh_node(list, i, 0.0f, 0.0f);
}

hittable *test_scene()
{
    hittable *list[3];
    int i = 0;
    list[i++] = new sphere(vec3(0, -100, 0), 100, new lambertian(vec3(0.2, 0.2, 0.2)));
    list[i++] = new sphere(vec3(1, 1, 0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    list[i++] = new sphere(vec3(-1, 1, 0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    return new bvh_node(list, i, 0.0f, 0.0f);
}
