#include "../renderer/renderer.h"
#include "../system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "../MouseCursors.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/undo.h"
#include "../system/format.h"
#include "PrimitivesStack.h"
#include "tds.h"
#include "export.h"

const size_t clipboard_buffer_size = (1<<18);
struct palette primitive_palette;

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Init(aabb zone, struct undo_context* undo)
{
    cc_init(&m_Primitives);
    cc_reserve(&m_Primitives, PRIMITIVES_STACK_RESERVATION);
    cc_init(&m_MultipleSelection);
    cc_reserve(&m_MultipleSelection, PRIMITIVES_STACK_RESERVATION);
    m_EditionZone = zone;
    m_pUndoContext = undo;
    m_SnapToGrid = false;
    m_GridSubdivision = 20.f;
    m_PrimitiveIdDebug = false;
    m_AABBDebug = false;
    m_PointColor = draw_color(0x10e010, 128);
    m_SelectedPrimitiveColor = draw_color(0x101020, 128);
    m_HoveredPrimitiveColor = draw_color(0x101020, 64);
    m_OutlineWidth = 1.f;
    m_GlobalOutline = 0;
    palette_default(&primitive_palette);
    New();
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::New()
{
    m_NewPrimitiveContextualMenuOpen = false;
    m_SelectedPrimitiveContextualMenuOpen = false;
    m_CurrentState = state::IDLE;
    m_CurrentPoint = 0;
    m_SDFOperationComboBox = 0;
    m_SmoothBlend = 1.f;
    m_AlphaValue = 1.f;
    SetSelectedPrimitive(INVALID_INDEX);
    primitive_set_invalid(&m_CopiedPrimitive);
    m_pGrabbedPoint = nullptr;
    cc_clear(&m_Primitives);
    cc_clear(&m_MultipleSelection);
    UndoSnapshot();
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::OnMouseMove(vec2 pos) 
{
    if ((GetState() == state::ADDING_POINTS || GetState() == state::MOVING_POINT) && m_SnapToGrid)
    {
        pos = vec2_div(pos - m_EditionZone.min, aabb_get_size(&m_EditionZone));
        pos = vec2_floor(vec2_add(vec2_scale(pos, m_GridSubdivision), vec2_splat(.5f)));
        pos = vec2_mul(vec2_scale(pos, 1.f / m_GridSubdivision), aabb_get_size(&m_EditionZone));
        pos += m_EditionZone.min;
    }

    m_MouseLastPosition = m_MousePosition;
    m_MousePosition = pos;

    if (GetState() == state::SET_ROUNDNESS)
    {
        m_Roundness = vec2_distance(pos, m_Reference);
    }
    else if (GetState() == state::SET_WIDTH)
    {
        m_Width = vec2_distance(pos, m_Reference) * 2.f;
    }
    else if (GetState() == state::SET_ANGLE)
    {
        vec2 to_mouse = m_MousePosition - m_PrimitivePoints[0];
        vec2_normalize(&to_mouse);
        m_Aperture = acosf(vec2_dot(m_Direction, to_mouse));
    }
    else if (GetState() == state::MOVING_POINT && m_pGrabbedPoint != nullptr)
    {
        *m_pGrabbedPoint = m_MousePosition;
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
    }
    else if (GetState() == state::MOVING_PRIMITIVE)
    {
        primitive_translate(cc_get(&m_Primitives, m_SelectedPrimitiveIndex), m_MousePosition - m_Reference);
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
        m_Reference = m_MousePosition;
    }
    else if (GetState() == state::ROTATING_PRIMITIVE)
    {
        primitive* selected = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
        *selected = m_CopiedPrimitive;
        primitive_rotate(selected, vec2_atan2(m_MousePosition - m_Reference));
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
    }
    else if (GetState() == state::SCALING_PRIMITIVE)
    {
        primitive* selected = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
        *selected = m_CopiedPrimitive;

        float scale = (m_MousePosition.x - m_Reference.x); ;
        if (scale>0.f)
            scale = (scale / (aabb_get_size(&m_EditionZone).x * 0.1f)) + 1.f;
        else
            scale = float_max(1.f + (scale / (aabb_get_size(&m_EditionZone).x * 0.1f)), 0.1f);

        primitive_scale(selected, scale);
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::OnMouseButton(int button, int action, int mods)
{
    bool left_button_pressed = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) && !m_NewPrimitiveContextualMenuOpen && !m_SelectedPrimitiveContextualMenuOpen;

    if (GetState() == state::IDLE && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (aabb_test_point(&m_EditionZone, m_MousePosition))
        {
            if (SelectedPrimitiveValid() && primitive_test_mouse_cursor(cc_get(&m_Primitives, m_SelectedPrimitiveIndex), m_MousePosition, false))
            {
                m_SelectedPrimitiveContextualMenuOpen = !m_SelectedPrimitiveContextualMenuOpen;
            }
            else
            {
                m_NewPrimitiveContextualMenuOpen = !m_NewPrimitiveContextualMenuOpen;
                SetSelectedPrimitive(INVALID_INDEX);
            }
            m_ContextualMenuPosition = m_MousePosition;
        }
    }
    // selecting primitive
    else if (GetState() == state::IDLE && left_button_pressed && aabb_test_point(&m_EditionZone, m_MousePosition))
    {
        if (SelectedPrimitiveValid())
        {
            primitive *primitive = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
            for(uint32_t i=0; i<primitive_get_num_points(primitive->m_Shape); ++i)
            {
                if (point_in_disc(primitive_get_points(primitive, i), primitive_point_radius, m_MousePosition))
                {
                    m_pGrabbedPoint = &primitive->m_Points[i];
                    *m_pGrabbedPoint = m_MousePosition;
                    SetState(state::MOVING_POINT);
                }
            }

            // clicking on already selected primitive to move/duplicate
            if (GetState() == state::IDLE && !SelectPrimitive() && SelectedPrimitiveValid())
            {
                // copy a primitive
                if (mods&GLFW_MOD_SUPER)
                    DuplicateSelected();
                
                m_Reference = m_MousePosition;
                m_StartingPoint = m_MousePosition;
                SetState(state::MOVING_PRIMITIVE);
            }
        }
        else
            SelectPrimitive();
    }
    // moving point
    else if (GetState() == state::MOVING_POINT && m_pGrabbedPoint != nullptr)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
            SetState(state::IDLE);
            UndoSnapshot();
        }
    }
    // moving primitive
    else if (GetState() == state::MOVING_PRIMITIVE && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
        SetState(state::IDLE);
        if (m_StartingPoint != m_MousePosition)
            UndoSnapshot();
    }
    // adding points
    else if (GetState() == state::ADDING_POINTS && left_button_pressed)
    {
        if (!aabb_test_point(&m_EditionZone, m_MousePosition))
            SetState(state::IDLE);

        assert(m_CurrentPoint < PRIMITIVE_MAXPOINTS);
        m_PrimitivePoints[m_CurrentPoint++] = m_MousePosition;

        if (m_CurrentPoint == primitive_get_num_points(m_PrimitiveShape))
        {
            if (m_PrimitiveShape == shape_oriented_box || m_PrimitiveShape == shape_oriented_ellipse)
                SetState(state::SET_WIDTH);
            else if (m_PrimitiveShape == shape_pie)
            {
                m_Direction = m_PrimitivePoints[1] - m_PrimitivePoints[0];
                vec2_normalize(&m_Direction);
                SetState(state::SET_ANGLE);
            }
            else
                SetState(state::SET_ROUNDNESS);
        }
    }
    else if (GetState() == state::SET_WIDTH && left_button_pressed)
    {
        if (m_PrimitiveShape == shape_oriented_box)
            SetState(state::SET_ROUNDNESS);
        else if (m_PrimitiveShape == shape_oriented_ellipse)
            SetState(state::CREATE_PRIMITIVE);
    }
    else if (GetState() == state::SET_ROUNDNESS && left_button_pressed)
    {
        SetState(state::CREATE_PRIMITIVE);
    }
    else if (GetState() == state::SET_ANGLE && left_button_pressed)
    {
        SetState(state::CREATE_PRIMITIVE);
    }
    else if ((GetState() == state::ROTATING_PRIMITIVE || GetState() == state::SCALING_PRIMITIVE) && left_button_pressed)
    {
        primitive_update_aabb(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
        SetState(state::IDLE);
        UndoSnapshot();
    }

    // primitive creation
    if (GetState() == state::CREATE_PRIMITIVE)
    {
        primitive new_primitive;
        primitive_init(&new_primitive, m_PrimitiveShape, op_union, unpacked_color(primitive_palette.entries[0]), m_Roundness, m_Width);

        if (m_PrimitiveShape == shape_arc || m_PrimitiveShape == shape_spline)
            new_primitive.m_Thickness = m_Roundness * 2.f;

        if (m_PrimitiveShape == shape_uneven_capsule)
            new_primitive.m_Radius = m_Roundness;

        for(uint32_t i=0; i<primitive_get_num_points(m_PrimitiveShape); ++i)
            primitive_set_points(&new_primitive, i, m_PrimitivePoints[i]);

        if (m_PrimitiveShape == shape_pie)
            new_primitive.m_Aperture = m_Aperture;

        primitive_update_aabb(&new_primitive);

        cc_push(&m_Primitives, new_primitive);
        SetState(state::IDLE);
        SetSelectedPrimitive(uint32_t(cc_size(&m_Primitives))-1);
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
bool PrimitivesStack::SelectPrimitive()
{
    cc_clear(&m_MultipleSelection);
    for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
    {
        primitive *p = cc_get(&m_Primitives, i);
        if (primitive_test_mouse_cursor(p, m_MousePosition, true))
            cc_push(&m_MultipleSelection, i);
    }

    size_t num_primitives = cc_size(&m_MultipleSelection);

    if (num_primitives == 0)
        return SetSelectedPrimitive(INVALID_INDEX);

    if (num_primitives == 1)
        return SetSelectedPrimitive(*cc_get(&m_MultipleSelection, 0));

    uint32_t nearest_primitive_index = INVALID_INDEX;
    float min_distance = FLT_MAX;
    for(uint32_t i=0; i<cc_size(&m_MultipleSelection); ++i)
    {
        primitive *p = cc_get(&m_Primitives, *cc_get(&m_MultipleSelection, i));
        float distance = primitive_distance_to_nearest_point(p, m_MousePosition);
        if (distance < min_distance)
        {
            min_distance = distance;
            nearest_primitive_index = i;
        }
    }

    if (nearest_primitive_index == INVALID_INDEX)
        return SetSelectedPrimitive(INVALID_INDEX);
    else
        return SetSelectedPrimitive(*cc_get(&m_MultipleSelection,nearest_primitive_index));
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Draw(struct renderer* context)
{
    // drawing *the* primitives
    renderer_set_outline_width(context, m_OutlineWidth);
    renderer_begin_combination(context, m_SmoothBlend);

    for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
    {
        primitive *primitive = cc_get(&m_Primitives, i);
        primitive_draw_alpha(primitive, context, m_AlphaValue);
        if (m_PrimitiveIdDebug)
        {
            vec2 center = primitive_compute_center(primitive);
            renderer_draw_text(context, center.x, center.y, format("%d", i), draw_color(na16_black));
        }
    }
    renderer_end_combination(context, m_GlobalOutline);

    if (GetState() == state::ADDING_POINTS)
    {
        for(uint32_t i=0; i<m_CurrentPoint; ++i)
            renderer_draw_disc(context, m_PrimitivePoints[i], primitive_point_radius, -1.f, fill_solid, m_PointColor, op_add);

        // preview primitive
        if (m_CurrentPoint == 1)
            renderer_draw_orientedbox(context, m_PrimitivePoints[0], m_MousePosition, 0.f, 0.f, -1.f, fill_solid, m_SelectedPrimitiveColor, op_add);

        if (m_PrimitiveShape == shape_triangle) 
        {
            if (m_CurrentPoint == 2 || (m_CurrentPoint == 3 && vec2_similar(m_PrimitivePoints[1], m_PrimitivePoints[2], 0.001f)))
                renderer_draw_triangle(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_MousePosition, 0.f, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
        }
        else if (m_PrimitiveShape == shape_arc)
        {
            if (m_CurrentPoint == 2 || (m_CurrentPoint == 3 && vec2_similar(m_PrimitivePoints[1], m_PrimitivePoints[2], 0.001f)))
                renderer_draw_arc_from_circle(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_MousePosition, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
        }
        else if (m_PrimitiveShape == shape_spline)
        {
            if (m_CurrentPoint == 2 || (m_CurrentPoint == 3 && vec2_similar(m_PrimitivePoints[1], m_PrimitivePoints[2], 0.001f)))
                primitive_draw_spline(context, (vec2[]) {m_PrimitivePoints[0], m_PrimitivePoints[1], m_MousePosition}, 3, 0.f, m_SelectedPrimitiveColor);
            else if (m_CurrentPoint == 3 || (m_CurrentPoint == 4 && vec2_similar(m_PrimitivePoints[2], m_PrimitivePoints[3], 0.001f)))
                primitive_draw_spline(context, (vec2[]) {m_PrimitivePoints[0], m_PrimitivePoints[1], m_PrimitivePoints[2], m_MousePosition}, 4, 0.f, m_SelectedPrimitiveColor);
        }
        renderer_draw_disc(context, m_MousePosition, primitive_point_radius, -1.f, fill_solid, m_PointColor, op_add);
    }
    else if (GetState() == state::SET_WIDTH)
    {
        renderer_draw_disc(context, m_Reference, primitive_point_radius, -1.f, fill_solid, m_PointColor, op_add);

        if (m_PrimitiveShape == shape_oriented_box)
            renderer_draw_orientedbox(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, 0.f, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
        else if (m_PrimitiveShape == shape_oriented_ellipse)
            renderer_draw_ellipse(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
    }
    else if (GetState() == state::SET_ROUNDNESS)
    {
        renderer_draw_disc(context, m_Reference, primitive_point_radius, -1.f, fill_solid, m_PointColor, op_add);

        // preview primitive
        switch(m_PrimitiveShape)
        {
        case shape_triangle:
        {
            renderer_draw_triangle(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_PrimitivePoints[2], m_Roundness, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
            break;
        }

        case shape_disc:
        {
            renderer_draw_disc(context, m_PrimitivePoints[0], m_Roundness, -1.f, fill_solid, m_SelectedPrimitiveColor, op_add);
            break;
        }

        case shape_oriented_box:
        {
            renderer_draw_orientedbox(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, m_Roundness, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
            break;
        }

        case shape_arc:
        {
            float thickness = float_min(m_Roundness * 2.f, primitive_max_thickness);
            renderer_draw_arc_from_circle(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_PrimitivePoints[2], thickness, fill_solid, m_SelectedPrimitiveColor, op_add);
            break;
        }

        case shape_spline:
        {
            float thickness = float_min(m_Roundness * 2.f, primitive_max_thickness);
            primitive_draw_spline(context, m_PrimitivePoints, primitive_get_num_points(m_PrimitiveShape), thickness, m_SelectedPrimitiveColor);
            break;
        }

        case shape_uneven_capsule:
        {
            renderer_draw_unevencapsule(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_Roundness, m_Roundness, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
            break;
        }

        default:break;
        }
    }
    else if (GetState() == state::SET_ANGLE)
    {
        renderer_draw_pie(context, m_PrimitivePoints[0], m_PrimitivePoints[1], m_Aperture, 0.f, fill_solid, m_SelectedPrimitiveColor, op_add);
    }
    else if (GetState() == state::IDLE && !m_NewPrimitiveContextualMenuOpen)
    {
        MouseCursors::GetInstance().Default();
        for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
        {
            primitive *primitive = cc_get(&m_Primitives, i);
            if (primitive_test_mouse_cursor(primitive, m_MousePosition, true))
            {
                primitive_draw_gizmo(primitive, context, m_HoveredPrimitiveColor);
                if (i == m_SelectedPrimitiveIndex)
                    MouseCursors::GetInstance().Set(MouseCursors::Hand);
            }
        }

        if (SelectedPrimitiveValid())
            primitive_draw_gizmo(cc_get(&m_Primitives, m_SelectedPrimitiveIndex), context, m_SelectedPrimitiveColor);
    }
    else if (GetState() == state::MOVING_POINT || GetState() == state::MOVING_PRIMITIVE)
    {
        if (SelectedPrimitiveValid())
        {
            primitive* primitive = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
            primitive_draw_gizmo(primitive, context, m_SelectedPrimitiveColor);

            if (m_PrimitiveIdDebug)
            {
                vec2 center = primitive_compute_center(primitive);
                renderer_draw_text(context, center.x, center.y, format("%d", m_SelectedPrimitiveIndex), draw_color(0xff000000));
            }
        }
    }
    else if (GetState() == state::ROTATING_PRIMITIVE || GetState() == state::SCALING_PRIMITIVE)
    {
        renderer_draw_disc(context, m_Reference, primitive_point_radius, -1.f, fill_solid, m_PointColor, op_add);
        primitive_draw_gizmo(cc_get(&m_Primitives, m_SelectedPrimitiveIndex), context, m_SelectedPrimitiveColor);
    }

    if (m_AABBDebug)
    {
        for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
        {
            primitive *p = cc_get(&m_Primitives, i);
            primitive_draw_aabb(p, context, draw_color(0x3fe01010));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::DuplicateSelected()
{
    if (SelectedPrimitiveValid())
    {
        primitive new_primitive = *cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
        cc_push(&m_Primitives, new_primitive);
        SetSelectedPrimitive(uint32_t(cc_size(&m_Primitives))-1);
        log_debug("primitive duplicated");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::UserInterface(struct mu_Context* gui_context)
{
    int res = 0;
    int window_options = MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE;
    if (mu_begin_window_ex(gui_context, "global control", mu_rect(50, 200, 400, 200), window_options))
    {
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_label(gui_context,"smooth blend");
        res |= mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);
        mu_label(gui_context, "alpha");
        res |= mu_slider_ex(gui_context, &m_AlphaValue, 0.f, 1.f, 0.01f, "%1.2f", 0);
        mu_label(gui_context, "global outline");
        res |= mu_checkbox(gui_context, "", &m_GlobalOutline);
        mu_label(gui_context, "outline width");
        res |= mu_slider_ex(gui_context, &m_OutlineWidth, 0.f, 20.f, 0.1f, "%2.2f", 0);
        mu_end_window(gui_context);
    }

    primitive* selected = (SelectedPrimitiveValid()) ? cc_get(&m_Primitives, m_SelectedPrimitiveIndex) : nullptr;
    if (mu_begin_window_ex(gui_context, "primitive inspector", mu_rect(50, 400, 400, 600), window_options))
    {
        if (selected)
            res |= primitive_property_grid(selected, gui_context);

        mu_end_window(gui_context);
    }

    // if something has changed, handle undo
    if (res & MU_RES_SUBMIT)
    {
        if (selected)
            primitive_update_aabb(selected);

        UndoSnapshot();
    }

    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_NewPrimitiveContextualMenuOpen)
    {
        mu_Rect window_rect;
        window_rect.h = gui_context->style->title_height * 10;
        window_rect.w = gui_context->text_width(0, "ellipseell", -1);
        window_rect.x = (int)m_ContextualMenuPosition.x - window_rect.w/2;
        window_rect.y = (int)m_ContextualMenuPosition.y - window_rect.h/2;

        aabb window_aabb =
        {
            .min = vec2_set(window_rect.x, window_rect.y),
            .max = vec2_set(window_rect.x + window_rect.w, window_rect.y + window_rect.h)
        };

        if (mu_begin_window_ex(gui_context, "new", window_rect, MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
        {
            mu_layout_row(gui_context, 1, (int[]) {90}, 0);
            if (mu_button_ex(gui_context, "disc", 0, 0))
            {
                m_PrimitiveShape = shape_disc;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "ellipse", 0, 0))
            {
                m_PrimitiveShape = shape_oriented_ellipse;
                SetState(state::ADDING_POINTS);
            }
            
            if (mu_button_ex(gui_context, "box", 0, 0))
            {
                m_PrimitiveShape = shape_oriented_box;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "triangle", 0, 0))
            {
                m_PrimitiveShape = shape_triangle;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "pie", 0, 0))
            {
                m_PrimitiveShape = shape_pie;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "arc", 0, 0))
            {
                m_PrimitiveShape = shape_arc;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "spline", 0, 0))
            {
                m_PrimitiveShape = shape_spline;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "capsule", 0, 0))
            {
                m_PrimitiveShape = shape_uneven_capsule;
                SetState(state::ADDING_POINTS);
            }

            if (!aabb_test_point(&window_aabb, m_MousePosition))
                m_NewPrimitiveContextualMenuOpen = false;

            mu_end_window(gui_context);
        }
    }
    else if (m_SelectedPrimitiveContextualMenuOpen && SelectedPrimitiveValid())
    {
        mu_Rect window_rect;
        window_rect.h = gui_context->style->title_height * 8;
        window_rect.w = gui_context->text_width(0, "rotaterota", -1);
        window_rect.x = (int)m_ContextualMenuPosition.x - window_rect.w/2;
        window_rect.y = (int)m_ContextualMenuPosition.y - window_rect.h/2;

        aabb window_aabb =
        {
            .min = vec2_set(window_rect.x, window_rect.y),
            .max = vec2_set(window_rect.x + window_rect.w, window_rect.y + window_rect.h)
        };

        if (mu_begin_window_ex(gui_context, "edit", window_rect, MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
        {
            mu_layout_row(gui_context, 1, (int[]) {90}, 0);
            if (mu_button_ex(gui_context, "rotate", 0, 0))
            {
                SetState(state::ROTATING_PRIMITIVE);
                m_SelectedPrimitiveContextualMenuOpen = false;
            }
            else if (mu_button_ex(gui_context, "scale", 0, 0))
            {
                SetState(state::SCALING_PRIMITIVE);
                m_SelectedPrimitiveContextualMenuOpen = false;
            }
            else if (mu_button_ex(gui_context, "front", 0, 0))
            {
                primitive temp = *cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
                cc_erase(&m_Primitives, m_SelectedPrimitiveIndex);
                cc_push(&m_Primitives, temp);
                SetSelectedPrimitive(INVALID_INDEX);
                UndoSnapshot();
                m_SelectedPrimitiveContextualMenuOpen = false;
            }
            else if (mu_button_ex(gui_context, "back", 0, 0))
            {
                primitive temp = *cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
                cc_erase(&m_Primitives, m_SelectedPrimitiveIndex);
                cc_insert(&m_Primitives, 0, temp);
                SetSelectedPrimitive(INVALID_INDEX);
                UndoSnapshot();
                m_SelectedPrimitiveContextualMenuOpen = false;
            }

            if (SelectedPrimitiveValid() && primitive_contextual_property_grid(cc_get(&m_Primitives, m_SelectedPrimitiveIndex), gui_context, &window_aabb))
            {
                m_SelectedPrimitiveContextualMenuOpen = false;
                UndoSnapshot();
            }

            if (!aabb_test_point(&window_aabb, m_MousePosition))
                m_SelectedPrimitiveContextualMenuOpen = false;

            mu_end_window(gui_context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Serialize(serializer_context* context, bool normalization)
{
    size_t array_size = cc_size(&m_Primitives);
    serializer_write_float(context, m_AlphaValue);
    serializer_write_float(context, m_SmoothBlend);
    serializer_write_uint32_t(context, m_SelectedPrimitiveIndex);
    serializer_write_float(context, m_OutlineWidth);

    serializer_write_size_t(context, cc_size(&m_Primitives));
     for(uint32_t i=0; i<array_size; ++i)
     {
        primitive p = *cc_get(&m_Primitives, i);
        if (normalization)
            primitive_normalize(&p, &m_EditionZone);

        primitive_serialize(&p, context);
     }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization)
{
    m_AlphaValue = serializer_read_float(context);
    m_SmoothBlend = serializer_read_float(context);
    m_SelectedPrimitiveIndex = serializer_read_uint32_t(context);

    if (minor>=5)
        m_OutlineWidth = serializer_read_float(context);

    size_t array_size = serializer_read_size_t(context);
    log_debug("%d primitives found", array_size);

    cc_resize(&m_Primitives, array_size);

    for(uint32_t i=0; i<array_size; ++i)
    {
        primitive* p = cc_get(&m_Primitives, i);

        primitive_deserialize(p, context, major, minor);
        if (normalization)
            primitive_expand(p, &m_EditionZone);
        primitive_update_aabb(p);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::UndoSnapshot()
{
    serializer_context serializer;
    size_t max_size;

    void* buffer = undo_begin_snapshot(m_pUndoContext, &max_size);
    serializer_init(&serializer, buffer, max_size);
    Serialize(&serializer, false);
    undo_end_snapshot(m_pUndoContext, buffer, serializer_get_position(&serializer));

    if (serializer_get_status(&serializer) == serializer_write_error)
        log_error("undo buffer is full");
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Undo()
{
    // cancel primitive creation
    if (GetState() == state::ADDING_POINTS || GetState() == state::SET_ROUNDNESS)
        SetState(state::IDLE);
    
    // if idle, call undo manager
    else if (GetState() == state::IDLE)
    {
        serializer_context serializer;
        size_t max_size;
        void* pBuffer = undo_undo(m_pUndoContext, &max_size);

        if (pBuffer != nullptr)
        {
            serializer_init(&serializer, pBuffer, max_size);
            Deserialize(&serializer, TDS_MAJOR, TDS_MINOR, false);
            if (serializer_get_status(&serializer) == serializer_read_error)
                log_fatal("corrupted undo buffer");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::CopySelected()
{
    if (SelectedPrimitiveValid())
    {
        m_CopiedPrimitive = *cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
        log_debug("primitive copied");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Paste()
{
    if (primitive_is_valid(&m_CopiedPrimitive))
    {
        vec2 center = primitive_compute_center(&m_CopiedPrimitive);
        primitive_translate(&m_CopiedPrimitive, m_MousePosition - center);
        primitive_update_aabb(&m_CopiedPrimitive);
        cc_push(&m_Primitives, m_CopiedPrimitive);
        SetSelectedPrimitive(uint32_t(cc_size(&m_Primitives))-1);
        UndoSnapshot();
        log_debug("primitive pasted");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::DeleteSelected()
{
    if (GetState() == state::IDLE && SelectedPrimitiveValid())
    {
        cc_erase(&m_Primitives, m_SelectedPrimitiveIndex);
        SetSelectedPrimitive(INVALID_INDEX);
        log_debug("primitive deleted");
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Export(void* window)
{
    struct string_buffer clipboard = string_buffer_init(clipboard_buffer_size);

    shadertoy_start(&clipboard);

    float normalized_smooth_blend = m_SmoothBlend / aabb_get_size(&m_EditionZone).x;
    for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
    {
        primitive p = *cc_get(&m_Primitives, i);

        primitive_normalize(&p, &m_EditionZone);
        shadertoy_export_primitive(&clipboard, &p, i, normalized_smooth_blend);
    }

    shadertoy_finalize(&clipboard);
    glfwSetClipboardString((GLFWwindow*) window, clipboard.buffer);

    string_buffer_terminate(&clipboard);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::SetState(enum state new_state)
{
    if (GetState() == state::IDLE && new_state == state::ADDING_POINTS)
    {
        m_CurrentPoint = 0;
        m_NewPrimitiveContextualMenuOpen = false;
        MouseCursors::GetInstance().Set(MouseCursors::CrossHair);
    }

    if (new_state == state::SCALING_PRIMITIVE)
    {
        m_Reference = m_MousePosition;
        m_CopiedPrimitive = *(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
        MouseCursors::GetInstance().Set(MouseCursors::HResize);
    }

    if (new_state == state::ROTATING_PRIMITIVE)
    {
        m_CopiedPrimitive = *(cc_get(&m_Primitives, m_SelectedPrimitiveIndex));
        m_Reference = primitive_compute_center(&m_CopiedPrimitive);
        m_Angle = 0.f;
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

    if (new_state == state::SET_ANGLE)
    {
        m_Aperture = 0.f;
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
void PrimitivesStack::Terminate()
{
    palette_free(&primitive_palette);
    cc_cleanup(&m_Primitives);
    cc_cleanup(&m_MultipleSelection);
}
