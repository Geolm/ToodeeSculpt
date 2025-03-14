#include "renderer/Metal.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#include "App.h"

#include "system/random.h"
#include "system/microui.h"
#include "system/color.h"
#include "system/sokol_time.h"
#include "system/palettes.h"
#include "system/format.h"
#include "system/whereami.h"
#include <string.h>
#include "editor/Editor.h"
#include "MouseCursors.h"
#include "icons.h"
#include "renderer/renderer.h"

#define UNUSED_VARIABLE(a) (void)(a)

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
void App::Init(MTL::Device* device, GLFWwindow* window, uint16_t viewport_width, uint16_t viewport_height)
{
    m_Device = device;
    m_Window = window;
    
    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);

    m_WindowWidth = (uint16_t) width;
    m_WindowHeight = (uint16_t) height;
    m_ViewportWidth = viewport_width;
    m_ViewportHeight = viewport_height;

    m_ScaleX = (float) m_ViewportWidth / (float) m_WindowWidth;
    m_ScaleY = (float) m_ViewportHeight / (float) m_WindowHeight;

    glfwGetWindowContentScale(window, &m_DPIScaleX, &m_DPIScaleY);

    m_pRenderer = renderer_init(m_Device, m_WindowWidth, m_WindowHeight);
    renderer_set_viewport(m_pRenderer, (float)m_ViewportWidth, (float)m_ViewportHeight);
    renderer_set_camera(m_pRenderer, vec2_zero(), 1.f);

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

    MouseCursors::CreateInstance();
    MouseCursors::GetInstance().Init(window);

    stm_setup();
    m_LastTime = m_StartTime = stm_now();
    m_AnimationTime = 0.f;

    RetrieveFolderPath();

    m_pEditor = new Editor;
    m_pEditor->Init(m_Window, (aabb) {.min = (vec2) {510.f, 100.f}, .max = (vec2) {1410.f, 1000.f}}, m_pFolderPath);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::RetrieveFolderPath()
{
    size_t path_length = wai_getExecutablePath(NULL, 0, NULL);
    m_pFolderPath = (char*)malloc(path_length + 1);
    int dirname_length;
    wai_getExecutablePath(m_pFolderPath, (int)path_length, &dirname_length);
    m_pFolderPath[dirname_length] = '\0';
    log_info("folder path : %s", m_pFolderPath);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::InitGui()
{
    m_pGuiContext = (mu_Context*) malloc(sizeof(mu_Context));
    mu_init(m_pGuiContext);

    m_pGuiContext->text_width = [](mu_Font font, const char *text, int len)  -> int
    {
        UNUSED_VARIABLE(font);
        if (len == -1)
            len = (int)strlen(text);
        return len * FONT_WIDTH;
    };

    m_pGuiContext->text_height = [] (mu_Font font) -> int { UNUSED_VARIABLE(font); return FONT_HEIGHT; };

    hueshift_ramp_desc ramp = 
    {
        .hue = 216.0f,
        .saturation = 0.65f,
        .value = 0.25f,
        .hue_shift = -200.f,
        .saturation_shift = -0.65f,
        .value_shift = .75f
    };

    for(uint32_t i=0; i<MU_COLOR_MAX; ++i)
        m_pGuiContext->style->colors[i] = to_mu_color(hueshift_ramp(&ramp, float(m_pGuiContext->style->colors[i].r) / 255.f, 0.9f));

    m_pGuiContext->style->colors[MU_COLOR_TEXT] = to_mu_color(0xffffffff);
    m_pGuiContext->style->colors[MU_COLOR_TITLETEXT] = to_mu_color(0xffffffff);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::DrawGui()
{
    renderer_set_cliprect(m_pRenderer, 0, 0, UINT16_MAX, UINT16_MAX);

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
            renderer_draw_text(m_pRenderer, (float)cmd->text.pos.x, (float)cmd->text.pos.y, cmd->text.str, from_mu_color(cmd->text.color));
            break;
        }
        case MU_COMMAND_RECT:
        {
            renderer_draw_box(m_pRenderer, x0, y0, x1, y1, from_mu_color(cmd->rect.color));
            break;
        }
        case MU_COMMAND_CLIP : 
        {
            uint16_t min_x = uint16_t(float(cmd->rect.rect.x * m_ScaleX));
            uint16_t min_y = uint16_t(float(cmd->rect.rect.x * m_ScaleY));
            uint16_t max_x = uint16_t(float(cmd->rect.rect.x + cmd->rect.rect.w) * m_ScaleX);
            uint16_t max_y = uint16_t(float(cmd->rect.rect.y + cmd->rect.rect.h) * m_ScaleY);
            renderer_set_cliprect(m_pRenderer, min_x, min_y, max_x, max_y);
            break;
        }
        case MU_COMMAND_ICON :
        {
            draw_color primary_color = from_mu_color(cmd->icon.color);
            draw_color secondary_color = from_mu_color(m_pGuiContext->style->colors[MU_COLOR_BASE]);
            aabb box = (aabb){.min = (vec2) {(float)cmd->icon.rect.x, (float)cmd->icon.rect.y},
                              .max = (vec2) {(float)(cmd->icon.rect.x + cmd->icon.rect.w), (float)(cmd->icon.rect.y + cmd->icon.rect.h)}};

            bool mouse_over = rect_overlaps_vec2(cmd->icon.rect, m_pGuiContext->mouse_pos);
            switch(cmd->icon.id)
            {
                case MU_ICON_CLOSE : DrawIcon(m_pRenderer, box, ICON_CLOSE, draw_color(na16_red), draw_color(na16_dark_brown), 0.f);break;
                default: DrawIcon(m_pRenderer, box, (enum icon_type) cmd->icon.id, primary_color, secondary_color, mouse_over ? m_AnimationTime : 0.f);break;
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
    m_AnimationTime += m_DeltaTime;

    while (m_AnimationTime>60.f)    // no animation lasts more than 60 seconds, so reset so we can keep float precision
        m_AnimationTime -= 60.f;

    mu_begin(m_pGuiContext);
    renderer_begin_frame(m_pRenderer);
    m_pEditor->Draw(m_pRenderer);
    m_pEditor->UserInterface(m_pGuiContext);

    // debug interface
    if (m_pEditor->IsDebugWindowOpen() && mu_begin_window_ex(m_pGuiContext, "Debug", mu_rect(1550, 500, 300, 400), MU_OPT_NOCLOSE))
    {
        if (mu_header_ex(m_pGuiContext, "Time", MU_OPT_EXPANDED))
        {
            mu_layout_row(m_pGuiContext, 2, (int[]) { 150, -1 }, 0);
            mu_text(m_pGuiContext, "deltatime");
            mu_text(m_pGuiContext, format("%3.2f ms", m_DeltaTime*1000.f));
            mu_text(m_pGuiContext, "time");
            mu_text(m_pGuiContext, format("%3.2f s", m_Time));
        }

        renderer_debug_interface(m_pRenderer, m_pGuiContext);
        m_pEditor->DebugInterface(m_pGuiContext);
        mu_end_window(m_pGuiContext);
    }

    mu_end(m_pGuiContext);
    DrawGui();

    renderer_end_frame(m_pRenderer);
    renderer_flush(m_pRenderer, drawable);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnKeyEvent(int key, int scancode, int action, int mods)
{
    UNUSED_VARIABLE(scancode);
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

    if (key == GLFW_KEY_R && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        renderer_reload_shaders(m_pRenderer);

    m_pEditor->OnKeyEvent(key, scancode, action, mods);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnWindowResize(int width, int height)
{
    m_WindowWidth = (uint16_t) width;
    m_WindowHeight = (uint16_t) height;
    renderer_resize(m_pRenderer, m_WindowWidth, m_WindowHeight);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnMouseMove(float x, float y)
{
    x *= m_ScaleX * m_DPIScaleX;
    y *= m_ScaleY * m_DPIScaleY;

    mu_input_mousemove(m_pGuiContext, (int)x, (int)y);
    m_MousePos = (vec2) {x, y};
    m_pEditor->OnMouseMove(m_MousePos);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::OnMouseButton(int button, int action, int mods)
{
    UNUSED_VARIABLE(mods);
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

    m_pEditor->OnMouseButton(button, action, mods);
}

//----------------------------------------------------------------------------------------------------------------------------
void App::Terminate()
{
    MouseCursors::GetInstance().Terminate();
    m_pEditor->Terminate();
    delete m_pEditor;
    renderer_terminate(m_pRenderer);
    free(m_pGuiContext);
    free(m_pFolderPath);
}
