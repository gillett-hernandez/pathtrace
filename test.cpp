#include <iostream>
#include "scene.h"
#include "mesh_loader.h"
#include "mesh.h"

int main(int argc, char const *argv[])
{
    auto ret = load_asset("assets/monkey.obj");
    std::cout << ret.size() << std::endl;
    return 0;
}
