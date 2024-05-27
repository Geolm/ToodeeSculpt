#include "MetalLayerHelper.h"

//----------------------------------------------------------------------------------------------------------------------------
void MetalLayerHelper::Init(const char* windowName, unsigned int window_width, unsigned int window_height)
{
    m_Device = MTL::CreateSystemDefaultDevice();
    InitWindow(windowName, window_width, window_height);
}

//----------------------------------------------------------------------------------------------------------------------------
void MetalLayerHelper::Terminate() 
{
    glfwTerminate();
    m_Device->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void MetalLayerHelper::InitWindow(const char* windowName, unsigned int window_width, unsigned int window_height)
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
    m_MetalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    m_MetalLayer.drawableSize = CGSizeMake(width, height);
    m_MetalWindow.contentView.layer = m_MetalLayer;
    m_MetalWindow.contentView.wantsLayer = YES;
}

//----------------------------------------------------------------------------------------------------------------------------
CA::MetalDrawable* MetalLayerHelper::GetDrawble()
{
    return (__bridge CA::MetalDrawable*)[m_MetalLayer nextDrawable];
}
