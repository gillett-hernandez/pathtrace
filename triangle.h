#ifndef TRIANGLE_H
#define TRIANGLE_H

#include "hittable.h"

class triangle : public hittable
{
public:
    triangle(mesh *p_mesh, int tri_num) : p_mesh(p_mesh)
    {
        idx = p_mesh->v_indices[3 * tri_num];
    }
    virtual bool hit(const ray &r, float t_min, float t_max, hit_record &rec) const
    {
    }
    virtual bool bounding_box(float t0, float t1, aabb &box) const
    {
        vec3 p0, p1, p2;
        p0 = p_mesh->vertices[idx];
        p1 = p_mesh->vertices[idx + 1];
        p2 = p_mesh->vertices[idx + 2];
        box = aabb(p0, p1, false).extend(p2);
        return true;
    }

    int idx;
    mesh *p_mesh;
};

#endif
