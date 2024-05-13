#pragma once

#include "Renderer/Renderer.h"

struct GLFWwindow;

class App
{
public:
    void Init(MTL::Device* device, GLFWwindow* window);
    void Update(CA::MetalDrawable* drawable);
    void Terminate();

private:
    MTL::Device* m_Device;
    Renderer m_Renderer;
    GLFWwindow* m_Window;
};