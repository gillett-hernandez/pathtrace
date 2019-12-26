#ifndef SCENEH
#define SCENEH
#include "bvh.h"
#include "enums.h"
#include "hittable.h"
#include "hittable_list.h"
#include "material.h"
#include "primitive.h"
#include "thirdparty/json.hpp"
#include "volume.h"
#include "world.h"
#include <map>
#include <string>

#define MAUVE vec3(0.8, 0.2, 0.8)

using json = nlohmann::json;

material *error_material()
{
    static material *mat = new lambertian(MAUVE);
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

class wrapped_material
{
public:
    wrapped_material() : _material(nullptr), _type("uninitialized") {}
    // wrapped_material() {}
    wrapped_material(material *_material, std::string type) : _material(_material), _type(type) {}
    material *_material;
    std::string _type;
    material *unwrap()
    {
        return _material;
    }
};

// static wrapped_material default_material = wrapped_material{nullptr, "error"};

class wrapped_hittable
{
public:
    wrapped_hittable() : _hittable(nullptr), _material(wrapped_material()) {}
    // wrapped_hittable() {}
    wrapped_hittable(hittable *_hittable, wrapped_material _material) : _hittable(_hittable), _material(_material) {}
    hittable *_hittable;
    wrapped_material _material;
    hittable *unwrap()
    {
        return _hittable;
    }
    wrapped_material get_material()
    {
        return _material;
    }
};
// static wrapped_hittable default_hittable = wrapped_hittable(nullptr, default_material);

wrapped_material wrapped_error_material()
{
    static wrapped_material wmat = wrapped_material(error_material(), "lambertian");
    return wmat;
}

wrapped_hittable
parse_prim_or_instance(std::map<std::string, wrapped_hittable> primitives, std::map<std::string, wrapped_material> materials, json element)
{
    // material assign
    wrapped_material _material;
    wrapped_hittable primitive;
    std::cout << element << '\n';

    if (element.contains("material") && element["material"].contains("id"))
    {
        std::string material_id = element["material"]["id"].get<std::string>();
        assert(materials.count(material_id) > 0);
        _material = materials[material_id];
    }
    else
    {
        // what do we do in this case? idk. any materials should have been found in the prior stage.
        std::cout << "found misconfigured material for primitive with json " << element << '\n';
        _material = wrapped_error_material();
    }
    // primitive construction
    switch (get_primitive_type_for(element["type"].get<std::string>()))
    {
    case MESH:
        std::cout << "found MESH" << '\n';
        break;

    case SPHERE:
    {
        std::cout << "found SPHERE" << '\n';
        float r = 1.0;
        vec3 origin = vec3(0.0, 0.0, 0.0);
        if (element.contains("radius"))
        {
            r = element["radius"].get<float>();
        }
        if (element.contains("origin"))
        {
            origin = json_to_vec3(element["origin"]);
        }
        primitive = wrapped_hittable(new sphere(origin, r, _material._material), _material);
        break;
    }
    case RECT:
    {
        std::cout << "found RECT" << '\n';
        plane_enum align = XZ;
        bool flipped = element.value("flip", false);

        if (element.contains("align"))
        {
            std::cout << "set alignment to " << element["align"].get<std::string>() << '\n';
            align = plane_enum_mapping(element["align"].get<std::string>());
        }

        if (element.contains("a0") && element.contains("b0") && element.contains("a1") && element.contains("b1"))
        {
            float a0, b0, a1, b1, c;
            a0 = element["a0"].get<float>();
            b0 = element["b0"].get<float>();
            a1 = element["a1"].get<float>();
            b1 = element["b1"].get<float>();
            c = element["c"].get<float>();
            primitive = wrapped_hittable(new rect(a0, b0, a1, b1, c, _material.unwrap(), align, flipped), _material);
        }

        else
        {
            float a, b;
            if (element.contains("size"))
            {
                a = element["size"].at(0);
                b = element["size"].at(1);
            }
            else
            {
                a = 1.0;
                b = 1.0;
            }
            primitive = wrapped_hittable(new rect(a, b, _material.unwrap(), align, flipped), _material);
        }
        break;
    }
    case BOX:
    {
        std::cout << "found BOX" << '\n';
        if (element.contains("p0") && element.contains("p1"))
        {
            std::cout << "\tp0 and p1 path" << std::endl;
            vec3 p0, p1;
            p0 = json_to_vec3(element["p0"]);
            p1 = json_to_vec3(element["p1"]);
            primitive = wrapped_hittable(new box(p0, p1, _material.unwrap()), _material);
        }
        else
        {
            std::cout << "\tsize path" << std::endl;
            vec3 size;
            if (element.contains("size"))
            {
                size = json_to_vec3(element["size"]);
            }
            else
            {
                size = vec3(1, 1, 1);
            }
            primitive = wrapped_hittable(new box(size.x(), size.y(), size.z(), _material.unwrap()), _material);
        }
        break;
    }

    case VOLUME:
    {
        std::cout << "found VOLUME" << '\n';
        std::string primitive_id = element["primitive"].get<std::string>();
        float density = element["density"].get<float>();
        vec3 color;
        if (element.contains("color"))
        {
            color = json_to_vec3(element["color"]);
            // } else if (element.contains("texture")) {
        }
        else
        {
            std::cout << "unimplemented\n";
            color = MAUVE;
        }
        primitive = wrapped_hittable(new constant_medium(primitives[primitive_id].unwrap(), density, color), primitives[primitive_id].get_material());
        break;
    }

    default:
        break;
    }
    return primitive;
}

world *build_scene(json scene)
{
    std::vector<hittable *> list;
    std::vector<hittable *> lights;
    std::map<std::string, texture *> textures;
    std::map<std::string, wrapped_material> materials;
    std::map<std::string, wrapped_hittable> primitives;
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
            materials.emplace(mat_id, wrapped_error_material());
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
                std::cout << "color path\n";
                materials.emplace(mat_id, wrapped_material(new lambertian(json_to_vec3(data["color"])), "lambertian"));
            }
            else if (data.contains("texture"))
            {
                std::cout << "texture path\n";
                // construct the material from the reference to a texture
                materials.emplace(mat_id, wrapped_material(new lambertian(textures[data["texture"].get<std::string>()]), "lambertian"));
            }
            else
            {
                // default to using a mauve-like color to indicate an invalid specification, or the lack of a specification
                std::cout << "material misconfigured, check" << '\n';
                materials.emplace(mat_id, wrapped_error_material());
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

            materials.emplace(mat_id, wrapped_material(new metal(color, data.value("roughness", 0.0)), "metal"));
            break;
        }
        case DIELECTRIC:
        {
            std::cout << "found DIELECTRIC" << '\n';
            float ri = 1.450;
            if (data.contains("ior"))
            {
                ri = data["ior"].get<float>();
            }
            if (data.contains("color"))
            {
                std::cout << "colored dielectrics are currently unsupported\n";
            }
            materials.emplace(mat_id, wrapped_material(new dielectric(ri), "dielectric"));
            break;
        }
        case DIFFUSE_LIGHT:
        {
            std::cout << "found DIFFUSE_LIGHT" << '\n';
            if (element["data"].contains("texture"))
            {
                // set tint
                texture *_texture = textures[element["data"]["texture"]];
                materials.emplace(mat_id, wrapped_material(new diffuse_light(_texture), "diffuse_light"));
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
                materials.emplace(mat_id, wrapped_material(new diffuse_light(color), "diffuse_light"));
            }
            break;
        }
        default:
            break;
        }
    }

    // iterate through primitives and instances. these are the non-instanced base prims that can be reused.
    for (auto &element : scene["primitives"])
    {
        std::string primitive_id;
        if (!element.contains("id"))
        {
            primitive_id = generate_new_id();
        }
        else
        {
            primitive_id = element["id"].get<std::string>();
        }
        primitives.emplace(primitive_id, parse_prim_or_instance(primitives, materials, element));
    }
    std::cout << "finshed primitives read, scanning instances" << '\n';
    for (auto &element : scene["instances"])
    {
        if (element["type"].get<std::string>() == "ref")
        {
            // ignore refs
            continue;
        }

        // replace directs with references to the primitive mapping
        std::string primitive_id = generate_new_id();
        primitives.emplace(primitive_id, parse_prim_or_instance(primitives, materials, element["primitive"]));
        element["type"] = "ref";
        json new_prim_contents = {{"id", primitive_id}};
        // json new_prim = {"primitive", new_prim_contents};
        element.erase("primitive");
        element.emplace("primitive", new_prim_contents);
    }
    std::cout << "finished instance scan, now constructing instances from primitives\n";
    // iterate through normal instances, which are
    //      instanced primitives, i.e. primitives with a transform
    for (auto &element : scene["instances"])
    {
        if (element.value("skip", false))
        {
            continue;
        }

        std::cout << element << '\n';
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
        // std::cout << "finished constructing transform for instance, now getting prim and creating instance for json\n";

        assert(element.contains("type") && element["type"].get<std::string>() == "ref");

        assert(element["primitive"].contains("id"));
        std::string primitive_id = element["primitive"]["id"].get<std::string>();
        assert(primitives.count(primitive_id) > 0);
        wrapped_hittable primitive = primitives[primitive_id];

        // assert(element["primitive"].contains("id"));
        // std::string primitive_id = element["primitive"]["id"].get<std::string>();
        // assert(primitives.count(primitive_id) > 0);
        // material *_material = materials[material_id];
        std::string mat_type = primitive.get_material()._type;

        hittable *_instance = new instance(primitive.unwrap(), transform);
        list.push_back(_instance);
        if (mat_type == "diffuse_light")
        {
            std::cout << "found diffuse light\n";
            lights.push_back(_instance);
        }
    }

    texture *background;
    if (scene.contains("world"))
    {
        if (scene["world"].contains("texture"))
        {
            background = textures[scene["world"]["texture"].get<std::string>()];
        }
        else if (scene["world"].contains("color"))
        {
            background = new constant_texture(json_to_vec3(scene["world"]["color"]));
        }
    }
    else
    {
        background = new constant_texture(vec3(0.3, 0.3, 0.3));
    }

    // list.push_back(
    //     new instance(
    //         new constant_medium(new box(165, 165, 165, error_material()), 0.01, vec3(0.9, 0.9, 0.9)),
    //         transform3::from_rotate_and_translate(vec3(
    //                                                   0.0,
    //                                                   -0.1,
    //                                                   0.0),
    //                                               vec3(
    //                                                   212.5,
    //                                                   82.5,
    //                                                   147.5))));

    // iterate through objects which are collections of instances

    std::cout << "found " << lights.size() << " lights\n";
    return new world(new bvh_node(list.data(), list.size(), 0.0f, 0.0f), background, lights);
}

#endif
