#include "../renderer/Renderer.h"
#include "../system/microui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "../MouseCursors.h"
#include "../system/palettes.h"
#include "../system/point_in.h"
#include "../system/undo.h"
#include "../system/format.h"
#include "../system/hash.h"
#include "PrimitivesStack.h"

const vec2 contextual_menu_size = {100.f, 140.f};

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Init(aabb zone, struct undo_context* undo)
{
    cc_init(&m_Primitives);
    cc_reserve(&m_Primitives, PRIMITIVES_STACK_RESERVATION);
    m_NewPrimitiveContextualMenuOpen = false;
    m_EditionZone = zone;
    m_CurrentState = state::IDLE;
    m_CurrentPoint = 0;
    m_SDFOperationComboBox = 0;
    m_SmoothBlend = 1.f;
    m_AlphaValue = 1.f;
    SetSelectedPrimitive(INVALID_INDEX);
    m_pUndoContext = undo;
    m_pGrabbedPoint = nullptr;
    m_SnapToGrid = false;
    m_GridSubdivision = 20.f;
    m_CopiedPrimitive.SetInvalid();
    m_DebugInfo = false;
    cc_init(&m_MultipleSelection);
    cc_reserve(&m_MultipleSelection, PRIMITIVES_STACK_RESERVATION);
    m_MultipleSelectionHash = 0;
    m_MultipleSelectionIndex = 0;

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
    // moving point
    else if (GetState() == state::MOVING_POINT && m_pGrabbedPoint != nullptr)
    {
        *m_pGrabbedPoint = m_MousePosition;
    }
    else if (GetState() == state::MOVING_PRIMITIVE)
    {
        cc_get(&m_Primitives, m_SelectedPrimitiveIndex)->Translate(m_MousePosition - m_Reference);
        m_Reference = m_MousePosition;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::OnMouseButton(int button, int action, int mods)
{
    bool left_button_pressed = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
    // contextual menu
    if (m_NewPrimitiveContextualMenuOpen && left_button_pressed)
    {
        aabb contextual_bbox = (aabb) {.min = m_ContextualMenuPosition, .max = m_ContextualMenuPosition + contextual_menu_size};

        if (!aabb_test_point(&contextual_bbox, m_MousePosition))
            m_NewPrimitiveContextualMenuOpen = false;
    }
    if (GetState() == state::IDLE && button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (aabb_test_point(&m_EditionZone, m_MousePosition))
        {
            m_NewPrimitiveContextualMenuOpen = !m_NewPrimitiveContextualMenuOpen;
            m_ContextualMenuPosition = m_MousePosition;
            SetSelectedPrimitive(INVALID_INDEX);
        }
    }
    // selecting primitive
    else if (GetState() == state::IDLE && left_button_pressed)
    {
        if (SelectedPrimitiveValid())
        {
            Primitive *primitive = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
            for(uint32_t i=0; i<primitive->GetNumPoints(); ++i)
            {
                if (point_in_disc(primitive->GetPoints(i), Primitive::point_radius, m_MousePosition))
                {
                    m_pGrabbedPoint = &primitive->GetPoints(i);
                    *m_pGrabbedPoint = m_MousePosition;
                    SetState(state::MOVING_POINT);
                }
            }
        }
        
        if (GetState() == state::IDLE && aabb_test_point(&m_EditionZone, m_MousePosition))
        {
            bool new_selection = SelectPrimitive(mods&GLFW_MOD_SHIFT);

            // clicking on already selected primitive to move/duplicate
            if (!new_selection && GetState() == state::IDLE && SelectedPrimitiveValid() && cc_get(&m_Primitives, m_SelectedPrimitiveIndex)->TestMouseCursor(m_MousePosition, false))
            {
                // copy a primitive
                if (mods&GLFW_MOD_SUPER)
                    DuplicateSelected();
                
                m_Reference = m_MousePosition;
                SetState(state::MOVING_PRIMITIVE);
            }
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
    // moving primitive
    else if (GetState() == state::MOVING_PRIMITIVE && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        SetState(state::IDLE);
        UndoSnapshot();
    }
    // adding points
    else if (GetState() == state::ADDING_POINTS && left_button_pressed)
    {
        if (!aabb_test_point(&m_EditionZone, m_MousePosition))
            SetState(state::IDLE);

        assert(m_CurrentPoint < PRIMITIVE_MAXPOINTS);
        m_PrimitivePoints[m_CurrentPoint++] = m_MousePosition;

        if (m_CurrentPoint == Primitive::GetNumPoints(m_PrimitiveType))
        {
            if (m_PrimitiveType == command_type::primitive_oriented_box || m_PrimitiveType == command_type::primitive_ellipse)
                SetState(state::SET_WIDTH);
            else
                SetState(state::SET_ROUNDNESS);
        }
    }
    else if (GetState() == state::SET_WIDTH && left_button_pressed)
    {
        if (m_PrimitiveType == command_type::primitive_oriented_box)
            SetState(state::SET_ROUNDNESS);
        else if (m_PrimitiveType == command_type::primitive_ellipse)
            SetState(state::CREATE_PRIMITIVE);
    }
    else if (GetState() == state::SET_ROUNDNESS && left_button_pressed)
    {
        SetState(state::CREATE_PRIMITIVE);
    }

    // primitive creation
    if (GetState() == state::CREATE_PRIMITIVE)
    {
        Primitive new_primitive(m_PrimitiveType, op_union, (primitive_color) {.red = 0.8f, .green = 0.2f, .blue = 0.4f}, m_Roundness, m_Width);

        for(uint32_t i=0; i<Primitive::GetNumPoints(m_PrimitiveType); ++i)
            new_primitive.SetPoints(i, m_PrimitivePoints[i]);

        cc_push(&m_Primitives, new_primitive);
        SetState(state::IDLE);
        SetSelectedPrimitive(uint32_t(cc_size(&m_Primitives))-1);
        UndoSnapshot();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
bool PrimitivesStack::SelectPrimitive(bool cycle_through)
{
    cc_clear(&m_MultipleSelection);
    for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
    {
        Primitive *p = cc_get(&m_Primitives, i);
        if (p->TestMouseCursor(m_MousePosition, true))
            cc_push(&m_MultipleSelection, i);
    }

    if (cc_size(&m_MultipleSelection)>0)
    {
        uint32_t new_hash = hash_fnv_1a(cc_get(&m_MultipleSelection, 0), sizeof(uint32_t) * cc_size(&m_MultipleSelection));

        log_debug("clicking on %d primitives, hash : 0x%X", cc_size(&m_MultipleSelection), new_hash);

        // same selection candidate
        if (new_hash == m_MultipleSelectionHash)
        {
            // cycle through selected primitive or keep the same if shift not pressed
            if (cycle_through)
            {
                if (++m_MultipleSelectionIndex >= cc_size(&m_MultipleSelection))
                    m_MultipleSelectionIndex = 0;
            }
        }
        else m_MultipleSelectionIndex = 0;

        m_MultipleSelectionHash = new_hash;
        return SetSelectedPrimitive(*cc_get(&m_MultipleSelection, m_MultipleSelectionIndex));
    }
    else
    {
        SetSelectedPrimitive(INVALID_INDEX);
        return false;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Draw(Renderer& renderer)
{
    // drawing *the* primitives
    renderer.BeginCombination(m_SmoothBlend);
    for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
    {
        Primitive *primitive = cc_get(&m_Primitives, i);
        primitive->Draw(renderer, m_AlphaValue);
        if (m_DebugInfo)
            renderer.DrawText(primitive->ComputerCenter(), format("%d", i), draw_color(na16_black));
    }
    renderer.EndCombination();

    if (GetState() == state::ADDING_POINTS)
    {
        for(uint32_t i=0; i<m_CurrentPoint; ++i)
            renderer.DrawCircleFilled(m_PrimitivePoints[i], Primitive::point_radius, draw_color(na16_red, 128));

        // preview primitive
        if (m_PrimitiveType == command_type::primitive_triangle_filled) 
        {
            if (m_CurrentPoint == 1)
                renderer.DrawOrientedBox(m_PrimitivePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
            else if (m_CurrentPoint == 2 || (m_CurrentPoint == 3 && vec2_similar(m_PrimitivePoints[1], m_PrimitivePoints[2], 0.001f)))
                renderer.DrawTriangleFilled(m_PrimitivePoints[0], m_PrimitivePoints[1], m_MousePosition, 0.f, draw_color(na16_light_blue, 128));
        }
        else if ((m_PrimitiveType == command_type::primitive_oriented_box || m_PrimitiveType == command_type::primitive_ellipse)
                  && m_CurrentPoint == 1)
        {
            renderer.DrawOrientedBox(m_PrimitivePoints[0], m_MousePosition, 0.f, 0.f, draw_color(na16_light_blue, 128));
        }
        renderer.DrawCircleFilled(m_MousePosition, Primitive::point_radius, draw_color(na16_red, 128));
    }
    else if (GetState() == state::SET_WIDTH)
    {
        renderer.DrawCircleFilled(m_Reference, Primitive::point_radius, draw_color(na16_red, 128));

        if (m_PrimitiveType == command_type::primitive_oriented_box)
            renderer.DrawOrientedBox(m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, 0.f, draw_color(na16_light_blue, 128));
        else if (m_PrimitiveType == command_type::primitive_ellipse)
            renderer.DrawEllipse(m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, draw_color(na16_light_blue, 128));
    }
    else if (GetState() == state::SET_ROUNDNESS)
    {
        renderer.DrawCircleFilled(m_Reference, Primitive::point_radius, draw_color(na16_red, 128));

        // preview primitive
        switch(m_PrimitiveType)
        {
        case command_type::primitive_triangle_filled:
            renderer.DrawTriangleFilled(m_PrimitivePoints[0], m_PrimitivePoints[1], m_PrimitivePoints[2], 
                                        m_Roundness, draw_color(na16_light_blue, 128));break;

        case command_type::primitive_circle_filled:
            renderer.DrawCircleFilled(m_PrimitivePoints[0], m_Roundness, draw_color(na16_light_blue, 128));break;

        case command_type::primitive_oriented_box:
            renderer.DrawOrientedBox(m_PrimitivePoints[0], m_PrimitivePoints[1], m_Width, m_Roundness, draw_color(na16_light_blue, 128));
            break;

        default:break;
        }
    }
    else if (GetState() == state::IDLE)
    {
        MouseCursors::GetInstance().Default();
        for(uint32_t i=0; i<cc_size(&m_Primitives); ++i)
        {
            Primitive *primitive = cc_get(&m_Primitives, i);
            if (primitive->TestMouseCursor(m_MousePosition, true))
            {
                primitive->DrawGizmo(renderer);
                if (i == m_SelectedPrimitiveIndex)
                    MouseCursors::GetInstance().Set(MouseCursors::Hand);
            }
        }

        if (SelectedPrimitiveValid())
            cc_get(&m_Primitives, m_SelectedPrimitiveIndex)->DrawGizmo(renderer);
    }
    else if (GetState() == state::MOVING_POINT || GetState() == state::MOVING_PRIMITIVE)
    {
        if (SelectedPrimitiveValid())
        {
            Primitive* primitive = cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
            primitive->DrawGizmo(renderer);

            if (m_DebugInfo)
                renderer.DrawText(primitive->ComputerCenter(), format("%d", m_SelectedPrimitiveIndex), draw_color(na16_black));
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::DuplicateSelected()
{
    if (SelectedPrimitiveValid())
    {
        Primitive new_primitive = *cc_get(&m_Primitives, m_SelectedPrimitiveIndex);
        cc_push(&m_Primitives, new_primitive);
        SetSelectedPrimitive(uint32_t(cc_size(&m_Primitives))-1);
        log_debug("primitive duplicated");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::UserInterface(struct mu_Context* gui_context)
{
    int res = 0;
    if (mu_begin_window_ex(gui_context, "global control", mu_rect(50, 100, 400, 200), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
    {
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_label(gui_context,"smoothness");
        res |= mu_slider_ex(gui_context, &m_SmoothBlend, 0.f, 100.f, 1.f, "%3.0f", 0);
        mu_label(gui_context, "alpha");
        res |= mu_slider_ex(gui_context, &m_AlphaValue, 0.f, 1.f, 0.01f, "%1.2f", 0);
        mu_end_window(gui_context);
    }
    if (mu_begin_window_ex(gui_context, "primitive inspector", mu_rect(50, 500, 400, 400), MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
    {
        if (SelectedPrimitiveValid() && mu_header_ex(gui_context, "selected primitive", MU_OPT_EXPANDED))
            res |= cc_get(&m_Primitives, m_SelectedPrimitiveIndex)->PropertyGrid(gui_context);

        mu_end_window(gui_context);
    }

    // if something has changed, handle undo
    if (res & MU_RES_SUBMIT)
        UndoSnapshot();

    ContextualMenu(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::ContextualMenu(struct mu_Context* gui_context)
{
    if (m_NewPrimitiveContextualMenuOpen)
    {
        if (mu_begin_window_ex(gui_context, "new", mu_rect((int)m_ContextualMenuPosition.x,
            (int)m_ContextualMenuPosition.y, (int)contextual_menu_size.x, (int)contextual_menu_size.y), 
            MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOCLOSE))
        {
            mu_layout_row(gui_context, 1, (int[]) {90}, 0);
            if (mu_button_ex(gui_context, "disc", 0, 0)&MU_RES_SUBMIT)
            {
                m_PrimitiveType = command_type::primitive_circle_filled;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "ellipse", 0, 0)&MU_RES_SUBMIT)
            {
                m_PrimitiveType = command_type::primitive_ellipse;
                SetState(state::ADDING_POINTS);
            }
            
            if (mu_button_ex(gui_context, "box", 0, 0)&MU_RES_SUBMIT)
            {
                m_PrimitiveType = command_type::primitive_oriented_box;
                SetState(state::ADDING_POINTS);
            }

            if (mu_button_ex(gui_context, "triangle", 0, 0)&MU_RES_SUBMIT)
            {
                m_PrimitiveType = command_type::primitive_triangle_filled;
                SetState(state::ADDING_POINTS);
            }
            mu_end_window(gui_context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
size_t PrimitivesStack::GetSerializedDataLength()
{
    return 2 * sizeof(float) + sizeof(uint32_t) + sizeof(size_t) + cc_size(&m_Primitives) * sizeof(Primitive);
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Serialize(serializer_context* context)
{
    size_t array_size = cc_size(&m_Primitives);
    serializer_write_float(context, m_AlphaValue);
    serializer_write_float(context, m_SmoothBlend);
    serializer_write_uint32_t(context, m_SelectedPrimitiveIndex);
    serializer_write_size_t(context, cc_size(&m_Primitives));
    if (array_size != 0)
        serializer_write_blob(context, cc_get(&m_Primitives, 0), cc_size(&m_Primitives) * sizeof(Primitive));
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::Deserialize(serializer_context* context)
{
    m_AlphaValue = serializer_read_float(context);
    m_SmoothBlend = serializer_read_float(context);
    m_SelectedPrimitiveIndex = serializer_read_uint32_t(context);
    size_t array_size = serializer_read_size_t(context);
    cc_resize(&m_Primitives, array_size);
    if (array_size != 0)
        serializer_read_blob(context, cc_get(&m_Primitives, 0), array_size * sizeof(Primitive));
}

//----------------------------------------------------------------------------------------------------------------------------
void PrimitivesStack::UndoSnapshot()
{
    serializer_context serializer;
    size_t max_size;

    void* buffer = undo_begin_snapshot(m_pUndoContext, &max_size);
    serializer_init(&serializer, buffer, max_size);
    Serialize(&serializer);
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
    else if (GetState() == state::IDLE || GetState() == state::PRIMITIVE_SELECTED)
    {
        serializer_context serializer;
        size_t max_size;
        void* pBuffer = undo_undo(m_pUndoContext, &max_size);

        if (pBuffer != nullptr)
        {
            serializer_init(&serializer, pBuffer, max_size);
            Deserialize(&serializer);
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
    if (m_CopiedPrimitive.IsValid())
    {
        vec2 center = m_CopiedPrimitive.ComputerCenter();
        m_CopiedPrimitive.Translate(m_MousePosition - center);
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
void PrimitivesStack::SetState(enum state new_state)
{
    if (GetState() == state::IDLE && new_state == state::ADDING_POINTS)
    {
        m_CurrentPoint = 0;
        m_NewPrimitiveContextualMenuOpen = false;
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
void PrimitivesStack::Terminate()
{
    cc_cleanup(&m_Primitives);
}
