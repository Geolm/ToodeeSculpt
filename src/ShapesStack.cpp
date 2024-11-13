#include "renderer/Renderer.h"
#include "ShapesStack.h"
#include "system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "MouseCursors.h"
#include "system/palettes.h"
#include "system/format.h"
#include "system/inside.h"
#include "system/undo.h"
#include "system/serializer.h"

const float shape_point_radius = 6.f;

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Init(aabb zone, struct undo_context* undo)
{
    cc_init(&m_Shapes);
    cc_reserve(&m_Shapes, SHAPES_STACK_RESERVATION);
    m_ContextualMenuOpen = false;
    m_EditionZone = zone;
    m_CurrentState = state::IDLE;
    m_ShapeNumPoints = 0;
    m_CurrentPoint = 0;
    m_SDFOperationComboBox = 0;
    m_SmoothBlend = 1.f;
    m_AlphaValue = 1.f;
    m_SelectedShapeIndex = INVALID_INDEX;
    m_pUndoContext = undo;
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
            m_SelectedShapeIndex = INVALID_INDEX;
        }
    }
    // selecting shape
    else if (m_CurrentState == state::IDLE && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        if (aabb_test_point(m_EditionZone, mouse_pos))
        {
            uint32_t selection = INVALID_INDEX;
            for(uint32_t i=0; i<cc_size(&m_Shapes) && selection == INVALID_INDEX; ++i)
            {
                shape *s = cc_get(&m_Shapes, i);
                if (MouseCursorInShape(s))
                    selection = i;
            }
            m_SelectedShapeIndex = selection;
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
        new_shape.color = (shape_color) {.red = 0.8f, .green = 0.2f, .blue = 0.4f, .alpha = 1.f};
        cc_push(&m_Shapes, new_shape);
        UndoSnapshot();

        SetState(state::IDLE);
        m_SelectedShapeIndex = uint32_t(cc_size(&m_Shapes))-1;
        
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Draw(Renderer& renderer)
{
    // drawing *the* shapes
    renderer.BeginCombination(m_SmoothBlend);
    for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
    {
        shape *s = cc_get(&m_Shapes, i);
        draw_color color;
        color.from_float(s->color.red, s->color.green, s->color.blue, m_AlphaValue);

        switch(s->shape_type)
        {
        case command_type::shape_triangle_filled: 
            {
                renderer.DrawTriangleFilled(s->shape_desc.triangle.p0, s->shape_desc.triangle.p1, s->shape_desc.triangle.p2, 
                                            s->roundness, color, s->op);
                break;
            }
        default: break;
        }
    }
    renderer.EndCombination();

    if (m_CurrentState == state::ADDING_POINTS)
    {
        for(uint32_t i=0; i<m_CurrentPoint; ++i)
            renderer.DrawCircleFilled(m_ShapePoints[i], shape_point_radius, draw_color(na16_red, 128));

        // preview shape
        if (m_ShapeType == command_type::shape_triangle_filled) 
        {
            if (m_CurrentPoint == 1)
                renderer.DrawOrientedBox(m_ShapePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
            else if (m_CurrentPoint == 2 || (m_CurrentPoint == 3 && vec2_similar(m_ShapePoints[1], m_ShapePoints[2], 0.001f)))
                renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_MousePosition, 0.f, draw_color(na16_light_blue, 128));

            renderer.DrawCircleFilled(m_MousePosition, shape_point_radius, draw_color(na16_red, 128));
        }
    }
    else if (m_CurrentState == state::SET_ROUNDNESS)
    {
        renderer.DrawCircleFilled(m_RoundnessReference, shape_point_radius, draw_color(na16_red, 128));

        // preview shape
        if (m_ShapeType == command_type::shape_triangle_filled) 
            renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_ShapePoints[2], m_Roundness, draw_color(na16_light_blue, 128));
    }
    else if (m_CurrentState == state::IDLE)
    {
        for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
        {
            shape *s = cc_get(&m_Shapes, i);
            if (MouseCursorInShape(s))
                DrawShapeGizmo(renderer, s);
        }

        if (m_SelectedShapeIndex != INVALID_INDEX && m_SelectedShapeIndex < cc_size(&m_Shapes))
            DrawShapeGizmo(renderer, cc_get(&m_Shapes, m_SelectedShapeIndex));
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::UserInterface(struct mu_Context* gui_context)
{
    if (mu_begin_window_ex(gui_context, "shape inspector", mu_rect(50, 50, 400, 600), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
    {
        int res = 0;
        if (mu_header_ex(gui_context, "global control", MU_OPT_EXPANDED))
        {
            mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
            mu_label(gui_context,"smooth blend");
            res |= mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);
            mu_label(gui_context, "alpha");
            res |= mu_slider_ex(gui_context, &m_AlphaValue, 0.f, 1.f, 0.01f, "%1.2f", 0);
        }

        if (m_SelectedShapeIndex != INVALID_INDEX && m_SelectedShapeIndex < cc_size(&m_Shapes)
            && mu_header_ex(gui_context, "selected shape", MU_OPT_EXPANDED))
        {
            shape *s = cc_get(&m_Shapes, m_SelectedShapeIndex);

            mu_layout_row(gui_context, 2, (int[]) { 100, -1 }, 0);
            mu_label(gui_context,"type");
            switch(s->shape_type)
            {
            case command_type::shape_circle_filled : 
            {
                mu_text(gui_context, "disc");
                mu_label(gui_context, "radius");
                res |= mu_slider_ex(gui_context, &s->shape_desc.disc.radius, 0.f, 100.f, 1.f, "%3.0f", 0);
                break;
            }
            case command_type::shape_triangle_filled : 
            {
                mu_text(gui_context, "triangle");
                mu_label(gui_context, "roundness");
                res |= mu_slider_ex(gui_context, &s->roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
                break;
            }
            case command_type::shape_oriented_box : mu_text(gui_context, "box");break;
            default : break;
            }

            _Static_assert(sizeof(s->op) == sizeof(int));
            mu_label(gui_context, "sdf op");
            const char* op_names[3] = {"union", "substraction", "intersection"};
            res |= mu_combo_box(gui_context, &m_SDFOperationComboBox, (int*)&s->op, 3, op_names);
            res |= mu_rgb_color(gui_context, &s->color.red, &s->color.green, &s->color.blue);
        }

        mu_end_window(gui_context);

        // if something has changed, handle undo
        if (res & MU_RES_CHANGE)
            UndoSnapshot();
    }
    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_ContextualMenuOpen)
    {
        if (mu_begin_window_ex(gui_context, "new shape", mu_rect((int)m_ContextualMenuPosition.x,
           (int)m_ContextualMenuPosition.y, 100, 140), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
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
void ShapesStack::UndoSnapshot()
{
    serializer_context serializer;
    size_t max_size;

    void* buffer = undo_begin_snapshot(m_pUndoContext, &max_size);
    serializer_init(&serializer, buffer, max_size);
    serializer_write_float(&serializer, m_AlphaValue);
    serializer_write_float(&serializer, m_Roundness);
    serializer_write_size_t(&serializer, cc_size(&m_Shapes));
    serializer_write_blob(&serializer, cc_get(&m_Shapes, 0), cc_size(&m_Shapes) * sizeof(shape));
    undo_end_snapshot(m_pUndoContext, buffer, serializer_get_position(&serializer));

    if (serializer_get_status(&serializer) == serializer_write_error)
        log_error("undo buffer is full");
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Undo()
{
    // cancel shape creation
    if (m_CurrentState == state::ADDING_POINTS || m_CurrentState == state::SET_ROUNDNESS)
    {
        SetState(state::IDLE);
    }
    // if idle, call undo manager
    else if (m_CurrentState == state::IDLE || m_CurrentState == state::SHAPE_SELECTED)
    {
        serializer_context serializer;
        size_t max_size;

        serializer_init(&serializer, undo_undo(m_pUndoContext, &max_size), max_size);
        m_AlphaValue = serializer_read_float(&serializer);
        m_Roundness = serializer_read_float(&serializer);
        size_t array_size = serializer_read_size_t(&serializer);
        cc_resize(&m_Shapes, array_size);
        serializer_read_blob(&serializer, cc_get(&m_Shapes, 0), array_size * sizeof(shape));

        if (serializer_get_status(&serializer) == serializer_read_error)
            log_fatal("corrupt undo buffer");
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
bool ShapesStack::MouseCursorInShape(const shape* s)
{
    switch(s->shape_type)
    {
    case command_type::shape_triangle_filled: 
    {
        const triangle_data& tri = s->shape_desc.triangle;
        bool result = point_in_triangle(tri.p0, tri.p1, tri.p2, m_MousePosition);
        result |= point_in_disc(tri.p0, shape_point_radius, m_MousePosition);
        result |= point_in_disc(tri.p1, shape_point_radius, m_MousePosition);
        result |= point_in_disc(tri.p2, shape_point_radius, m_MousePosition);
        return result;
    }

    default: 
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::DrawShapeGizmo(Renderer& renderer, const shape* s)
{
    switch(s->shape_type)
    {
    case command_type::shape_triangle_filled: 
        {
            const triangle_data& tri = s->shape_desc.triangle;
            renderer.DrawTriangleFilled(tri.p0, tri.p1, tri.p2, 0.f, draw_color(na16_orange, 128));
            renderer.DrawCircleFilled(tri.p0, shape_point_radius, draw_color(na16_black, 128));
            renderer.DrawCircleFilled(tri.p1, shape_point_radius, draw_color(na16_black, 128));
            renderer.DrawCircleFilled(tri.p2, shape_point_radius, draw_color(na16_black, 128));
            break;
        }

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Terminate()
{
    cc_cleanup(&m_Shapes);
}
