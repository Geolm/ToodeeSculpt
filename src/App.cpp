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
#include "system/palettes.h"
#include "system/format.h"
#include <string.h>
#include "Editor.h"
#include "MouseCursors.h"
#include "Icons.h"

static inline draw_color from_mu_color(mu_Color color) {return draw_color(color.r, color.g, color.b, color.a);}
static inline mu_Color to_mu_color(uint32_t packed_color) 
{
    mu_Color result;
    result.a = (packed_color>>24)&0xff;
    result.b = (packed_color>>16)&0xff;
    result.g = (packed_color>>8)&0xff;
    result.r = packed_color&0xff;
    return result;
}

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

    glfwSetScrollCallback(window, [] (GLFWwindow* window, double xoffset, double yoffset)
    {
        App* user_ptr = (App*) glfwGetWindowUserPointer(window);
        mu_input_scroll(user_ptr->m_pGuiContext, (int)xoffset, (int)yoffset);
    });

    log_add_callback([] (log_Event *ev)
    {
        App* user_ptr = (App*) ev->udata;

        snprintf(user_ptr->m_pLogBuffer, user_ptr->m_LogSize, ev->fmt, ev->ap);
    }, this, 0);

    MouseCursors::CreateInstance();
    MouseCursors::GetInstance().Init(window);

    stm_setup();
    m_LastTime = m_StartTime = stm_now();

    m_pEditor = new Editor;
    m_pEditor->Init((aabb) {.min = (vec2) {510.f, 100.f}, .max = (vec2) {1410.f, 900.f}});

    glfwGetWindowContentScale(window, &m_ScaleX, &m_ScaleY);
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
        return len * (FONT_WIDTH + FONT_SPACING);
    };

    m_pGuiContext->text_height = [] (mu_Font font) -> int { return FONT_HEIGHT; };

    hueshift_ramp_desc ramp = 
    {
        .hue = 216.0f,
        .saturation = 0.65f,
        .value = 0.15f,
        .hue_shift = -150.f,
        .saturation_shift = -1.f,
        .value_shift = 1.0f
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
    m_Renderer.SetClipRect(0, 0, UINT16_MAX, UINT16_MAX);

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
        case MU_COMMAND_ICON :
        {
            aabb box = (aabb){.min = (vec2) {(float)cmd->icon.rect.x, (float)cmd->icon.rect.y},
                              .max = (vec2) {(float)(cmd->icon.rect.x + cmd->icon.rect.w), (float)(cmd->icon.rect.y + cmd->icon.rect.h)}};
            switch(cmd->icon.id)
            {
                case MU_ICON_CLOSE : DrawIcon(m_Renderer, box, ICON_CLOSE, draw_color(na16_red), draw_color(na16_dark_brown), 0.f);break;
                case MU_ICON_COLLAPSED : DrawIcon(m_Renderer, box, ICON_COLLAPSED, from_mu_color(cmd->icon.color), draw_color(0), 0.f);break;
                case MU_ICON_EXPANDED : DrawIcon(m_Renderer, box, ICON_EXPANDED, from_mu_color(cmd->icon.color), draw_color(0), 0.f);break;
                case MU_ICON_CHECK : DrawIcon(m_Renderer, box, ICON_CHECK, from_mu_color(cmd->icon.color), draw_color(0), 0.f);break;
            }

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
    m_pEditor->Draw(m_Renderer);
    m_pEditor->UserInterface(m_pGuiContext);

    // debug interface
    if (mu_begin_window_ex(m_pGuiContext, "Debug", mu_rect(1550, 0, 300, 600), MU_OPT_NOCLOSE))
    {
        if (mu_header(m_pGuiContext, "FPS"))
        {
            mu_layout_row(m_pGuiContext, 2, (int[]) { 150, -1 }, 0);
            mu_text(m_pGuiContext, "deltatime");
            mu_text(m_pGuiContext, format("%3.2f ms", m_DeltaTime*1000.f));
        }

        m_Renderer.DebugInterface(m_pGuiContext);
        m_pEditor->DebugInterface(m_pGuiContext);
        mu_end_window(m_pGuiContext);
    }
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
    if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_Z && action == GLFW_PRESS)
    {
        char text[2] = {(char)key, 0};
        
        if (!(mods&GLFW_MOD_SHIFT))
            text[0] = tolower(key);

        mu_input_text(m_pGuiContext, text);
    }

    int ui_key = 0;
    if (key == GLFW_KEY_BACKSPACE)
        ui_key |= MU_KEY_BACKSPACE;

    if (key == GLFW_KEY_ENTER)
        ui_key |= MU_KEY_RETURN;

    if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT)
        ui_key |= MU_KEY_SHIFT;

    if (key == GLFW_KEY_RIGHT_CONTROL || key == GLFW_KEY_LEFT_CONTROL)
        ui_key |= MU_KEY_CTRL;

    if (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT)
        ui_key |= MU_KEY_ALT;

    if (action == GLFW_PRESS)
        mu_input_keydown(m_pGuiContext, ui_key);

    if (action == GLFW_RELEASE)
        mu_input_keyup(m_pGuiContext, ui_key);

    if (key == GLFW_KEY_R && action == GLFW_PRESS && mods&GLFW_MOD_CONTROL)
        m_Renderer.ReloadShaders();
        
    if (key == GLFW_KEY_Z && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        m_pEditor->Undo();
    
    if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        m_pEditor->Delete();
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
    x *= m_ScaleX; y *= m_ScaleY;
    mu_input_mousemove(m_pGuiContext, (int)x, (int)y);
    m_MousePos = (vec2) {x, y};
    m_pEditor->OnMouseMove(m_MousePos);
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
        mu_input_mousedown(m_pGuiContext, (int)m_MousePos.x, (int)m_MousePos.y, mu_button);
    else if (action == GLFW_RELEASE)
        mu_input_mouseup(m_pGuiContext, (int)m_MousePos.x, (int)m_MousePos.y, mu_button);

    m_pEditor->OnMouseButton(m_MousePos, button, action);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    MouseCursors::GetInstance().Terminate();
    m_pEditor->Terminate();
    delete m_pEditor;
    m_Renderer.Terminate();
    free(m_pGuiContext);
    free(m_pLogBuffer);
}
