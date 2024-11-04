#include "renderer/Renderer.h"
#include "ShapesStack.h"
#include "system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "MouseCursors.h"
#include "system/palettes.h"
#include "system/format.h"

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Init(aabb zone)
{
    cc_init(&m_Shapes);
    cc_reserve(&m_Shapes, SHAPES_STACK_RESERVATION);
    m_ContextualMenuOpen = false;
    m_EditionZone = zone;
    m_CurrentState = state::IDLE;
    m_ShapeNumPoints = 0;
    m_CurrentPoint = 0;
    m_SmoothBlend = 1.f;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::OnMouseMove(vec2 pos) 
{
    m_MousePosition = pos;

    if (m_CurrentState == state::SET_ROUNDNESS)
    {
        m_Roundness = vec2_distance(pos, m_RoundnessReference);
    }
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
            SetState(state::SET_ROUNDNESS);
    }
    else if (m_CurrentState == state::SET_ROUNDNESS && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        shape new_shape;
        new_shape.shape_type = m_ShapeType;
        new_shape.op = op_union;
        new_shape.roundness = m_Roundness;
        new_shape.shape_desc.triangle.p0 = m_ShapePoints[0];
        new_shape.shape_desc.triangle.p1 = m_ShapePoints[1];
        new_shape.shape_desc.triangle.p2 = m_ShapePoints[2];
        new_shape.color = draw_color(na16_purple);
        cc_push(&m_Shapes, new_shape);

        SetState(state::IDLE);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Draw(Renderer& renderer)
{
    if (m_CurrentState == state::ADDING_POINTS)
    {
        for(uint32_t i=0; i<m_CurrentPoint; ++i)
            renderer.DrawCircleFilled(m_ShapePoints[i], 5.f, draw_color(na16_red, 128));

        // preview shape
        if (m_ShapeType == command_type::shape_triangle_filled) 
        {
            if (m_CurrentPoint == 1)
                renderer.DrawOrientedBox(m_ShapePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
            else if (m_CurrentPoint == 2)            
                renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_MousePosition, 0.f, draw_color(na16_light_blue, 128));

            renderer.DrawCircleFilled(m_MousePosition, 5.f, draw_color(na16_red, 128));
        }
    }
    else if (m_CurrentState == state::SET_ROUNDNESS)
    {
        renderer.DrawCircleFilled(m_RoundnessReference, 5.f, draw_color(na16_red, 128));

        // preview shape
        if (m_ShapeType == command_type::shape_triangle_filled) 
            renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_ShapePoints[2], m_Roundness, draw_color(na16_light_blue, 128));
    }

    // drawing *the* shapes
    renderer.BeginCombination(m_SmoothBlend);
    for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
    {
        shape *s = cc_get(&m_Shapes, i);

        switch(s->shape_type)
        {
        case command_type::shape_triangle_filled: 
            {
                renderer.DrawTriangleFilled(s->shape_desc.triangle.p0, s->shape_desc.triangle.p1, s->shape_desc.triangle.p2, 
                                            s->roundness, s->color, s->op);
                break;
            }
        default: break;
        }
    }
    renderer.EndCombination();
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::UserInterface(struct mu_Context* gui_context)
{
    if (mu_begin_window_ex(gui_context, "shapes stack", mu_rect(50, 400, 400, 600), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT))
    {
        int res = 0;
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_label(gui_context,"smooth blend :");
        res |= mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);

        for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
        {
            shape *s = cc_get(&m_Shapes, i);
            if (mu_header(gui_context, format("shape #%d", i)))
            {
                mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
                mu_label(gui_context,"type:");
                switch(s->shape_type)
                {
                case command_type::shape_circle_filled : 
                {
                    mu_text(gui_context, "disc");
                    mu_label(gui_context, "center");
                    mu_text(gui_context, format("x:%f, y:%f", s->shape_desc.disc.center.x, s->shape_desc.disc.center.y));
                    mu_label(gui_context, "radius");
                    res |= mu_slider_ex(gui_context, &s->shape_desc.disc.radius, 0.f, 100.f, 1.f, "%3.0f", 0);
                    break;
                }
                case command_type::shape_triangle_filled : 
                {
                    mu_text(gui_context, "triangle");
                    mu_label(gui_context, "point0");
                    mu_text(gui_context, format("x:%4.1f, y:%4.1f", s->shape_desc.triangle.p0.x, s->shape_desc.triangle.p0.y));
                    mu_label(gui_context, "point1");
                    mu_text(gui_context, format("x:%4.1f, y:%4.1f", s->shape_desc.triangle.p1.x, s->shape_desc.triangle.p1.y));
                    mu_label(gui_context, "point2");
                    mu_text(gui_context, format("x:%4.1f, y:%4.1f", s->shape_desc.triangle.p2.x, s->shape_desc.triangle.p2.y));
                    mu_label(gui_context, "roundness");
                    res |= mu_slider_ex(gui_context, &s->roundness, 0.f, 100.f, 0.1f, "%3f", 0);
                    break;
                }
                case command_type::shape_oriented_box : mu_text(gui_context, "box");break;
                default : break;
                }

                mu_label(gui_context, "color");
                mu_text(gui_context, format("%8x", s->color));
            }
        }

        mu_end_window(gui_context);

        // if something has changed, handle undo
        if (res & (MU_RES_CHANGE|MU_RES_SUBMIT))
        {
            // TODO : undo
        }
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
        m_Roundness = 0.f;
        MouseCursors::GetInstance().Set(MouseCursors::HResize);
    }

    if (new_state == state::IDLE)
        MouseCursors::GetInstance().Default();

    m_CurrentState = new_state;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Terminate()
{
    cc_cleanup(&m_Shapes);
}
