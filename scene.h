#pragma once
#include "vec3.h"
#include "material.h"
#include "primitive.h"
#include <vector>
#include <map>

typedef vec3 color;

enum material_type
{
    LAMBERTIAN,
    METAL,
    DIELECTRIC,
    DIFFUSE_LIGHT,
    ISOTROPIC
};

material_type get_material_type_for(std::string type)
{
    static std::map<std::string, material_type> mapping = {
        {"lambertian", LAMBERTIAN},
        {"metal", METAL},
        {"dielectric", DIELECTRIC},
        {"isotropic", ISOTROPIC},
        {"diffuse_light", DIFFUSE_LIGHT}};
    return mapping[type];
}

enum primitive_type
{
    MESH,
    SPHERE,
    RECT,
    BOX,
    VOLUME
};

primitive_type get_primitive_type_for(std::string type)
{
    static std::map<std::string, primitive_type> mapping = {
        {"mesh", MESH},
        {"sphere", SPHERE},
        {"rect", RECT},
        {"volume", VOLUME},
        {"box", BOX}};
    return mapping[type];
}

enum instance_type
{
    REF,
    DIRECT
};

instance_type get_instance_type_for(std::string type)
{
    static std::map<std::string, instance_type> mapping = {
        {"ref", REF},
        {"direct", DIRECT}};
    return mapping[type];
}

enum texture_type
{
    CONSTANT,
    CHECKERED,
    PERLIN
};

texture_type get_texture_type_for(std::string type)
{
    static std::map<std::string, texture_type> mapping = {
        {"constant", CONSTANT},
        {"checker", CHECKERED},
        {"perlin", PERLIN}};
    return mapping[type];
}

enum world_background_type
{
    HDRI,
    TEXTURE,
    GRADIENT
};

world_background_type get_world_background_type_for(std::string type)
{
    static std::map<std::string, world_background_type> mapping = {
        {"hdri", HDRI},
        {"texture", TEXTURE},
        {"gradient", GRADIENT}};
    return mapping[type];
}

struct s_camera
{
    vec3 look_from;
    vec3 look_at;
    float fov;
    float aperture;
    float dist_to_focus;
};

struct s_world
{
    world_background_type type;
    color background_color;
};

struct s_material
{
    material_type type;
    std::string id;
    material *p_material;
};

struct s_primitive
{
    primitive_type type;
    std::string id;
    hittable *p_primitive;
};

struct s_instance
{
    instance_type type;
};

struct scene
{
    s_camera camera;
    s_world world;
    std::vector<s_material> materials;
    std::vector<s_primitive> primitives;
    std::vector<s_instance> instances;
};