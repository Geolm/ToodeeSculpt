#include "../renderer/renderer.h"
#include "../system/microui.h"
#include "../MouseCursors.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/undo.h"
#include "../system/format.h"
#include "PrimitiveEditor.h"
#include "tds.h"
#include "../icons.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define UNUSED_VARIABLE(a) (void)(a)

struct palette primitive_palette;

//----------------------------------------------------------------------------------------------------------------------------
PrimitiveEditor::PrimitiveEditor() : EditorInterface()
{
}

//----------------------------------------------------------------------------------------------------------------------------
PrimitiveEditor::~PrimitiveEditor()
{
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Init(struct GLFWwindow* window, aabb zone, struct undo_context* undo)
{
    plist_init(PRIMITIVES_STACK_RESERVATION);
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
    m_GlobalOutline = 0;
    m_pWindow = window;
    palette_default(&primitive_palette);
    New();
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::New()
{
    m_SelectedPrimitiveContextualMenuOpen = false;
    m_CurrentState = state::IDLE;
    m_CurrentPoint = 0;
    m_SDFOperationComboBox = 0;
    m_SmoothBlend = 1.f;
    m_AlphaValue = 1.f;
    SetSelectedPrimitive(INVALID_INDEX);
    primitive_set_invalid(&m_CopiedPrimitive);
    m_pGrabbedPoint = nullptr;
    plist_clear();
    cc_clear(&m_MultipleSelection);
    UndoSnapshot();
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::OnKeyEvent(int key, int scancode, int action, int mods)
{
    UNUSED_VARIABLE(key);
    UNUSED_VARIABLE(scancode);
    UNUSED_VARIABLE(action);
    UNUSED_VARIABLE(mods);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::OnMouseMove(vec2 pos) 
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

    primitive* selected = SelectedPrimitiveValid() ? plist_get(m_SelectedPrimitiveIndex) : NULL;

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
        primitive_update_aabb(selected);
    }
    else if (GetState() == state::MOVING_PRIMITIVE)
    {
        primitive_translate(selected, m_MousePosition - m_Reference);
        primitive_update_aabb(selected);
        m_Reference = m_MousePosition;
    }
    else if (GetState() == state::ROTATING_PRIMITIVE)
    {
        *selected = m_CopiedPrimitive;
        primitive_rotate(selected, vec2_atan2(m_MousePosition - m_Reference));
        primitive_update_aabb(selected);
    }
    else if (GetState() == state::SCALING_PRIMITIVE)
    {
        *selected = m_CopiedPrimitive;

        float scale = (m_MousePosition.x - m_Reference.x); ;
        if (scale>0.f)
            scale = (scale / (aabb_get_size(&m_EditionZone).x * 0.1f)) + 1.f;
        else
            scale = float_max(1.f + (scale / (aabb_get_size(&m_EditionZone).x * 0.1f)), 0.1f);

        primitive_scale(selected, scale);
        primitive_update_aabb(selected);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::OnMouseButton(int button, int action, int mods)
{
    bool left_button_pressed = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) && !m_SelectedPrimitiveContextualMenuOpen;

    if (GetState() == state::IDLE && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (aabb_test_point(&m_EditionZone, m_MousePosition) && SelectedPrimitiveValid())
        {
            m_SelectedPrimitiveContextualMenuOpen = !m_SelectedPrimitiveContextualMenuOpen;
            m_ContextualMenuPosition = m_MousePosition;
        }
    }
    // selecting primitive
    else if (GetState() == state::IDLE && left_button_pressed && aabb_test_point(&m_EditionZone, m_MousePosition))
    {
        if (SelectedPrimitiveValid())
        {
            primitive *primitive = plist_get(m_SelectedPrimitiveIndex);
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
            primitive_update_aabb(plist_get(m_SelectedPrimitiveIndex));
            SetState(state::IDLE);
            UndoSnapshot();
        }
    }
    // moving primitive
    else if (GetState() == state::MOVING_PRIMITIVE && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        primitive_update_aabb(plist_get(m_SelectedPrimitiveIndex));
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
        primitive_update_aabb(plist_get(m_SelectedPrimitiveIndex));
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

        plist_push(&new_primitive);
        SetState(state::IDLE);
        SetSelectedPrimitive(plist_last());
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
bool PrimitiveEditor::SelectPrimitive()
{
    cc_clear(&m_MultipleSelection);
    for(uint32_t i=0; i<plist_size(); ++i)
    {
        primitive *p = plist_get(i);
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
        primitive *p = plist_get(*cc_get(&m_MultipleSelection, i));
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
void PrimitiveEditor::Draw(struct renderer* context)
{
    renderer_set_cliprect(context, (int)m_EditionZone.min.x, (int)m_EditionZone.min.y, (int)m_EditionZone.max.x, (int)m_EditionZone.max.y);

    // drawing *the* primitives
    renderer_begin_combination(context, m_SmoothBlend);

    for(uint32_t i=0; i<plist_size(); ++i)
    {
        primitive *primitive = plist_get(i);
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
    else if (GetState() == state::IDLE)
    {
        MouseCursors::GetInstance().Default();
        for(uint32_t i=0; i<plist_size(); ++i)
        {
            primitive *primitive = plist_get(i);
            if (primitive_test_mouse_cursor(primitive, m_MousePosition, true))
            {
                primitive_draw_gizmo(primitive, context, m_HoveredPrimitiveColor);
                if (i == m_SelectedPrimitiveIndex)
                    MouseCursors::GetInstance().Set(MouseCursors::Hand);
            }
        }

        if (SelectedPrimitiveValid())
            primitive_draw_gizmo(plist_get(m_SelectedPrimitiveIndex), context, m_SelectedPrimitiveColor);
    }
    else if (GetState() == state::MOVING_POINT || GetState() == state::MOVING_PRIMITIVE)
    {
        if (SelectedPrimitiveValid())
        {
            primitive* primitive = plist_get(m_SelectedPrimitiveIndex);
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
        primitive_draw_gizmo(plist_get(m_SelectedPrimitiveIndex), context, m_SelectedPrimitiveColor);
    }

    if (m_AABBDebug)
    {
        for(uint32_t i=0; i<plist_size(); ++i)
        {
            primitive *p = plist_get(i);
            primitive_draw_aabb(p, context, draw_color(0x3fe01010));
        }
    }

    renderer_set_cliprect(context, 0, 0, UINT16_MAX, UINT16_MAX);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::DuplicateSelected()
{
    if (SelectedPrimitiveValid())
    {
        primitive new_primitive = *plist_get(m_SelectedPrimitiveIndex);
        plist_push(&new_primitive);
        SetSelectedPrimitive(plist_last());
        log_debug("primitive duplicated");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::DebugInterface(struct mu_Context* gui_context)
{
    UNUSED_VARIABLE(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::PropertyGrid(struct mu_Context* gui_context)
{
    primitive* selected = (SelectedPrimitiveValid()) ? plist_get(m_SelectedPrimitiveIndex) : nullptr;
    if (selected && primitive_property_grid(selected, gui_context)& MU_RES_SUBMIT)
    {
        primitive_update_aabb(selected);
        UndoSnapshot();
    }
    
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Toolbar(struct mu_Context* gui_context)
{
    if (mu_header_ex(gui_context, "new", MU_OPT_EXPANDED))
    {
        mu_layout_row(gui_context, 4, (int[]) { 50, 50, 50, 50}, 0);
        mu_layout_height(gui_context, 50);
        
        if (mu_button_ex(gui_context, NULL, ICON_DISC, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_disc;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_ORIENTEDBOX, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_oriented_box;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_ELLIPSE, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_oriented_ellipse;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_PIE, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_pie;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_ARC, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_arc;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_TRIANGLE, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_triangle;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_CAPSULE, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_uneven_capsule;
            SetState(state::ADDING_POINTS);
        }

        if (mu_button_ex(gui_context, NULL, ICON_SPLINE, 0) && GetState() == state::IDLE)
        {
            m_PrimitiveShape = shape_spline;
            SetState(state::ADDING_POINTS);
        }
    }

    if (mu_header_ex(gui_context, "edit", MU_OPT_EXPANDED))
    {
        primitive* selected = (SelectedPrimitiveValid()) ? plist_get(m_SelectedPrimitiveIndex) : nullptr;
        mu_layout_row(gui_context, 4, (int[]) { 50, 50, 50, 50}, 0);
        mu_layout_height(gui_context, 50);

        if (mu_button_ex(gui_context, NULL, ICON_SCALE, 0) && selected && GetState() == state::IDLE)
        {
            m_Reference = selected->m_AABB.max;
            glfwSetCursorPos(m_pWindow, m_Reference.x, m_Reference.y);
            SetState(state::SCALING_PRIMITIVE);
        }

        if (mu_button_ex(gui_context, NULL, ICON_ROTATE, 0) && selected && GetState() == state::IDLE)
        {
            SetState(state::ROTATING_PRIMITIVE);
            m_Reference = primitive_compute_center(&m_CopiedPrimitive);
            glfwSetCursorPos(m_pWindow, selected->m_AABB.max.x, m_Reference.y);
        }

        if (mu_button_ex(gui_context, NULL, ICON_LAYERUP, 0) && selected && GetState() == state::IDLE)
        {
            primitive temp = *plist_get(m_SelectedPrimitiveIndex);
            plist_erase(m_SelectedPrimitiveIndex);
            plist_push(&temp);
            SetSelectedPrimitive(plist_last());
            UndoSnapshot();
        }

        if (mu_button_ex(gui_context, NULL, ICON_LAYERDOWN, 0) && selected && GetState() == state::IDLE)
        {
            primitive temp = *plist_get(m_SelectedPrimitiveIndex);
            plist_erase(m_SelectedPrimitiveIndex);
            plist_insert(0, &temp);
            SetSelectedPrimitive(0);
            UndoSnapshot();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::GlobalControl(struct mu_Context* gui_context)
{
    mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
    mu_label(gui_context,"smooth blend");
    int res = mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);
    mu_label(gui_context, "alpha");
    res |= mu_slider_ex(gui_context, &m_AlphaValue, 0.f, 1.f, 0.01f, "%1.2f", 0);
    mu_label(gui_context, "global outline");
    res |= mu_checkbox(gui_context, "", &m_GlobalOutline);

    if (res&MU_RES_SUBMIT)
        UndoSnapshot();
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::UserInterface(struct mu_Context* gui_context)
{
    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_SelectedPrimitiveContextualMenuOpen && SelectedPrimitiveValid())
    {
        if (mu_begin_window_ex(gui_context, "edit", mu_rect((int)m_ContextualMenuPosition.x, (int)m_ContextualMenuPosition.y, 110, 80),
            MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE|MU_OPT_AUTOSIZE))
        {
            int res = primitive_contextual_property_grid(plist_get(m_SelectedPrimitiveIndex), gui_context);

            m_SelectedPrimitiveContextualMenuOpen = (res == 0);
            if (res&MU_RES_SUBMIT)
                UndoSnapshot();
            
            mu_end_window(gui_context);
        }
        
        
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Serialize(serializer_context* context, bool normalization)
{
    serializer_write_float(context, m_AlphaValue);
    serializer_write_float(context, m_SmoothBlend);
    serializer_write_uint32_t(context, m_SelectedPrimitiveIndex);
    plist_serialize(context, normalization, &m_EditionZone);
    palette_serialize(context, &primitive_palette);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization)
{
    m_AlphaValue = serializer_read_float(context);
    m_SmoothBlend = serializer_read_float(context);
    m_SelectedPrimitiveIndex = serializer_read_uint32_t(context);

    if (minor==5)
        serializer_read_float(context); //skip outline width

    plist_deserialize(context, major, minor, normalization, &m_EditionZone);

    if (minor >= 7)
        palette_deserialize(context, &primitive_palette);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::UndoSnapshot()
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
void PrimitiveEditor::Undo()
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
void PrimitiveEditor::Copy()
{
    if (SelectedPrimitiveValid())
    {
        m_CopiedPrimitive = *plist_get(m_SelectedPrimitiveIndex);
        log_debug("primitive copied");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Paste()
{
    if (primitive_is_valid(&m_CopiedPrimitive))
    {
        vec2 center = primitive_compute_center(&m_CopiedPrimitive);
        primitive_translate(&m_CopiedPrimitive, m_MousePosition - center);
        primitive_update_aabb(&m_CopiedPrimitive);
        plist_push(&m_CopiedPrimitive);
        SetSelectedPrimitive(plist_last());
        UndoSnapshot();
        log_debug("primitive pasted");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Delete()
{
    if (GetState() == state::IDLE && SelectedPrimitiveValid())
    {
        plist_erase(m_SelectedPrimitiveIndex);
        SetSelectedPrimitive(INVALID_INDEX);
        log_debug("primitive deleted");
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::Export()
{
    plist_export(m_pWindow, m_SmoothBlend, &m_EditionZone);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitiveEditor::SetState(enum state new_state)
{
    if (GetState() == state::IDLE && new_state == state::ADDING_POINTS)
    {
        m_CurrentPoint = 0;
        MouseCursors::GetInstance().Set(MouseCursors::CrossHair);
    }

    if (new_state == state::SCALING_PRIMITIVE)
    {
        m_CopiedPrimitive = *(plist_get(m_SelectedPrimitiveIndex));
        MouseCursors::GetInstance().Set(MouseCursors::HResize);
    }

    if (new_state == state::ROTATING_PRIMITIVE)
    {
        m_CopiedPrimitive = *(plist_get(m_SelectedPrimitiveIndex));

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
void PrimitiveEditor::Terminate()
{
    palette_free(&primitive_palette);
    plist_terminate();
    cc_cleanup(&m_MultipleSelection);
}
