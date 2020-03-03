#include "vec3.h"
#include "ray.h"
#include "camera.h"

class manager
{
public:
    // should return the next ray to fire into the scene
    virtual ray next() = 0;
    // should record color data onto film
    virtual void record(vec3 color) = 0;
    // should advance internal data and return true if done
    virtual bool advance() = 0;
    vec3 **framebuffer;
    int samples_completed, samples_remaining;
};

class progressive : public manager
{
public:
    progressive(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
    {
        j = height - 1;
        i = 0;
        s = samples;
    }
    ray get_ray()
    {
        float u = float(i + random_double()) / float(width);
        float v = float(j + random_double()) / float(height);
        ray r = cam->get_ray(u, v);
        return r;
    }
    bool advance()
    {
        // advance horizontally
        i++;
        if (i >= width)
        {
            // if reached end, reset i to beginning, and go down a line
            i = 0;
            j--;
        }
        if (j < 0)
        {
            // if reached bottom, reset j to top and decrease remaning samples
            j = height - 1;
            s--;
        }
        if (s < 0)
        {
            // if samples remaining is 0, quit
            return true;
        }
        return false;
    }
    void record(vec3 color)
    {
        if (s > 0)
        {
            color_buffer += color;
        }
        else
        {
            framebuffer[j][i] += color_buffer;
        }
    }
    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
    // the color buffer, a single pixel's value is stored here before being inserted into the framebuffer
    vec3 color_buffer;
    int i, j, s;
};

class naive : public manager
{
public:
    naive(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
    {
        j = height - 1;
        i = 0;
        s = samples;
    }
    ray get_ray()
    {
        float u = float(i + random_double()) / float(width);
        float v = float(j + random_double()) / float(height);
        ray r = cam->get_ray(u, v);
        return r;
    }
    bool advance()
    {
        if (s <= 0)
        {
            // if no more remaining samples for this pixel, reset samples counter and increase width
            s = samples;
            i++;
        }
        // decrease remaining samples for pixel
        s--;
        if (i >= width)
        {
            // if width reached, reset and go a line down
            i = 0;
            j--;
        }
        if (j < 0)
        {
            // if bottom reached, done
            return true;
        }
        return false;
    }
    void record(vec3 color)
    {
        if (s > 0)
        {
            color_buffer += color;
        }
        else
        {
            framebuffer[j][i] += color_buffer;
        }
    }
    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
    // the color buffer, a single pixel's value is stored here before being inserted into the framebuffer
    vec3 color_buffer;
    int i, j, s;
};

class tiled : public manager
{
public:
    tiled(camera *cam, vec3 **framebuffer, float width, float height, int samples) : cam(cam), framebuffer(framebuffer), width(width), height(height), samples(samples)
    {
        j = height - 1;
        i = 0;
        s = samples;
    }
    ray get_ray()
    {
        float u = float(i + random_double()) / float(width);
        float v = float(j + random_double()) / float(height);
        ray r = cam->get_ray(u, v);
        return r;
    }
    bool advance()
    {

        s--;
        if (s < 0)
        {
            s = samples;
            i++;
        }
        if (i >= width)
        {
            i = 0;
            j--;
        }
        if (j < 0)
        {
            j = height - 1;
        }
    }
    void record(vec3 color)
    {
        if (s > 0)
        {
            color_buffer += color;
        }
        else
        {
            framebuffer[j][i] += color_buffer;
        }
    }
    camera *cam;
    int width, height, samples;

private:
    // the framebuffer for the screen
    vec3 **framebuffer;
    // the color buffer, a single pixel's value is stored here before being inserted into the framebuffer
    vec3 color_buffer;
    int i, j, s;
};
