#pragma once
#include <map>

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

enum plane_enum
{
    XY,
    XZ,
    YZ
};

plane_enum plane_enum_mapping(std::string alignment)
{
    static std::map<std::string, plane_enum> mapping = {
        {"xy", XY},
        {"xz", XZ},
        {"yz", YZ}};
    return mapping[alignment];
}

