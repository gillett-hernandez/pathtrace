#ifndef SCENEH
#define SCENEH
#include "bvh.h"
#include "enums.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "thirdparty/json.hpp"
#include "world.h"
#include <map>
#include <string>

using json = nlohmann::json;

material *error_material()
{
    static material *mat = new lambertian(vec3(0.8, 0.2, 0.8));
    return mat;
}

std::string generate_new_id()
{
    static int last_id = 0;
    std::string s = std::to_string(last_id);
    last_id++;
    return s;
}

vec3 json_to_vec3(json color)
{
    return vec3(color.at(0), color.at(1), color.at(2));
}

world *build_scene(json scene)
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
        assert(element.contains("id"));
        std::string mat_id = element["id"].get<std::string>();
        if (!element.contains("data"))
        {
            materials.emplace(mat_id, error_material());
            continue;
        }
        std::cout << element << '\n';

        json data = element["data"];
        switch (get_material_type_for(element["type"].get<std::string>()))
        {
        case LAMBERTIAN:
            std::cout << "found LAMBERTIAN" << '\n';

            if (data.contains("color"))
            {
                materials.emplace(mat_id, new lambertian(json_to_vec3(data["color"])));
            }
            else if (data.contains("texture"))
            {
                // construct the material from the reference to a texture
                materials.emplace(mat_id, new lambertian(textures[data["texture"].get<std::string>()]));
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
            if (data.contains("color"))
            {
                // set tint
                color = json_to_vec3(data["color"]);
            }
            else
            {
                // default to no tint == white
                color = vec3(1.0, 1.0, 1.0);
            }

            materials.emplace(mat_id, new metal(color, data.value("roughness", 0.0)));
            break;
        }
        case DIELECTRIC:
        {
            std::cout << "found DIELECTRIC. TODO: PROGRAM THIS!" << '\n';
            break;
        }
        case DIFFUSE_LIGHT:
        {
            std::cout << "found DIFFUSE_LIGHT" << '\n';
            if (element["data"].contains("texture"))
            {
                // set tint
                texture *_texture = textures[element["data"]["texture"]];
                materials.emplace(mat_id, new diffuse_light(_texture));
            }
            else
            {
                vec3 color;
                if (element["data"].contains("color"))
                {
                    color = json_to_vec3(data["color"]);
                }
                else
                {
                    // default to no tint == white
                    color = vec3(1.0, 1.0, 1.0);
                }
                materials.emplace(mat_id, new diffuse_light(color));
            }
            break;
        }
        default:
            break;
        }
    }
    // iterate through normal instances, which are
    //      instanced primitives, i.e. primitives with a transform
    for (auto &element : scene["instances"])
    {
        std::cout << element << '\n';
        std::string instance_id = element.value("id", generate_new_id());
        material *_material;
        if (element["material"].contains("id"))
        {
            _material = materials[element["material"]["id"].get<std::string>()];
        }
        else
        {
            // what do we do in this case? idk. any materials should have been found in the prior stage.
            std::cout << "found misconfigured material for primitive with id " << instance_id << " and json " << element << '\n';
            _material = error_material();
        }
        transform3 transform;
        if (!element.contains("transform"))
        {
            transform = transform3();
        }
        else
        {
            vec3 scale, rotate, translate;
            json j_transform = element["transform"];
            if (j_transform.contains("scale") && j_transform["scale"].is_array())
            {
                scale = json_to_vec3(j_transform["scale"]);
            }
            else
            {
                float fscale = j_transform.value("scale", 1.0);
                scale = vec3(fscale, fscale, fscale);
            }
            if (j_transform.contains("rotate"))
            {
                rotate = json_to_vec3(j_transform["rotate"]);
            }
            else
            {
                rotate = vec3(0.0, 0.0, 0.0);
            }
            if (j_transform.contains("translate"))
            {
                translate = json_to_vec3(j_transform["translate"]);
            }
            else
            {
                translate = vec3(0.0, 0.0, 0.0);
            }
            transform = transform3(scale, rotate, translate);
        }
        switch (get_primitive_type_for(element["type"].get<std::string>()))
        {
        case MESH:
            std::cout << "found MESH" << '\n';
            break;

        case SPHERE:
        {
            std::cout << "found SPHERE" << '\n';
            list.push_back(new instance(new sphere(vec3(0, 0, 0), 1.0, _material), transform));
            break;
        }
        case RECT:
        {
            std::cout << "found RECT" << '\n';
            float x, z;
            if (element.contains("size"))
            {
                x = element["size"].at(0);
                z = element["size"].at(1);
            }
            else
            {
                x = 1.0;
                z = 1.0;
            }
            list.push_back(new instance(new rect(x, z, _material), transform));
            break;
        }
        case CUBE:
            std::cout << "found CUBE" << '\n';
            break;

        default:
            break;
        }
    }


    texture *background;
    if (scene.contains("world")) {
        if (scene["world"].contains("texture")) {
            background = textures[scene["world"]["texture"].get<std::string>()];
        } else if (scene["world"].contains("color")) {
            background = new constant_texture(json_to_vec3(scene["world"]["color"]));
        }
    } else {
        background = new constant_texture(vec3(0.3, 0.3, 0.3));
    }

    // iterate through objects which are collections of instances
    int size = list.size();
    return new world(new bvh_node(list.data(), size, 0.0f, 0.0f), background);
}

#endif
