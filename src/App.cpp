#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

#include "system/random.h"

//----------------------------------------------------------------------------------------------------------------------------
void App::Init(MTL::Device* device, GLFWwindow* window)
{
    m_Device = device;
    m_Window = window;
    
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    m_ViewportHeight = (uint32_t) height;
    m_ViewportWidth = (uint32_t) width;

    m_Renderer.Init(m_Device, m_ViewportWidth, m_ViewportHeight);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Update(CA::MetalDrawable* drawable)
{
    m_Renderer.BeginFrame();

    int seed = 0x12345678;

    for(uint32_t i=0; i<1000; i++)
    {
        m_Renderer.DrawCircleFilled(iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight), 25.f, 0x7fc0c741);
    }
    
    m_Renderer.EndFrame();
    m_Renderer.Flush(drawable);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    m_Renderer.Terminate();
}
