#pragma once

#include "renderer/Renderer.h"

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
    uint32_t m_ViewportHeight;
    uint32_t m_ViewportWidth;
};
