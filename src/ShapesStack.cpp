#include "renderer/Renderer.h"
#include "ShapesStack.h"
#include "system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "MouseCursors.h"
#include "system/na16_palette.h"
#include "system/format.h"

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
void ShapesStack::OnMouseButton(vec2 mouse_pos, int button, int action)
{
    // contextual menu
    if (m_CurrentState == state::IDLE && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (aabb_test_point(m_EditionZone, mouse_pos))
        {
            m_ContextualMenuOpen = !m_ContextualMenuOpen;
            m_ContextualMenuPosition = mouse_pos;
        }
    }
    // adding points
    else if (m_CurrentState == state::ADDING_POINTS && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        assert(m_CurrentPoint < SHAPE_MAXPOINTS);
        m_ShapePoints[m_CurrentPoint++] = mouse_pos;

        if (m_CurrentPoint == m_ShapeNumPoints)
        {
            SetState(state::SET_ROUNDNESS);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Draw(Renderer& renderer)
{
    if (m_CurrentState == state::ADDING_POINTS)
    {
        for(uint32_t i=0; i<m_CurrentPoint; ++i)
            renderer.DrawCircleFilled(m_ShapePoints[i].x, m_ShapePoints[i].y, 5.f, draw_color(na16_red, 128));

        // preview shape
        if (m_ShapeType == command_type::shape_triangle_filled) 
        {
            if (m_CurrentPoint == 1)
                renderer.DrawOrientedBox(m_ShapePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
            else if (m_CurrentPoint == 2)            
                renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_MousePosition, 0.f, draw_color(na16_light_blue, 128));

            renderer.DrawCircleFilled(m_MousePosition.x, m_MousePosition.y, 5.f, draw_color(na16_red, 128));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::UserInterface(struct mu_Context* gui_context)
{
    if (mu_begin_window_ex(gui_context, "shapes stack", mu_rect(50, 400, 400, 600), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT))
    {
        for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
        {
            mu_header(gui_context, format("shape %d", i));
        }

        mu_end_window(gui_context);
    }
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
                m_ShapeType = command_type::shape_triangle_filled;
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

    if (m_CurrentState == state::ADDING_POINTS && new_state == state::SET_ROUNDNESS)
    {
        m_RoundnessReference = m_MousePosition;
        MouseCursors::GetInstance().Default();
    }

    m_CurrentState = new_state;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Terminate()
{
    cc_cleanup(&m_Shapes);
}
