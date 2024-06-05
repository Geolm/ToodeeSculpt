#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

#include "system/random.h"
#include "system/microui.h"
#include <string.h>

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
void App::InitGui()
{
    m_pGuiContext = (mu_Context*) malloc(sizeof(mu_Context));
    mu_init(m_pGuiContext);

    m_pGuiContext->text_width = [](mu_Font font, const char *text, int len)  -> int
    {
        if (len == -1)
            len = (int)strlen(text);

        return len * FONT_SPACING;
    };

    m_pGuiContext->text_height = [] (mu_Font font) -> int { return FONT_HEIGHT; };
}

//----------------------------------------------------------------------------------------------------------------------------
void App::DrawGui()
{
    mu_Command *cmd = NULL;
    while (mu_next_command(m_pGuiContext, &cmd))
    {
    case MU_COMMAND_TEXT:
        {
            //m_Renderer.DrawText((float)cmd->text.pos.x, (float)cmd->text.pos.y, cmd->text.str, )
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Update(CA::MetalDrawable* drawable)
{
    m_Renderer.BeginFrame();

    int seed = 0x12345678;
    //for(uint32_t i=0; i<1000; i++)
    if (0)
    {
        float2 p0 = {.x = (float)iq_random_clamped(&seed, 0, m_ViewportWidth), .y = (float)iq_random_clamped(&seed, 0, m_ViewportHeight)};
        float2 p1 = {.x = (float)iq_random_clamped(&seed, 0, m_ViewportWidth), .y = (float)iq_random_clamped(&seed, 0, m_ViewportHeight)};
        m_Renderer.DrawCircleFilled(p0.x, p0.y, 25.f, 0x7f7ec4c1);
        m_Renderer.DrawCircleFilled(p1.x, p1.y, 25.f, 0x7f7ec4c1);
        //m_Renderer.DrawBox(p0.x, p0.y, p1.x, p1.y, 0x4fd26471);
        m_Renderer.DrawLine(p0.x, p0.y, p1.x, p1.y, 5.f, 0x7f34859d);
    }
    
    m_Renderer.DrawText(50.f, 20.f, "Ceci est un test!", 0x7fffffff);
    
    DrawGui();
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
    free(m_pGuiContext);
}
