#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "../system/cc.h"
#include "../system/aabb.h"
#include "../system/serializer.h"
#include "../shaders/common.h"
#include "Primitive.h"

struct mu_Context;
class Renderer;

enum {PRIMITIVES_STACK_RESERVATION = 100};

//----------------------------------------------------------------------------------------------------------------------------
class PrimitivesStack
{
public:
    void Init(aabb zone, struct undo_context* undo);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action, int mods);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void UndoSnapshot();
    void Undo();
    size_t GetSerializedDataLength();
    void Serialize(serializer_context* context);
    void Deserialize(serializer_context* context);
    void CopySelected();
    void Paste();
    void DeleteSelected();
    void Terminate();

    void DuplicateSelected();
    void SetSnapToGrid(bool b) {m_SnapToGrid = b;}
    void SetGridSubdivision(float f) {m_GridSubdivision = f;}
    void SetDebugInfo(bool flag) {m_DebugInfo = flag;}

private:
    
    enum state
    {
        IDLE,
        PRIMITIVE_SELECTED,
        ADDING_POINTS,
        SET_WIDTH,
        SET_ROUNDNESS,
        SET_ANGLE,
        CREATE_PRIMITIVE,
        MOVING_POINT,
        MOVING_PRIMITIVE
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    enum state GetState() const {return m_CurrentState;}
    bool SelectPrimitive(bool cycle_through);
    inline bool SelectedPrimitiveValid() {return m_SelectedPrimitiveIndex < cc_size(&m_Primitives);}
    inline bool SetSelectedPrimitive(uint32_t index);

private:
    // serialized data 
    cc_vec(Primitive) m_Primitives;
    float m_SmoothBlend;
    float m_AlphaValue;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
    bool m_NewPrimitiveContextualMenuOpen;
    bool m_SelectedPrimitiveContextualMenuOpen;
    int m_SDFOperationComboBox;
    vec2 m_MousePosition;
    vec2 m_MouseLastPosition;
    bool m_DebugInfo;
    bool m_SnapToGrid;
    float m_GridSubdivision;

    draw_color m_PointColor;
    draw_color m_SelectedPrimitiveColor;
    draw_color m_HoveredPrimitiveColor;

    // primitive creation
    state m_CurrentState;
    uint32_t m_CurrentPoint;
    vec2 m_PrimitivePoints[PRIMITIVE_MAXPOINTS];
    vec2 m_Reference;
    vec2 m_StartingPoint;
    vec2 m_Direction;
    float m_Width;
    float m_Roundness;
    float m_Aperture;
    command_type m_PrimitiveType;

    // primitive selection
    uint32_t m_SelectedPrimitiveIndex;
    uint32_t m_MultipleSelectionHash;
    uint32_t m_MultipleSelectionIndex;
    cc_vec(uint32_t) m_MultipleSelection;

    struct undo_context* m_pUndoContext;
    vec2* m_pGrabbedPoint;
    Primitive m_CopiedPrimitive;
};

//----------------------------------------------------------------------------------------------------------------------------
inline bool PrimitivesStack::SetSelectedPrimitive(uint32_t index)
{
    assert(index == INVALID_INDEX || index < cc_size(&m_Primitives));
    bool different = (m_SelectedPrimitiveIndex != index);
    m_SelectedPrimitiveIndex = index;
    log_debug("selected primitive : %d", m_SelectedPrimitiveIndex);
    return different;
}

