#pragma once

#define TINYOBJLOADER_IMPLEMENTATION
#include "thirdparty/tiny_obj_loader.h"

#include "hittable.h"
#include "material.h"
#include "mesh.h"
#include "vec3.h"

std::vector<mesh *> load_asset(std::string filepath, std::string basedir)
{
    std::string inputfile = filepath;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str(), basedir.c_str(), true);

    if (!warn.empty())
    {
        std::cout << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << err << std::endl;
    }

    if (!ret)
    {
        exit(1);
    }

    std::vector<mesh *> meshes = std::vector<mesh *>();
    int material_id = 0;
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // determine x, y, z span of the shape, as well as the computed origin/median point
        std::vector<int> v_indices = std::vector<int>();
        std::vector<vec3> vertices = std::vector<vec3>();
        std::vector<int> n_indices = std::vector<int>();
        std::vector<vec3> normals = std::vector<vec3>();
        std::vector<int> material_ids = std::vector<int>();
        std::cout << "found shape, parsing...\n";
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int fv = shapes[s].mesh.num_face_vertices[f];

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                if (attrib.texcoords.size() > 2 * idx.texcoord_index + 1)
                {
                    tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    // add texcoords
                }
                else
                {
                    //don't add, or use default
                }
                // Optional: vertex colors
                // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
                // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
                // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
                v_indices.push_back(idx.vertex_index);
                vertices.push_back(vec3(vx, vy, vz));
                n_indices.push_back(idx.normal_index);
                normals.push_back(vec3(nx, ny, nz));
            }
            index_offset += fv;

            // per-face material
            material_ids.push_back(shapes[s].mesh.material_ids[f]);
        }
        vec3 min = vec3(MAXFLOAT, MAXFLOAT, MAXFLOAT);
        vec3 max = vec3(-MAXFLOAT, -MAXFLOAT, -MAXFLOAT);
        for (int i = 0; i < attrib.vertices.size(); i++)
        {
            auto v = vertices[i];
            for (int j = 0; j < 3; j++)
            {
                // if (min[j] > v[j])
                // {
                //     min[j] = v[j];
                // }
                min[j] = std::min(min[j], v[j]);
                // if (max[j] < v[j])
                // {
                //     max[j] = v[j];
                // }
                max[j] = std::max(max[j], v[j]);
            }
        }
        std::cout << "mesh bounds are " << min << " to " << max << '\n';
        mesh *temp_mesh = new mesh((int)shapes[s].mesh.num_face_vertices.size(), v_indices, vertices, n_indices, normals, material_ids);
        meshes.push_back(temp_mesh);
    }
    return meshes;
}
std::vector<mesh *> load_asset(std::string filepath)
{
    return load_asset(filepath, "assets/");
}
