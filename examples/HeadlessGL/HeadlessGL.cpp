// HeadlessGL.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#define GLFW_INCLUDE_NONE
#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "wander.h"


#ifdef _WIN32

#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "glfw/lib-vc2022/glfw3.lib")

extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}

extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#ifdef _WIN64
// TODO - move to wander lib
#pragma comment(lib, "wasmtime.dll.lib")
#endif

#endif


int main()
{
    std::cout << "Hello World!\n";
}
