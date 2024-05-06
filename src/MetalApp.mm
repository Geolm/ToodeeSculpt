#include "MetalApp.h"

void MetalApp::Init(const char* windowName, unsigned int window_width, unsigned int window_height)
{
    InitDevice();
    InitWindow(windowName, window_width, window_height);
}

void MetalApp::Run() 
{
    while (!glfwWindowShouldClose(m_Window)) 
    {
        glfwPollEvents();
    }
}

void MetalApp::Cleanup() 
{
    glfwTerminate();
    m_Device->release();
}

void MetalApp::InitDevice() 
{
    m_Device = MTL::CreateSystemDefaultDevice();
}

void MetalApp::InitWindow(const char* windowName, unsigned int window_width, unsigned int window_height)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(window_width, window_height, windowName, NULL, NULL);
    if (!m_Window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    m_MetalWindow = glfwGetCocoaWindow(m_Window);
    m_MetalLayer = [CAMetalLayer layer];
    m_MetalLayer.device = (__bridge id<MTLDevice>)m_Device;
    m_MetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_MetalLayer.drawableSize = CGSizeMake(width, height);
    m_MetalWindow.contentView.layer = m_MetalLayer;
    m_MetalWindow.contentView.wantsLayer = YES;
}
