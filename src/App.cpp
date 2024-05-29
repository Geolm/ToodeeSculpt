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

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) 
    {
        App* user_ptr = (App*) glfwGetWindowUserPointer(window);
        user_ptr->OnKeyEvent(key, scancode, action, mods);
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
    {
        App* user_ptr = (App*) glfwGetWindowUserPointer(window);
        user_ptr->OnWindowResize(width, height);
    });
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Update(CA::MetalDrawable* drawable)
{
    m_Renderer.BeginFrame();

    int seed = 0x12345678;
    for(uint32_t i=0; i<10000; i++)
    {
        m_Renderer.DrawCircleFilled(iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight), 25.f, 0x7fc0c741);

        m_Renderer.DrawLine(iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight),
                            iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight), 5.f, 0x7fc0c741);
    }

    m_Renderer.EndFrame();
    m_Renderer.Flush(drawable);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnKeyEvent(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        m_Renderer.ReloadShaders();
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnWindowResize(int width, int height)
{
    m_ViewportWidth = (uint32_t) width;
    m_ViewportHeight = (uint32_t) height;
    m_Renderer.Resize(m_ViewportWidth, m_ViewportHeight);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    m_Renderer.Terminate();
}
