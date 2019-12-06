#ifndef SCENEH
#define SCENEH
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "bvh.h"
#include "thirdparty/json.hpp"

using json = nlohmann::json;

hittable *build_scene(json scene)
{
    std::vector<hittable *> list;
    // iterate through assets, importing them and building meshes from them
    // iterate through normal instances, which are
    //      instanced primitives, i.e. primitives with a transform
    for (auto &element : scene["instances"])
    {
        std::cout << element << '\n';
    }

    texture *checker = new checker_texture(
        new constant_texture(vec3(0.2, 0.3, 0.1)),
        new constant_texture(vec3(0.9, 0.9, 0.9)));
    list.push_back(new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker)));
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
                    list.push_back(new sphere(
                        center,
                        0.2,
                        new lambertian(
                            new constant_texture(
                                vec3(random_double() * random_double(),
                                     random_double() * random_double(),
                                     random_double() * random_double())))));
                }
                else if (choose_mat < 0.95)
                { // metal
                    list.push_back(new sphere(center, 0.2,
                                              new metal(vec3(0.5 * (1 + random_double()),
                                                             0.5 * (1 + random_double()),
                                                             0.5 * (1 + random_double())),
                                                        0.5 * random_double())));
                }
                else
                { // glass
                    list.push_back(new sphere(center, 0.2, new dielectric(1.5)));
                }
            }
        }
    }

    list.push_back(new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5)));
    list.push_back(new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4, 0.2, 0.1)))));
    list.push_back(new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.9, 0.9, 0.9), 0.0)));
    list.push_back(new sphere(vec3(0, 8, 0), 5.0, new diffuse_light(new constant_texture(vec3(1.0, 1.0, 1.0)))));
    // iterate through objects which are collections of instances
    int size = list.size();
    return new bvh_node(list.data(), size, 0.0f, 0.0f);
}

#endif
