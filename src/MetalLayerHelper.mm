#include "MetalLayerHelper.h"

//----------------------------------------------------------------------------------------------------------------------------
void MetalLayerHelper::Init()
{
    m_Device = MTL::CreateSystemDefaultDevice();
    glfwInit();
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
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, 0);    // we do the anti-aliasing in the shader

    m_Window = glfwCreateWindow(window_width, window_height, windowName, NULL, NULL);
    if (!m_Window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    float x_scale, y_scale;
    glfwGetWindowContentScale(m_Window, &x_scale, &y_scale);
    
    // recreate the window if some scale is happening
    if (x_scale > 1.f || y_scale > 1.f)
    {
        glfwDestroyWindow(m_Window);
        m_Window = glfwCreateWindow(window_width + window_width/2, window_height + window_height/2, windowName, NULL, NULL);
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
