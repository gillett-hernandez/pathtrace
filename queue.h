#pragma once
#include "helpers.h"
#include <queue>
#include <mutex>
#include <condition_variable>

// A threadsafe-queue.
template <class T>
class SafeQueue
{
public:
    SafeQueue(void)
        : q(), m(), c()
    {
    }

    ~SafeQueue(void)
    {
    }

    // Add an element to the queue.
    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m);
        q.push(t);
        c.notify_one();
    }

    // Get the "front"-element.
    // If the queue is empty, wait till a element is avaiable.
    T dequeue(void)
    {
        std::unique_lock<std::mutex> lock(m);
        while (q.empty())
        {
            // release lock as long as the wait and reaquire it afterwards.
            c.wait(lock);
        }
        T val = q.front();
        q.pop();
        return val;
    }

    bool empty(void)
    {
        return q.empty();
    }

private:
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;
};


class Spiral
{
public:
    virtual std::pair<std::pair<int, int>, std::pair<int, int>> next() = 0;
    virtual bool is_empty() = 0;

    SafeQueue<std::pair<int, int>> queue;
};

class NaiveSpiral : public Spiral
{
public:
    NaiveSpiral(int width, int height, int block_width, int block_height) : queue(), width(width), height(height), block_width(block_width), block_height(block_height)
    {
        // queue = spiral_queue(new SafeQueue<std::pair<int, int>>(), (int)std::ceil((float)width / block_width), (int)std::ceil((float)height / block_height));
        int tiles_width = (int)std::ceil((float)width / block_width);
        int tiles_height = (int)std::ceil((float)height / block_height);
        int radius = 1;
        int start_x = -1;
        int start_y = -1;
        int x = start_x == -1 ? (tiles_width % 2 == 0 ? (tiles_width / 2 - 1) : (tiles_width / 2)) : start_x;
        int y = start_y == -1 ? (tiles_height % 2 == 0 ? (tiles_height / 2 - 1) : (tiles_height / 2)) : start_y;

        // get largest of the two dimensions
        int furthest = max(tiles_width, tiles_height);
        int dir_x = 1;
        int dir_y = 0;
        int count = radius;
        while (radius <= furthest)
        {
            if (x >= 0 && y >= 0 && x < tiles_width && y < tiles_height)
            {
                queue.enqueue(std::pair<int, int>(x, y));
            }
            x += dir_x;
            y += dir_y;
            count--;
            if (count <= 0)
            {
                if (dir_x == 0 && dir_y == 1)
                {
                    dir_x = -1;
                    dir_y = 0;
                    radius++;
                }
                else if (dir_x == 1 && dir_y == 0)
                {
                    dir_x = 0;
                    dir_y = 1;
                }
                else if (dir_x == -1 && dir_y == 0)
                {
                    dir_x = 0;
                    dir_y = -1;
                }
                else if (dir_x == 0 && dir_y == -1)
                {
                    dir_x = 1;
                    dir_y = 0;
                    radius++;
                }
                count = radius;
            }
        }
    }
    std::pair<std::pair<int, int>, std::pair<int, int>> next()
    {
        std::pair<int, int> topleft_in_block_coordinates = queue.dequeue();
        std::pair<int, int> topleft = std::pair<int, int>(topleft_in_block_coordinates.first * block_width, topleft_in_block_coordinates.second * block_height);
        std::pair<int, int> bottomright = std::pair<int, int>(min(topleft.first + block_width, width), min(topleft.second + block_height, height));
        return std::pair<std::pair<int, int>, std::pair<int, int>>(topleft, bottomright);
    }

    bool is_empty()
    {
        return queue.empty();
    }
    int block_width;
    int block_height;
    int width;
    int height;
    SafeQueue<std::pair<int, int>> queue;
};

// class HilbertSpiral : public Spiral
// {
// public:
//     HilbertSpiral(int width, int height, int block_width, int block_height) : queue(), width(width), height(height), block_width(block_width), block_height(block_height)
//     {
//         spiral_queue(std::shared_ptr<SafeQueue<std::pair<int, int>>>(&queue), (int)std::ceil((float)width / block_width), (int)std::ceil((float)height / block_height));
//     }
//     std::pair<std::pair<int, int>, std::pair<int, int>> next()
//     {
//         std::pair<int, int> topleft_in_block_coordinates = queue.dequeue();
//         std::pair<int, int> topleft = std::pair<int, int>(topleft_in_block_coordinates.first * block_width, topleft_in_block_coordinates.second * block_height);
//         std::pair<int, int> bottomright = std::pair<int, int>(min(topleft.first + block_width, width), min(topleft.second + block_height, height));
//         return std::pair<std::pair<int, int>, std::pair<int, int>>(topleft, bottomright);
//     }
//     int block_width;
//     int block_height;
//     int width;
//     int height;

//     SafeQueue<std::pair<int, int>> queue;
// };