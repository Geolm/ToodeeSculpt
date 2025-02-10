#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "../system/cc.h"
#include "../system/aabb.h"
#include "../system/serializer.h"
#include "../system/log.h"
#include "../shaders/common.h"
#include "primitive.h"

struct mu_Context;
struct renderer;

enum {PRIMITIVES_STACK_RESERVATION = 100};

//----------------------------------------------------------------------------------------------------------------------------
class PrimitivesStack
{
public:
    void Init(aabb zone, struct undo_context* undo);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action, int mods);
    void Draw(struct renderer* context);
    void UserInterface(struct mu_Context* gui_context);
    void UndoSnapshot();
    void Undo();
    void Serialize(serializer_context* context, bool normalization);
    void Deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization);
    void CopySelected();
    void Paste();
    void DeleteSelected();
    void New();
    void Export(void* window);
    void Terminate();

    void DuplicateSelected();
    void SetSnapToGrid(bool b) {m_SnapToGrid = b;}
    void SetGridSubdivision(float f) {m_GridSubdivision = f;}
    void SetPrimimitiveIdDebug(bool flag) {m_PrimitiveIdDebug = flag;}
    void setAABBDebug(bool flag) {m_AABBDebug = flag;}

private:
    
    enum state
    {
        IDLE,
        ADDING_POINTS,
        SET_WIDTH,
        SET_ROUNDNESS,
        SET_ANGLE,
        CREATE_PRIMITIVE,
        MOVING_POINT,
        MOVING_PRIMITIVE,
        ROTATING_PRIMITIVE,
        SCALING_PRIMITIVE
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    enum state GetState() const {return m_CurrentState;}
    bool SelectPrimitive();
    inline bool SelectedPrimitiveValid() {return m_SelectedPrimitiveIndex < cc_size(&m_Primitives);}
    inline bool SetSelectedPrimitive(uint32_t index);

private:
    // serialized data 
    cc_vec(primitive) m_Primitives;
    float m_SmoothBlend;
    float m_AlphaValue;
    uint32_t m_SelectedPrimitiveIndex;
    float m_OutlineWidth;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
    bool m_NewPrimitiveContextualMenuOpen;
    bool m_SelectedPrimitiveContextualMenuOpen;
    int m_SDFOperationComboBox;
    vec2 m_MousePosition;
    vec2 m_MouseLastPosition;
    bool m_PrimitiveIdDebug;
    bool m_AABBDebug;
    bool m_SnapToGrid;
    float m_GridSubdivision;
    int m_GlobalOutline;

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
    float m_Angle;
    primitive_shape m_PrimitiveShape;

    // primitive selection
    cc_vec(uint32_t) m_MultipleSelection;

    struct undo_context* m_pUndoContext;
    vec2* m_pGrabbedPoint;
    primitive m_CopiedPrimitive;
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

