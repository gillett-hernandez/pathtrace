#pragma once

#include "hittable.h"
#include "primitive.h"
#include "bvh.h"
#include "vec3.h"
// #include "triangle.h"
#include "helpers.h"

class mesh;

class triangle : public hittable
{
public:
    triangle(mesh *, int);

    virtual bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const;
    virtual bool bounding_box(float t0, float t1, aabb &box) const;
    int idx;
    mesh *p_mesh;
};

class mesh : public hittable
{
public:
    mesh(int num_faces, std::vector<int> v_indices, std::vector<vec3> vertices, std::vector<vec3> normals, std::vector<material*> materials) : num_faces(num_faces), v_indices(v_indices), vertices(vertices), normals(normals), materials(materials)
    {
        for (size_t i = 0; i < v_indices.size(); i++)
        {
            int index = v_indices[i];
            triangles.push_back(new triangle(this, index));
        }

        bvh = new bvh_node((hittable **)triangles.data(), triangles.size(), 0.00001f, MAXFLOAT);
    };

    bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const
    {
        return bvh->hit(r, t_min, t_max, rec);
    }
    bool bounding_box(float t0, float t1, aabb &box) const
    {
        bvh->bounding_box(t0, t1, box);
        return true;
    }

    int num_faces;
    bvh_node *bvh;
    std::vector<triangle *> triangles;
    std::vector<int> v_indices;
    std::vector<vec3> vertices;
    // std::vector<int> n_indices;
    std::vector<vec3> normals;
    std::vector<material*> materials;
};

triangle::triangle(mesh *p_mesh, int tri_num) : p_mesh(p_mesh)
{
    idx = p_mesh->v_indices[3 * tri_num];
}

bool triangle::hit(const ray &r, float t_min, float t_max, hit_record &rec) const
{
    // get coordinates
    vec3 p0 = p_mesh->vertices[idx];
    vec3 p1 = p_mesh->vertices[idx + 1];
    vec3 p2 = p_mesh->vertices[idx + 2];
    // convert to ray space
    vec3 p0t = p0 - r.origin();
    vec3 p1t = p1 - r.origin();
    vec3 p2t = p2 - r.origin();
    // rearrange triangle dimensions and point coordinates based on ray orientation
    int kz = max_dimension(r.direction().abs());
    int kx = kz + 1;
    if (kx == 3)
    {
        kx = 0;
    }
    int ky = kx + 1;
    if (ky == 3)
    {
        ky = 0;
    }
    vec3 d = permute(r.direction(), kx, ky, kz);
    p0t = permute(p0t, kx, ky, kz);
    p1t = permute(p1t, kx, ky, kz);
    p2t = permute(p2t, kx, ky, kz);

    // apply shear
    float sx = -d[0] / d[2];
    float sy = -d[1] / d[2];
    float sz = 1.f / d[2];
    p0t[0] += sx * p0t[2];
    p0t[1] += sy * p0t[2];
    p1t[0] += sx * p1t[2];
    p1t[1] += sy * p1t[2];
    p2t[0] += sx * p2t[2];
    p2t[1] += sy * p2t[2];
    // get edge coefficients?
    float e0 = p1t[0] * p2t[1] - p1t[1] * p2t[0];
    float e1 = p2t[0] * p0t[1] - p2t[1] * p0t[0];
    float e2 = p0t[0] * p1t[1] - p0t[1] * p1t[0];
    // fallback to double precision edge test if any of the edges are of size "0"
    if (e0 == 0.0f || e1 == 0.0f || e2 == 0.0f)
    {
        double p2txp1ty = (double)p2t[0] * (double)p1t[1];
        double p2typ1tx = (double)p2t[1] * (double)p1t[0];
        e0 = (float)(p2typ1tx - p2txp1ty);
        double p0txp2ty = (double)p0t[0] * (double)p2t[1];
        double p0typ2tx = (double)p0t[1] * (double)p2t[0];
        e1 = (float)(p0typ2tx - p0txp2ty);
        double p1txp0ty = (double)p1t[0] * (double)p0t[1];
        double p1typ0tx = (double)p1t[1] * (double)p0t[0];
        e2 = (float)(p1typ0tx - p1txp0ty);
    }
    if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
    {
        return false;
    }
    float det = e0 + e1 + e2;
    if (det == 0)
    {
        return false;
    }

    p0t[2] *= sz;
    p1t[2] *= sz;
    p2t[2] *= sz;
    float t_scaled = e0 * p0t[2] + e1 * p1t[2] + e2 * p2t[2];
    if (det < 0 && (t_scaled >= 0 || t_scaled < t_max * det))
    {
        return false;
    }
    else if (det > 0 && (t_scaled <= 0 || t_scaled > t_max * det))
    {
        return false;
    }
    float invDet = 1 / det;
    float b0 = e0 * invDet;
    float b1 = e1 * invDet;
    float b2 = e2 * invDet;
    vec3 dp02 = p0 - p2;
    vec3 dp12 = p1 - p2;
    rec.p = b0 * p0 + b1 * p1 + b2 * p2;
    rec.t = t_scaled * invDet;
    rec.normal = cross(dp02, dp12).normalized();
    // rec.u = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];
    rec.u = 0.5;
    rec.v = 0.5;
    rec.mat_ptr = p_mesh->materials[0];
    return true;
}
bool triangle::bounding_box(float t0, float t1, aabb &box) const
{
    vec3 p0, p1, p2;
    p0 = p_mesh->vertices[idx];
    p1 = p_mesh->vertices[idx + 1];
    p2 = p_mesh->vertices[idx + 2];
    box = aabb(p0, p1, false).extend(p2);
    return true;
}
