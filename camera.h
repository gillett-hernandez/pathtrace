#ifndef CAMERAH
#define CAMERAH

#include "ray.h"
#include "helpers.h"

class camera
{
public:
    camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect,
           float aperture, float focus_dist, float t0, float t1)
    {
        lens_radius = aperture / 2;
        // vfov is top to bottom in degrees
        // vec3 u, v, w;
        float theta = vfov * M_PI / 180;
        float half_height = tan(theta / 2);
        float half_width = aspect * half_height;
        origin = lookfrom;
        w = unit_vector(lookfrom - lookat);
        std::cout << "w " << w << std::endl;
        u = unit_vector(cross(vup, w));
        std::cout << "u " << u << std::endl;
        v = cross(w, u);
        std::cout << "v " << v << std::endl;
        lower_left_corner = origin - half_width * focus_dist * u - half_height * focus_dist * v - focus_dist * w;
        std::cout << lower_left_corner << std::endl;

        horizontal = 2 * half_width * focus_dist * u;
        std::cout << horizontal << std::endl;
        vertical = 2 * half_height * focus_dist * v;
        std::cout << vertical << std::endl;
    }

    ray get_ray(float s, float t)
    {
        // circular sensor?
        vec3 rd = lens_radius * random_in_unit_disk();
        vec3 offset = u * rd.x() + v * rd.y();
        float time = time0 + random_double() * (time1 - time0);
        return ray(origin + offset,
                   lower_left_corner + s * horizontal + t * vertical - origin - offset,
                   time);
    }

    void project(vec3 point, float &x, float &y)
    {
        // project a point through the camera and get the x and y values
        // this completely disregards lens effects.
        // this should be reimplemented for other lenses
        // TODO: refactor this class to an abstract base class + thinlens class
        // for now, project onto uv plane

        // formula for projecting onto a plane is:
        // ray r = ray(point, origin - point);
        // camera_ray = ray(origin + offset,
        //            lower_left_corner + s * horizontal + t * vertical - origin - offset,
        //            time);
        // camera_ray : origin + (lower_left_corner + x*horizontal + y*vertical - origin)*t
        // origin + (lower_left_corner + x*horizontal + y*vertical - origin)*t = point
        // vec3 pt = (point - origin) / t - lower_left_corner + origin;
        // x = dot(pt, horizontal);
        // y = dot(pt, vertical);
        // plane is defined from horizontal and vertical
        // ray is defined from
        // (ro + rd * t - p_0) = 0
        // vec3 p_0 = lower_left_corner;

        ray r = ray(point, origin - point);

        float a = dot(lower_left_corner - r.origin(), w);
        float b = dot(r.direction(), w);

        float t = a / b;
        // if (t < 0.0)
        // {
        //     t = -t;
        // }
        std::cout << " point: " << point << std::endl;
        // std::cout << " u:" << u << " v:" << v;
        // std::cout << " direction: " << r.direction() << std::endl;
        // std::cout << " a:" << a << " b:" << b;
        // std::cout << " t:" << t;
        vec3 p = r.point_at_parameter(t) - lower_left_corner;
        std::cout << " p:" << p;
        // p is point in uv plane
        x = dot(horizontal, p) / horizontal.squared_length();
        y = dot(vertical, p) / vertical.squared_length();
        std::cout << " x:" << x;
        std::cout << " y:" << y << '\n';
        std::cout << std::endl;
    }

    vec3 origin;
    vec3 lower_left_corner;
    vec3 horizontal;
    vec3 vertical;
    vec3 u, v, w;
    float lens_radius;
    float time0, time1;
};
#endif
