#ifndef TRANSFORM3H
#define TRANSFORM3H
#include "thirdparty/Eigen/Dense"
#include "thirdparty/Eigen/Geometry"
#include "vec3.h"

typedef Eigen::Matrix<float, 3, 3> Matrix3f;
#define ONE vec3(1.0, 1.0, 1.0)
#define ZERO vec3(0.0, 0.0, 0.0)
typedef Eigen::Transform<float, 3, Eigen::Affine> Affine3f;
// typedef Eigen::Affine3f transform3;

class transform3
{
public:
    transform3(vec3 scale, vec3 rotate, vec3 translate)
    {
        auto t_scale = Eigen::Scaling(scale.x(), scale.y(), scale.z());
        auto t_rotate = (Eigen::AngleAxisf(rotate.x() * M_PI, Vector3f::UnitX()) * Eigen::AngleAxisf(rotate.y() * M_PI, Vector3f::UnitY()) * Eigen::AngleAxisf(rotate.z() * M_PI, Vector3f::UnitZ()));
        auto t_translate = Eigen::Translation<float, 3>(translate.as_eigen_vector());
        _transform = t_scale * t_rotate * t_translate;
    }
    static transform3 from_rotate_and_translate(vec3 rotate, vec3 translate)
    {
        return transform3(ONE, rotate, translate);
    }
    static transform3 from_scale_and_translate(vec3 scale, vec3 translate)
    {
        return transform3(scale, ZERO, translate);
    }
    static transform3 from_scale_and_rotate(vec3 scale, vec3 rotate)
    {
        return transform3(scale, rotate, ZERO);
    }
    static transform3 from_scale(vec3 scale)
    {
        return transform3(scale, ZERO, ZERO);
    }
    static transform3 from_rotate(vec3 rotate)
    {
        return transform3(ONE, rotate, ZERO);
    }
    static transform3 from_translate(vec3 translate)
    {
        return transform3(ONE, ZERO, translate); //Eigen::Translation<float, 3>(translate.as_eigen_vector()));
    }
    transform3()
    {
        // return transform3(ONE, ZERO, ZERO);
    }
    Eigen::Affine3f _transform;
};

#endif
