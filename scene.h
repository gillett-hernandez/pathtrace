#ifndef SCENEH
#define SCENEH
#include "bvh.h"
#include "enums.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "thirdparty/json.hpp"
#include <map>

using json = nlohmann::json;

material *error_material()
{
    static material *mat = new lambertian(vec3(0.8, 0.2, 0.8));
    return mat;
}

vec3 json_to_vec3(json color)
{
    return vec3(color.at(0), color.at(1), color.at(2));
}

hittable *build_scene(json scene)
{
    std::vector<hittable *> list;
    std::map<std::string, texture *> textures;
    std::map<std::string, material *> materials;
    // the following line is where the assets would be stored upon loading, but since this is not coded yet, it's commented out
    // std::map<std::string, hittable*> assets;

    // iterate through assets, importing them and building meshes from them
    for (auto &element : scene["assets"])
    {
        if (element.value("skip", false))
        {
            continue;
        }
        std::cout << element << '\n';
        // currently the only accepted asset type is a .obj
        assert(element["type"].get<std::string>() == "object");
    }
    // iterate through and load textures
    for (auto &element : scene["textures"])
    {
        if (element.value("skip", false))
        {
            continue;
        }
        std::cout << element << '\n';
        // currently the only accepted asset type is a .obj
        // assert(element["type"].get<std::string>() == "object");
    }
    // iterate through and construct materials
    for (auto &element : scene["materials"])
    {
        if (element.value("skip", false))
        {
            continue;
        }
        std::cout << element << '\n';
        assert(element.contains("id"));
        std::string mat_id = element["id"].get<std::string>();
        switch (get_material_type_for(element["type"].get<std::string>()))
        {
        case LAMBERTIAN:
            std::cout << "found LAMBERTIAN" << '\n';

            if (element["data"].contains("color"))
            {
                materials.emplace(mat_id, new lambertian(json_to_vec3(element["data"]["color"])));
            }
            else if (element["data"].contains("texture"))
            {
                // construct the material from the reference to a texture
                materials.emplace(mat_id, new lambertian(textures[element["data"]["texture"].get<std::string>()]));
            }
            else
            {
                // default to using a mauve-like color to indicate an invalid specification, or the lack of a specification
                std::cout << "material misconfigured, check" << '\n';
                materials.emplace(mat_id, error_material());
            }
            break;
        case METAL:
        {
            std::cout << "found METAL" << '\n';
            vec3 color;
            if (element["data"].contains("color"))
            {
                // set tint
                color = json_to_vec3(element["data"]["color"]);
            }
            else
            {
                // default to no tint == white
                color = vec3(1.0, 1.0, 1.0);
            }
            if (element["data"].contains("roughness"))
            {
            }
            else
            {
                // default to using a mauve-like color to indicate an invalid specification, or the lack of a specification
                std::cout << "material misconfigured, check" << '\n';
                materials.emplace(mat_id, error_material());
            }
            break;
        }
        case DIELECTRIC:
            std::cout << "found DIELECTRIC" << '\n';
            break;

        case DIFFUSE_LIGHT:
            std::cout << "found DIFFUSE_LIGHT" << '\n';
            break;

        default:
            break;
        }
    }
    // iterate through normal instances, which are
    //      instanced primitives, i.e. primitives with a transform
    for (auto &element : scene["instances"])
    {
        std::cout << element << '\n';
        switch (get_primitive_type_for(element["type"].get<std::string>()))
        {
        case MESH:
            std::cout << "found MESH" << '\n';
            break;

        case SPHERE:
            std::cout << "found SPHERE" << '\n';
            break;

        case RECT:
            std::cout << "found RECT" << '\n';
            break;

        case CUBE:
            std::cout << "found CUBE" << '\n';
            break;

        default:
            break;
        }
    }

    texture *checker = new checker_texture(
        vec3(0.2, 0.3, 0.1),
        vec3(0.9, 0.9, 0.9),
        5.0f);
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
    list.push_back(new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.9, 0.2, 0.2), 0.1)));
    list.push_back(new sphere(vec3(0, 8, 0), 5.0, new diffuse_light(new constant_texture(vec3(1.0, 1.0, 1.0)))));
    // iterate through objects which are collections of instances
    int size = list.size();
    return new bvh_node(list.data(), size, 0.0f, 0.0f);
}

#endif
