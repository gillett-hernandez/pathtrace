#ifndef MESHH
#define MESHH

#include "hittable.h"
#include "primitive.h"
#include "triangle.h"
#include "helpers.h"

class mesh // : public hittable
{
public:
    mesh(){};
    // mesh(int num_faces, list<int> v_indices, list<vec3> vertices, list<int> n_indices, list<vec3> normals) : num_faces(num_faces), v_indices(v_indices), vertices(vertices), n_vertices(n_vertices), normals(normals){};
    mesh(int num_faces, list<int> v_indices, list<vec3> vertices, list<vec3> normals) : num_faces(num_faces), v_indices(v_indices), vertices(vertices), normals(normals){};
    // uvs() const
    mesh apply(transform3)
    {
        // apply transform to vertices
        // apply transform to normals
    }
    int num_faces;
    // int n_faces;
    list<int> v_indices;
    list<vec3> vertices;
    // list<int> n_indices;
    list<vec3> normals;
    material *materials;
};

// class instanced_mesh : public bvh_node
// {
//     instanced_mesh(mesh *primitive, transform3 transform)
//     {
//         // instanced = mesh(*primitive);
bvh_node *instance_mesh(mesh *primitive)
{
    std::vector<triangle *> tris;
    for (int i = 0; i < primitive->v_indices.length; i++)
    {
        int idx = primitive->v_indices[i];
        tris.push_back(new triangle(primitive, idx));
    }
    return new bvh_node(tris.data(), tris.size(), 0.0, 1.0);
}
//     }
//     mesh *primitive;
//     // mesh instanced;
// };

#endif
