#ifndef ENUMSH
#define ENUMSH

#include <map>

enum material_type
{
    LAMBERTIAN,
    METAL,
    DIELECTRIC,
    DIFFUSE_LIGHT
};

material_type get_material_type_for(std::string type)
{
    static std::map<std::string, material_type> mapping = {
        {"lambertian", LAMBERTIAN},
        {"metal", METAL},
        {"dielectric", DIELECTRIC},
        {"diffuse_light", DIFFUSE_LIGHT}};
    return mapping[type];
}

enum primitive_type
{
    MESH,
    SPHERE,
    RECT,
    CUBE
};

primitive_type get_primitive_type_for(std::string type)
{
    static std::map<std::string, primitive_type> mapping = {
        {"mesh", MESH},
        {"sphere", SPHERE},
        {"rect", RECT},
        {"cube", CUBE}};
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

#endif
