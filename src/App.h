#pragma once

#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

class App
{
public:
    void Init(MTL::Device* device, GLFWwindow* window);
    void Loop();
    void Terminate();

private:
    MTL::Device* m_Device;
    GLFWwindow* m_Window;
};