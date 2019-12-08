#ifndef TRANSFORM3H
#define TRANSFORM3H
#include "thirdparty/Eigen/Dense"
#include "thirdparty/Eigen/Geometry"

typedef Eigen::Matrix<float, 3, 3> Matrix3f;
// static vec3 ONE = vec3(1.0, 1.0, 1.0);
// static vec3 ZERO = vec3(0.0, 0.0, 0.0);
// typedef Eigen::Transform<float, 3, Eigen::Affine> transform3;
typedef Eigen::Affine3f transform3;

// class transform3
// {
// public:
//     transform3(vec3 scale, vec3 rotate, vec3 translate) {
//     }
//     transform3 from_scale(vec3 scale)
//     {
//         return transform3(scale, vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 0.0));
//     }
//     transform3 from_rotation(vec3 rotate)
//     {
//         return transform3(vec3(1.0, 1.0, 1.0), rotate, vec3(0.0, 0.0, 0.0));
//     }
//     transform3 from_translation(vec3 translate)
//     {
//         return transform3(ONE, ZERO, translate);
//     }
//     transform3 from_scale_and_rotate(vec3 scale, vec3 rotate)
//     {
//         return transform3(scale, rotate, ZERO);
//     }
//     transform3 from_scale_and_translate(vec3 scale, vec3 translate)
//     {
//         return transform3(scale, ZERO, translate);
//     }
//     transform3 from_rotate_and_translate(vec3 rotate, vec3 translate)
//     {
//         return transform3(ONE, rotate, translate);
//     }
// };

#endif
