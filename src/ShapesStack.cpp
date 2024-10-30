#include "renderer/Renderer.h"
#include "ShapesStack.h"
#include "system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "MouseCursors.h"

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Init(aabb zone)
{
    cc_init(&m_Shapes);
    m_ContextualMenuOpen = false;
    m_EditionZone = zone;
    m_CurrentState = state::IDLE;
    m_ShapeNumPoints = 0;
    m_CurrentPoint = 0;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::OnMouseButton(float x, float y, int button, int action)
{
    vec2 mouse_pos = (vec2) {x, y};
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && m_CurrentState == state::IDLE)
    {
        if (aabb_test_point(m_EditionZone, mouse_pos))
        {
            m_ContextualMenuOpen = !m_ContextualMenuOpen;
            m_ContextualMenuPosition = mouse_pos;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Draw(Renderer& renderer)
{

}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::UserInterface(struct mu_Context* gui_context)
{
    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_ContextualMenuOpen)
    {
        if (mu_begin_window_ex(gui_context, "new shape", mu_rect((int)m_ContextualMenuPosition.x,
           (int)m_ContextualMenuPosition.y, 100, 140), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT))
        {
            mu_layout_row(gui_context, 1, (int[]) { 90}, 0);
            if (mu_button_ex(gui_context, "disc", 0, 0))
            {
                m_ContextualMenuOpen = false;
            }

            if (mu_button_ex(gui_context, "circle", 0, 0))
            {
                m_ContextualMenuOpen = false;
            }
            
            if (mu_button_ex(gui_context, "box", 0, 0))
            {
                m_ContextualMenuOpen = false;
            }

            if (mu_button_ex(gui_context, "triangle", 0, 0))
            {
                m_ShapeNumPoints = 3;
                SetState(state::ADDING_POINTS);
            }
            
            mu_end_window(gui_context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::SetState(enum state new_state)
{
    if (m_CurrentState == state::IDLE && new_state == state::ADDING_POINTS)
    {
        m_CurrentPoint = 0;
        m_ContextualMenuOpen = false;
        MouseCursors::GetInstance().Set(MouseCursors::CrossHair);
    }

    m_CurrentState = new_state;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Terminate()
{
    cc_cleanup(&m_Shapes);
}