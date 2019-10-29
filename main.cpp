#include "float.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "random.h"
#include "helpers.h"
#include "bvh.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>


#define MAX_BOUNCES 30
#define N_THREADS 4


class lambertian : public material {
    public:
        lambertian(const vec3& a) : albedo(a) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec,
                             vec3& attenuation, ray& scattered) const {
            vec3 target = rec.p + rec.normal + random_in_unit_sphere();
            scattered = ray(rec.p, target-rec.p, r_in.time());
            attenuation = albedo;
            return true;
        }

        vec3 albedo;
};


class metal : public material {
    public:
        metal(const vec3& a, float f) : albedo(a) {
            if (f < 1) fuzz = f; else fuzz = 1;
        }
        virtual bool scatter(const ray& r_in, const hit_record& rec,
                             vec3& attenuation, ray& scattered) const {
            vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
            scattered = ray(rec.p, reflected);
            attenuation = albedo;
            return (dot(scattered.direction(), rec.normal) > 0);
        }
        vec3 albedo;
        float fuzz;
};

class dielectric : public material {
    public:
        dielectric(float ri) : ref_idx(ri) {}
        virtual bool scatter(const ray& r_in, const hit_record& rec,
                             vec3& attenuation, ray& scattered) const {
            vec3 outward_normal;
            vec3 reflected = reflect(r_in.direction(), rec.normal);
            float ni_over_nt;
            attenuation = vec3(1.0, 1.0, 1.0);
            vec3 refracted;

            float reflect_prob;
            float cosine;

            if (dot(r_in.direction(), rec.normal) > 0) {
                outward_normal = -rec.normal;
                ni_over_nt = ref_idx;
                cosine = ref_idx * dot(r_in.direction(), rec.normal)
                        / r_in.direction().length();
            }
            else {
                outward_normal = rec.normal;
                ni_over_nt = 1.0 / ref_idx;
                cosine = -dot(r_in.direction(), rec.normal)
                        / r_in.direction().length();
            }

            if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
                reflect_prob = schlick(cosine, ref_idx);
            }
            else {
                reflect_prob = 1.0;
            }

            if (random_double() < reflect_prob) {
                scattered = ray(rec.p, reflected);
            }
            else {
                scattered = ray(rec.p, refracted);
            }

            return true;
        }

        float ref_idx;
};

vec3 color(const ray& r, hittable *world, int depth) {
    hit_record rec;
    if (world->hit(r, 0.001, MAXFLOAT, rec)) {
        ray scattered;
        vec3 attenuation;
        if (depth < MAX_BOUNCES && rec.mat_ptr->scatter(r, rec, attenuation, scattered)) {
            return attenuation*color(scattered, world, depth+1);
        }
        else {
            return vec3(0,0,0);
        }
    }
    else {
        // world background color here
        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5*(unit_direction.y() + 1.0);
        return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
    }
}

hittable *random_scene() {
    int n = 500;
    hittable **list = new hittable*[n+1];
    list[0] =  new sphere(vec3(0,-1000,0), 1000, new lambertian(vec3(0.5, 0.5, 0.5)));
    int i = 1;
    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            float choose_mat = random_double();
            vec3 center(a+0.9*random_double(),0.2,b+0.9*random_double());
            if ((center-vec3(4,0.2,0)).length() > 0.9) {
                if (choose_mat < 0.8) {  // diffuse
                    list[i++] = new sphere(center, 0.2,
                        new lambertian(vec3(random_double()*random_double(),
                                            random_double()*random_double(),
                                            random_double()*random_double())
                        )
                    );
                }
                else if (choose_mat < 0.95) { // metal
                    list[i++] = new sphere(center, 0.2,
                            new metal(vec3(0.5*(1 + random_double()),
                                           0.5*(1 + random_double()),
                                           0.5*(1 + random_double())),
                                      0.5*random_double()));
                }
                else {  // glass
                    list[i++] = new sphere(center, 0.2, new dielectric(1.5));
                }
            }
        }
    }

    list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
    list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(vec3(0.4, 0.2, 0.1)));
    list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

    return new bvh_node(list,i, 0.0f, 0.0f);
}


std::mutex framebuffer_lock;

void compute_rays(vec3** buffer, int nx, int ny, int ns, camera cam, hittable* world) {

    for (int j = ny-1; j >= 0; j--) {
        // std::cerr << "computing row " << j << std::endl;
        for (int i = 0; i < nx; i++) {
            // std::cerr << "computing column " << i << std::endl
            vec3 col = vec3(0, 0, 0);
            for (int s = 0; s < ns; s++) {
                float u = float(i + random_double()) / float(nx);
                float v = float(j + random_double()) / float(ny);
                ray r = cam.get_ray(u, v);
                col += color(r, world, 0);
            }

            framebuffer_lock.lock();
            buffer[j][i] += col;
            framebuffer_lock.unlock();
        }
    }
}



int main() {
    int nx = 1920;
    int ny = 1080;
    int ns = 20;
    // round up to nearest multiple of N_THREADS
    ns += N_THREADS;
    ns -= ns % N_THREADS;
    vec3** framebuffer = new vec3*[ny];
    for (int j = ny-1; j >= 0; j--) {
        // std::cerr << "computed row " << j << std::endl;
        framebuffer[j] = new vec3[nx];
        for (int i = 0; i < nx; i++) {
            framebuffer[j][i] = vec3(0, 0, 0);
        }
    }


    // x,y,z
    // y is up.
    auto t1 = std::chrono::high_resolution_clock::now();
    hittable *list[3];
    int i = 0;
    list[i++] = new sphere(vec3(0,-100,0), 100, new lambertian(vec3(0.2, 0.2, 0.2)));
    list[i++] = new sphere(vec3(1,1,0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    list[i++] = new sphere(vec3(-1,1,0), 1.0, new metal(vec3(1.0, 1.0, 1.0), 0.05));
    hittable *world = new bvh_node(list, i, 0.0f, 0.0f);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds = t2-t1;
    std::cerr << "time taken to build bvh " << elapsed_seconds.count() << std::endl; 
    // hittable *world = random_scene();
    vec3 lookfrom(0,1,5);
    vec3 lookat(0,1,0);
    float dist_to_focus = (lookfrom-lookat).length();
    float aperture = 0.005;
    int fov = 40;

    camera cam(lookfrom, lookat, vec3(0,1,0), fov,
           float(nx)/float(ny), aperture, dist_to_focus, 0.0f, 0.0f);
    int pixels = 0;
    int total_pixels = nx * ny;
    std::thread threads[N_THREADS];

    for (int t = 0; t < N_THREADS; t++) {
        std::cerr << "spawning thread " << t << std::endl;
        threads[t] = std::thread(compute_rays, std::ref(framebuffer), nx, ny, ns/N_THREADS, cam, world);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds2 = t3-t2;
    std::cerr << "time taken to setup the rest and spawn threads " << elapsed_seconds2.count() << std::endl; 
    for (int t = 0; t < N_THREADS; t++) {
        std::cerr << "joining thread " << t << std::endl;
        threads[t].join();
        std::cerr << "joined thread " << t << std::endl;
    }
    auto t4 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds3 = t4-t3;
    std::cerr << "time taken to compute " << elapsed_seconds3.count() << std::endl;

    std::cerr << "computed " << total_pixels * ns << " rays in " << elapsed_seconds3.count() << "seconds, at " << total_pixels*ns/elapsed_seconds3.count() << " rays per second" << std::endl;

    std::cout << "P6\n" << nx << " " << ny << "\n255\n";
    for (int j = ny-1; j >= 0; j--) {
        // std::cerr << "computed row " << j << std::endl;
        for (int i = 0; i < nx; i++) {
            vec3 col = framebuffer[j][i];
            col /= float(ns);

            col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]) );

            char ir = int(255.99*col[0]);
            char ig = int(255.99*col[1]);
            char ib = int(255.99*col[2]);
            std::cout << ir <<  ig <<  ib;
        }
    }
}
