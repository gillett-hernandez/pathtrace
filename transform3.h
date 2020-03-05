#pragma once
#include "thirdparty/Eigen/Dense"
#include "thirdparty/Eigen/Geometry"
#include "vec3.h"

// typedef Eigen::Matrix<float, 4, 4> Matrix4f;
// using Matrix4f = Eigen::Matrix4f;
using Matrix3f = Eigen::Matrix3f;
#define ONE vec3(1.0, 1.0, 1.0)
#define ZERO vec3(0.0, 0.0, 0.0)
// typedef Eigen::Transform<float, 4, Eigen::Affine> Affine4f;
using Affine3f = Eigen::Affine3f;

class transform3
{
public:
    // transform3() {}
    transform3(Eigen::Affine3f transform) : _transform(transform) {}
    transform3(vec3 scale = ONE, vec3 rotate = ZERO, vec3 translate = ZERO)
    {
        auto t_scale = Eigen::Scaling(scale.x(), scale.y(), scale.z());
        auto t_rotate = Eigen::AngleAxisf(rotate.x() * M_PI, Vector3f::UnitX()) * Eigen::AngleAxisf(rotate.y() * M_PI, Vector3f::UnitY()) * Eigen::AngleAxisf(rotate.z() * M_PI, Vector3f::UnitZ());
        auto t_translate = Eigen::Translation<float, 3>(translate.as_eigen_vector3());
        _transform = t_translate * t_rotate * t_scale;
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
        return transform3(scale, rotate);
    }
    static transform3 from_scale(vec3 scale)
    {
        return transform3(scale);
    }
    static transform3 from_rotate(vec3 rotate)
    {
        return transform3(ONE, rotate);
    }
    static transform3 from_translate(vec3 translate)
    {
        return transform3(ONE, ZERO, translate);
    }

    transform3 inverse() const
    {
        return transform3(_transform.inverse());
    }

    vec3 apply_linear(vec3 v) const
    {
        return vec3((Vector3f)(_transform.linear() * v.as_eigen_vector3()));
    }
    vec3 apply_normal(vec3 v) const
    {
        return vec3((Vector3f)((_transform.linear().inverse().transpose() * v.as_eigen_vector3()).normalized()));
    }

    inline vec3 operator*(vec3 vec) const
    {
        return vec3(_transform * vec.as_eigen_vector3());
    }
    Eigen::Affine3f _transform;
};
