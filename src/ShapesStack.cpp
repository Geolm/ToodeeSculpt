#include "renderer/Renderer.h"
#include "ShapesStack.h"
#include "system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "MouseCursors.h"
#include "system/palettes.h"
#include "system/format.h"
#include "system/point_in.h"
#include "system/undo.h"
#include "system/serializer.h"

const float shape_point_radius = 6.f;
const vec2 contextual_menu_size = {100.f, 140.f};

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Init(aabb zone, struct undo_context* undo)
{
    cc_init(&m_Shapes);
    cc_reserve(&m_Shapes, SHAPES_STACK_RESERVATION);
    m_NewShapeContextualMenuOpen = false;
    m_EditionZone = zone;
    m_CurrentState = state::IDLE;
    m_CurrentPoint = 0;
    m_SDFOperationComboBox = 0;
    m_SmoothBlend = 1.f;
    m_AlphaValue = 1.f;
    m_SelectedShapeIndex = INVALID_INDEX;
    m_pUndoContext = undo;
    m_pGrabbedPoint = nullptr;
    m_SnapToGrid = false;
    m_GridSubdivision = 20.f;

    UndoSnapshot();
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::OnMouseMove(vec2 pos) 
{
    if ((GetState() == state::ADDING_POINTS || GetState() == state::MOVING_POINT) && m_SnapToGrid)
    {
        pos = vec2_div(pos - m_EditionZone.min, aabb_get_size(&m_EditionZone));
        pos = vec2_floor(vec2_add(vec2_scale(pos, m_GridSubdivision), vec2_splat(.5f)));
        pos = vec2_mul(vec2_scale(pos, 1.f / m_GridSubdivision), aabb_get_size(&m_EditionZone));
        pos += m_EditionZone.min;
    }

    m_MousePosition = pos;

    if (GetState() == state::SET_ROUNDNESS)
    {
        m_Roundness = vec2_distance(pos, m_Reference);
    }
    else if (GetState() == state::SET_WIDTH)
    {
        m_Width = vec2_distance(pos, m_Reference) * 2.f;
    }
    // moving point
    else if (GetState() == state::MOVING_POINT && m_pGrabbedPoint != nullptr)
    {
        *m_pGrabbedPoint = m_MousePosition;
    }
    else if (GetState() == state::MOVING_SHAPE)
    {
        shape *s = cc_get(&m_Shapes, m_SelectedShapeIndex);
        vec2 delta = m_MousePosition - m_Reference;
        vec2* points = s->shape_desc.points;
        for(uint32_t i=0; i<ShapeNumPoints(s->shape_type); ++i)
            points[i] += delta;

        m_Reference = m_MousePosition;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::OnMouseButton(int button, int action)
{
    bool left_button_pressed = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
    // contextual menu
    if (m_NewShapeContextualMenuOpen && left_button_pressed)
    {
        aabb contextual_bbox = (aabb) {.min = m_ContextualMenuPosition, .max = m_ContextualMenuPosition + contextual_menu_size};

        if (!aabb_test_point(&contextual_bbox, m_MousePosition))
            m_NewShapeContextualMenuOpen = false;
    }
    if (GetState() == state::IDLE && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (aabb_test_point(&m_EditionZone, m_MousePosition))
        {
            m_NewShapeContextualMenuOpen = !m_NewShapeContextualMenuOpen;
            m_ContextualMenuPosition = m_MousePosition;
            m_SelectedShapeIndex = INVALID_INDEX;
        }
    }
    // selecting shape
    else if (GetState() == state::IDLE && left_button_pressed)
    {
        if (SelectedShapeValid())
        {
            shape *s = cc_get(&m_Shapes, m_SelectedShapeIndex);
            vec2* points = s->shape_desc.points;
            for(uint32_t i=0; i<ShapeNumPoints(s->shape_type); ++i)
            {
                if (point_in_disc(points[i], shape_point_radius, m_MousePosition))
                {
                    m_pGrabbedPoint = &points[i];
                    *m_pGrabbedPoint = m_MousePosition;
                    SetState(state::MOVING_POINT);
                }
            }

            if (GetState() == state::IDLE && MouseCursorInShape(s, false))
            {
                m_Reference = m_MousePosition;
                SetState(state::MOVING_SHAPE);
            }
        }

        if (aabb_test_point(&m_EditionZone, m_MousePosition) && m_pGrabbedPoint == nullptr)
        {
            uint32_t selection = INVALID_INDEX;
            for(uint32_t i=0; i<cc_size(&m_Shapes) && selection == INVALID_INDEX; ++i)
            {
                shape *s = cc_get(&m_Shapes, i);
                if (MouseCursorInShape(s, true))
                    selection = i;
            }
            m_SelectedShapeIndex = selection;
        }
    }
    // moving point
    else if (GetState() == state::MOVING_POINT && m_pGrabbedPoint != nullptr)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            SetState(state::IDLE);
            UndoSnapshot();
        }
    }
    // moving shape
    else if (GetState() == state::MOVING_SHAPE && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        SetState(state::IDLE);
        UndoSnapshot();
    }
    // adding points
    else if (GetState() == state::ADDING_POINTS && left_button_pressed)
    {
        if (!aabb_test_point(&m_EditionZone, m_MousePosition))
            SetState(state::IDLE);

        assert(m_CurrentPoint < SHAPE_MAXPOINTS);
        m_ShapePoints[m_CurrentPoint++] = m_MousePosition;

        if (m_CurrentPoint == ShapeNumPoints(m_ShapeType))
        {
            if (m_ShapeType == command_type::shape_oriented_box || m_ShapeType == command_type::shape_ellipse)
                SetState(state::SET_WIDTH);
            else
                SetState(state::SET_ROUNDNESS);
        }
    }
    else if (GetState() == state::SET_WIDTH && left_button_pressed)
    {
        if (m_ShapeType == command_type::shape_oriented_box)
            SetState(state::SET_ROUNDNESS);
        else if (m_ShapeType == command_type::shape_ellipse)
            SetState(state::CREATE_SHAPE);
    }
    else if (GetState() == state::SET_ROUNDNESS && left_button_pressed)
    {
        SetState(state::CREATE_SHAPE);
    }

    // shape creation
    if (GetState() == state::CREATE_SHAPE)
    {
        shape new_shape;
        new_shape.shape_type = m_ShapeType;
        new_shape.op = op_union;
        new_shape.roundness = m_Roundness;
        new_shape.color = (shape_color) {.red = 0.8f, .green = 0.2f, .blue = 0.4f};
        new_shape.shape_desc.width = m_Width;

        for(uint32_t i=0; i<ShapeNumPoints(m_ShapeType); ++i)
            new_shape.shape_desc.points[i] = m_ShapePoints[i];

        cc_push(&m_Shapes, new_shape);

        SetState(state::IDLE);
        m_SelectedShapeIndex = uint32_t(cc_size(&m_Shapes))-1;

        UndoSnapshot();
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
        DrawShape(renderer, s, s->roundness, color, s->op);
    }
    renderer.EndCombination();

    if (GetState() == state::ADDING_POINTS)
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
        }
        else if ((m_ShapeType == command_type::shape_oriented_box || m_ShapeType == command_type::shape_ellipse)
                  && m_CurrentPoint == 1)
        {
            renderer.DrawOrientedBox(m_ShapePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
        }
        renderer.DrawCircleFilled(m_MousePosition, shape_point_radius, draw_color(na16_red, 128));
    }
    else if (GetState() == state::SET_WIDTH)
    {
        renderer.DrawCircleFilled(m_Reference, shape_point_radius, draw_color(na16_red, 128));

        if (m_ShapeType == command_type::shape_oriented_box)
            renderer.DrawOrientedBox(m_ShapePoints[0], m_ShapePoints[1], m_Width, 0.f, draw_color(na16_light_blue, 128));
        else if (m_ShapeType == command_type::shape_ellipse)
            renderer.DrawEllipse(m_ShapePoints[0], m_ShapePoints[1], m_Width, draw_color(na16_light_blue, 128));
    }
    else if (GetState() == state::SET_ROUNDNESS)
    {
        renderer.DrawCircleFilled(m_Reference, shape_point_radius, draw_color(na16_red, 128));

        // preview shape
        switch(m_ShapeType)
        {
        case command_type::shape_triangle_filled:
            renderer.DrawTriangleFilled(m_ShapePoints[0], m_ShapePoints[1], m_ShapePoints[2], 
                                        m_Roundness, draw_color(na16_light_blue, 128));break;

        case command_type::shape_circle_filled:
            renderer.DrawCircleFilled(m_ShapePoints[0], m_Roundness, draw_color(na16_light_blue, 128));break;

        case command_type::shape_oriented_box:
            renderer.DrawOrientedBox(m_ShapePoints[0], m_ShapePoints[1], m_Width, m_Roundness, draw_color(na16_light_blue, 128));
            break;

        default:break;
        }
    }
    else if (GetState() == state::IDLE)
    {
        MouseCursors::GetInstance().Default();
        for(uint32_t i=0; i<cc_size(&m_Shapes); ++i)
        {
            shape *s = cc_get(&m_Shapes, i);
            if (MouseCursorInShape(s, true))
            {
                DrawShapeGizmo(renderer, s);
                if (i == m_SelectedShapeIndex)
                    MouseCursors::GetInstance().Set(MouseCursors::Hand);
            }
        }

        if (SelectedShapeValid())
            DrawShapeGizmo(renderer, cc_get(&m_Shapes, m_SelectedShapeIndex));
    }
    else if (GetState() == state::MOVING_POINT || GetState() == state::MOVING_SHAPE)
    {
        if (SelectedShapeValid())
            DrawShapeGizmo(renderer, cc_get(&m_Shapes, m_SelectedShapeIndex));
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::UserInterface(struct mu_Context* gui_context)
{
    if (mu_begin_window_ex(gui_context, "shape inspector", mu_rect(50, 500, 400, 400), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
    {
        int res = 0;

        if (mu_header_ex(gui_context, "global control", MU_OPT_EXPANDED))
        {
            mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
            mu_label(gui_context,"smoothness");
            res |= mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);
            mu_label(gui_context, "alpha");
            res |= mu_slider_ex(gui_context, &m_AlphaValue, 0.f, 1.f, 0.01f, "%1.2f", 0);
        }

        if (SelectedShapeValid() && mu_header_ex(gui_context, "selected shape", MU_OPT_EXPANDED))
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
                    res |= mu_slider_ex(gui_context, &s->roundness, 0.f, 1000.f, 1.f, "%3.0f", 0);
                    break;
                }
            case command_type::shape_ellipse :
                {
                    mu_text(gui_context, "ellipse");
                    mu_label(gui_context, "width");
                    res |= mu_slider_ex(gui_context, &s->shape_desc.width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
                    break;
                }
            case command_type::shape_triangle_filled : 
                {
                    mu_text(gui_context, "triangle");
                    mu_label(gui_context, "roundness");
                    res |= mu_slider_ex(gui_context, &s->roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
                    break;
                }
            case command_type::shape_oriented_box : 
                {
                    mu_text(gui_context, "box");
                    mu_label(gui_context, "width");
                    res |= mu_slider_ex(gui_context, &s->shape_desc.width, 0.f, 1000.f, 0.1f, "%3.2f", 0);
                    mu_label(gui_context, "roundness");
                    res |= mu_slider_ex(gui_context, &s->roundness, 0.f, 100.f, 0.1f, "%3.2f", 0);
                    break;
                }

            default : break;
            }

            _Static_assert(sizeof(s->op) == sizeof(int));
            mu_label(gui_context, "operation");
            const char* op_names[op_last] = {"add", "blend", "sub", "overlap"};
            res |= mu_combo_box(gui_context, &m_SDFOperationComboBox, (int*)&s->op, op_last, op_names);
            res |= mu_rgb_color(gui_context, &s->color.red, &s->color.green, &s->color.blue);
        }

        mu_end_window(gui_context);

        // if something has changed, handle undo
        if (res & MU_RES_SUBMIT)
            UndoSnapshot();

    }
    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_NewShapeContextualMenuOpen)
    {
        if (mu_begin_window_ex(gui_context, "new shape", mu_rect((int)m_ContextualMenuPosition.x,
            (int)m_ContextualMenuPosition.y, (int)contextual_menu_size.x, (int)contextual_menu_size.y), 
            MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
        {
            mu_layout_row(gui_context, 1, (int[]) { 90}, 0);
            if (mu_button_ex(gui_context, "disc", 0, 0))
            {
                m_ShapeType = command_type::shape_circle_filled;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "ellipse", 0, 0))
            {
                m_ShapeType = command_type::shape_ellipse;
                SetState(state::ADDING_POINTS);
            }
            
            if (mu_button_ex(gui_context, "box", 0, 0))
            {
                m_ShapeType = command_type::shape_oriented_box;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "triangle", 0, 0))
            {
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
    size_t array_size = cc_size(&m_Shapes);

    void* buffer = undo_begin_snapshot(m_pUndoContext, &max_size);
    serializer_init(&serializer, buffer, max_size);
    serializer_write_float(&serializer, m_AlphaValue);
    serializer_write_float(&serializer, m_SmoothBlend);
    serializer_write_uint32_t(&serializer, m_SelectedShapeIndex);
    serializer_write_size_t(&serializer, cc_size(&m_Shapes));
    if (array_size != 0)
        serializer_write_blob(&serializer, cc_get(&m_Shapes, 0), cc_size(&m_Shapes) * sizeof(shape));
    undo_end_snapshot(m_pUndoContext, buffer, serializer_get_position(&serializer));

    if (serializer_get_status(&serializer) == serializer_write_error)
        log_error("undo buffer is full");
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Undo()
{
    // cancel shape creation
    if (GetState() == state::ADDING_POINTS || GetState() == state::SET_ROUNDNESS)
    {
        SetState(state::IDLE);
    }
    // if idle, call undo manager
    else if (GetState() == state::IDLE || GetState() == state::SHAPE_SELECTED)
    {
        serializer_context serializer;
        size_t max_size;
        void* pBuffer = undo_undo(m_pUndoContext, &max_size);

        if (pBuffer != nullptr)
        {
            serializer_init(&serializer, pBuffer, max_size);
            m_AlphaValue = serializer_read_float(&serializer);
            m_SmoothBlend = serializer_read_float(&serializer);
            m_SelectedShapeIndex = serializer_read_uint32_t(&serializer);
            size_t array_size = serializer_read_size_t(&serializer);
            cc_resize(&m_Shapes, array_size);
            if (array_size != 0)
                serializer_read_blob(&serializer, cc_get(&m_Shapes, 0), array_size * sizeof(shape));

            if (serializer_get_status(&serializer) == serializer_read_error)
                log_fatal("corrupt undo buffer");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::DeleteSelected()
{
    if (GetState() == state::IDLE && SelectedShapeValid())
    {
        cc_erase(&m_Shapes, m_SelectedShapeIndex);
        m_SelectedShapeIndex = INVALID_INDEX;
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::SetState(enum state new_state)
{
    if (GetState() == state::IDLE && new_state == state::ADDING_POINTS)
    {
        m_CurrentPoint = 0;
        m_NewShapeContextualMenuOpen = false;
        MouseCursors::GetInstance().Set(MouseCursors::CrossHair);
    }

    if (new_state == state::SET_ROUNDNESS)
    {
        m_Reference = m_MousePosition;
        m_Roundness = 0.f;
        MouseCursors::GetInstance().Set(MouseCursors::HResize);
    }

    if (new_state == state::SET_WIDTH)
    {
        m_Width = 0.f;
        m_Reference = m_MousePosition;
        MouseCursors::GetInstance().Set(MouseCursors::HResize);
    }

    if (GetState() == state::MOVING_POINT && new_state == state::IDLE)
        m_pGrabbedPoint = nullptr;

    if (new_state == state::MOVING_POINT)
        MouseCursors::GetInstance().Set(MouseCursors::Hand);

    if (new_state == state::IDLE)
        MouseCursors::GetInstance().Default();

    m_CurrentState = new_state;
}

//----------------------------------------------------------------------------------------------------------------------------
bool ShapesStack::MouseCursorInShape(const shape* s, bool test_vertices)
{
    const vec2* points = s->shape_desc.points;
    bool result = false;

    switch(s->shape_type)
    {
    case command_type::shape_triangle_filled: 
        {
            result = point_in_triangle(points[0], points[1], points[2], m_MousePosition);
            break;
        }
    case command_type::shape_circle_filled:
        {
            result = point_in_disc(points[0], s->roundness, m_MousePosition);
            break;
        }
    case command_type::shape_ellipse:
        {
            result = point_in_ellipse(points[0], points[1], s->shape_desc.width, m_MousePosition);
            break;
        }
    case command_type::shape_oriented_box:
        {
            result = point_in_oriented_box(points[0], points[1], s->shape_desc.width, m_MousePosition);
            break;
        }

    default: 
        return false;
    }

    if (test_vertices)
    {
        for(uint32_t i=0; i<ShapeNumPoints(s->shape_type); ++i)
            result |= point_in_disc(points[i], shape_point_radius, m_MousePosition);
    }

    return result;
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::DrawShapeGizmo(Renderer& renderer, const shape* s)
{
    DrawShape(renderer, s, 0.f, draw_color(na16_orange, 128), op_add);

    for(uint32_t i=0; i<ShapeNumPoints(s->shape_type); ++i)
        renderer.DrawCircleFilled(s->shape_desc.points[i], shape_point_radius, draw_color(na16_black, 128));
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::DrawShape(Renderer& renderer, const shape* s, float roundness, draw_color color, sdf_operator op)
{
    const vec2* points = s->shape_desc.points;
    switch(s->shape_type)
    {
    case command_type::shape_triangle_filled: 
        renderer.DrawTriangleFilled(points[0], points[1], points[2], roundness, color, op);
        break;

    case command_type::shape_circle_filled:
        renderer.DrawCircleFilled(points[0], roundness, color, op);
        break;

    case command_type::shape_ellipse:
        renderer.DrawEllipse(points[0], points[1], s->shape_desc.width, color, op);
        break;

    case command_type::shape_oriented_box:
        renderer.DrawOrientedBox(points[0], points[1], s->shape_desc.width, roundness, color, op);
        break;

    default: 
        break;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void ShapesStack::Terminate()
{
    cc_cleanup(&m_Shapes);
}
