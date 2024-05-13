#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#include "Metal.hpp"

#import <QuartzCore/CAMetalLayer.h>

class MetalLayerHelper
{
public:
    void Init(const char* windowName, unsigned int window_width, unsigned int window_height);
    void Terminate();

    MTL::Device* GetDevice() {return m_Device;}
    GLFWwindow* GetGLFWWindow() {return m_Window;}
    CA::MetalDrawable* GetDrawble();

private:
    void InitWindow(const char* windowName, unsigned int window_width, unsigned int window_height);

private:
    MTL::Device* m_Device;
    GLFWwindow* m_Window;
    NSWindow* m_MetalWindow;
    CAMetalLayer* m_MetalLayer;
};
