#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

#include "system/random.h"
#include "system/microui.h"
#include <string.h>

static inline draw_color from_mu_color(mu_Color color) {return draw_color(color.r, color.b, color.g, color.a);}

//----------------------------------------------------------------------------------------------------------------------------
void App::Init(MTL::Device* device, GLFWwindow* window)
{
    m_Device = device;
    m_Window = window;
    
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    m_ViewportHeight = (uint32_t) height;
    m_ViewportWidth = (uint32_t) width;
    m_LogSize = 1024;
    m_pLogBuffer = (char*) malloc(m_LogSize);

    m_Renderer.Init(m_Device, m_ViewportWidth, m_ViewportHeight);
    InitGui();

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

    glfwSetCursorPosCallback(window, [] (GLFWwindow* window, double xpos, double ypos)
    {
        App* user_ptr = (App*) glfwGetWindowUserPointer(window);
        user_ptr->OnMouseMove((float)xpos, (float)ypos);
    });

    glfwSetMouseButtonCallback(window, [] (GLFWwindow* window, int button, int action, int mods)
    {
        App* user_ptr = (App*) glfwGetWindowUserPointer(window);
        user_ptr->OnMouseButton(button, action, mods);
    });

    log_add_callback([] (log_Event *ev)
    {
        App* user_ptr = (App*) ev->udata;

        snprintf(user_ptr->m_pLogBuffer, user_ptr->m_LogSize, ev->fmt, ev->ap);
    }, this, 0);
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
        float x0 = (float)cmd->rect.rect.x;
        float y0 = (float)cmd->rect.rect.y;
        float x1 = x0 + (float)cmd->rect.rect.w;
        float y1 = y0 + (float)cmd->rect.rect.h;

        switch (cmd->type) 
        {
        case MU_COMMAND_TEXT:
            {
                m_Renderer.DrawText((float)cmd->text.pos.x, (float)cmd->text.pos.y, cmd->text.str, from_mu_color(cmd->text.color));
                break;
            }
        case MU_COMMAND_RECT:
            {
                m_Renderer.DrawBox(x0, y0, x1, y1, from_mu_color(cmd->rect.color));
                break;
            }
        case MU_COMMAND_CLIP : 
            {
                m_Renderer.SetClipRect((uint16_t)cmd->rect.rect.x, (uint16_t)cmd->rect.rect.y,
                                       (uint16_t)(cmd->rect.rect.x + cmd->rect.rect.w), (uint16_t)(cmd->rect.rect.y + cmd->rect.rect.h));
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Update(CA::MetalDrawable* drawable)
{
    mu_begin(m_pGuiContext);
    m_Renderer.BeginFrame();


    int seed = 0x12345678;
    for(uint i=0; i<10000; ++i)
    {
        m_Renderer.DrawCircleFilled(iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight),
                                    iq_random_clamped(&seed, 0, 20), draw_color(220, 110, 0, 64));

        m_Renderer.DrawLine(iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight),
                            iq_random_clamped(&seed, 0, m_ViewportWidth), iq_random_clamped(&seed, 0, m_ViewportHeight),
                            iq_random_float(&seed) * 4.f, draw_color(24, 24, 224, 64));
    }


    m_Renderer.UserInterface(m_pGuiContext);
    LogUserInterface();
    mu_end(m_pGuiContext);
    DrawGui();
    m_Renderer.EndFrame();
    m_Renderer.Flush(drawable);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::LogUserInterface()
{
    if (mu_begin_window_ex(m_pGuiContext, "Log", mu_rect(1680, 700, 300, 300), 0))
    {
        mu_text(m_pGuiContext, m_pLogBuffer);
        mu_end_window(m_pGuiContext);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnKeyEvent(int key, int scancode, int action, int mods)
{
    // TODO : add keycode conversion
    if (action == GLFW_PRESS)
        mu_input_keydown(m_pGuiContext, key);

    if (action == GLFW_RELEASE)
        mu_input_keyup(m_pGuiContext, key);

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
void App::OnMouseMove(float x, float y)
{
    mu_input_mousemove(m_pGuiContext, (int)x, (int)y);
    m_MouseX = x; m_MouseY = y;
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnMouseButton(int button, int action, int mods)
{
    int mu_button = 0;
    switch(button)
    {
    case GLFW_MOUSE_BUTTON_LEFT : mu_button = MU_MOUSE_LEFT; break;
    case GLFW_MOUSE_BUTTON_MIDDLE : mu_button = MU_MOUSE_MIDDLE; break;
    case GLFW_MOUSE_BUTTON_RIGHT : mu_button = MU_MOUSE_RIGHT; break;
    }

    if (action == GLFW_PRESS)
        mu_input_mousedown(m_pGuiContext, (int)m_MouseX, (int)m_MouseY, mu_button);
    else if (action == GLFW_RELEASE)
        mu_input_mouseup(m_pGuiContext, (int)m_MouseX, (int)m_MouseY, mu_button);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    m_Renderer.Terminate();
    free(m_pGuiContext);
    free(m_pLogBuffer);
}
