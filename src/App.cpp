#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

//----------------------------------------------------------------------------------------------------------------------------
void App::Init(MTL::Device* device, GLFWwindow* window)
{
    m_Device = device;
    m_Window = window;

    m_Renderer.Init(m_Device);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Update(CA::MetalDrawable* drawable)
{
    m_Renderer.Draw(drawable);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    m_Renderer.Terminate();
}
