#include "Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

#include "system/random.h"
#include "system/microui.h"
#include "system/color_ramp.h"
#include "system/sokol_time.h"
#include <string.h>

static inline draw_color from_mu_color(mu_Color color) {return draw_color(color.r, color.b, color.g, color.a);}
static inline mu_Color to_mu_color(uint32_t packed_color) 
{
    mu_Color result; 
    uint32_t* packed = (uint32_t*)&result.r;
    *packed = packed_color;
    return result;
}

#define CANVAS_WIDTH (16.f)
#define CANVAS_HEIGHT (9.f)

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

    stm_setup();
    m_LastTime = m_StartTime = stm_now();
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

    hueshift_ramp_desc ramp = 
    {
        .hue = 220.0f,
        .hue_shift = 60.0f,
        .saturation_min = 0.1f,
        .saturation_max = 0.9f,
        .value_min = 0.2f,
        .value_max = 1.0f
    };

    for(uint32_t i=0; i<MU_COLOR_MAX; ++i)
        m_pGuiContext->style->colors[i] = to_mu_color(hueshift_ramp(&ramp, float(m_pGuiContext->style->colors[i].r) / 255.f, 0.9f));

    m_pGuiContext->style->colors[MU_COLOR_TEXT] = to_mu_color(0xffffffff);
    m_pGuiContext->style->colors[MU_COLOR_TITLETEXT] = to_mu_color(0xffffffff);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::DrawGui()
{
    m_Renderer.ResetCanvas();

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
    m_Time = (float)stm_sec(stm_since(m_StartTime));
    m_DeltaTime = (float) stm_sec(stm_laptime(&m_LastTime));

    mu_begin(m_pGuiContext);
    m_Renderer.BeginFrame();
    m_Renderer.SetCanvas(CANVAS_WIDTH, CANVAS_HEIGHT);

    m_Renderer.BeginCombination(0.1f);
    m_Renderer.DrawOrientedBox(3.f, 8.f, 12.f, 2.f, 1.f, 0.2f, draw_color(32, 224, 32, 192));
    m_Renderer.DrawCircleFilled(4.f + sinf(m_Time), 6.f, 1.f, draw_color(224, 32, 32, 255), op_union);
    m_Renderer.DrawCircleFilled(9.f + sinf(m_Time), 4.f + cosf(m_Time * 1.6666f), .5f, draw_color(0), op_subtraction);
    m_Renderer.DrawOrientedBox(4.35f, 7.4, 4.65f, 6.6f, 0.35f, 0.1f, draw_color(0), op_subtraction);
    
    m_Renderer.EndCombination();

    m_Renderer.DrawTriangleFilled((vec2){3.f + cosf(m_Time) * 3.f, 3.f + sinf(m_Time)},
                                  (vec2){6.f + cosf(m_Time*1.5f) * 3.f, 3.f},
                                  (vec2){3.f, 6.f+ sinf(m_Time*2.f) * 2.f},
                                  0.2f * fabsf(sinf(m_Time)), draw_color(128, 112, 224, 128));

    m_Renderer.DrawTriangle((vec2){8.f, 3.f},
                            (vec2){12.f, 5.f},
                            (vec2){9.f, 8.f}, 0.1f * (m_Time - floorf(m_Time)), draw_color(128, 112, 224, 128));
    

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
