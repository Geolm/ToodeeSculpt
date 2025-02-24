#pragma once

#define CC_USE_ASSERT
#define CC_NO_SHORT_NAMES
#include "../system/cc.h"
#include "../system/aabb.h"
#include "../system/serializer.h"
#include "../system/log.h"
#include "../shaders/common.h"
#include "primitive.h"
#include "primitive_list.h"

#include "EditorInterface.h"

struct mu_Context;
struct renderer;
struct GLFWwindow;

enum {PRIMITIVES_STACK_RESERVATION = 100};

//----------------------------------------------------------------------------------------------------------------------------
class PrimitiveEditor : public EditorInterface
{
public:
    PrimitiveEditor();
    ~PrimitiveEditor();

    virtual void OnKeyEvent(int key, int scancode, int action, int mods) {}
    virtual void OnMouseMove(vec2 pos);
    virtual void OnMouseButton(int button, int action, int mods);
    virtual void Draw(struct renderer* context);
    virtual void DebugInterface(struct mu_Context* gui_context) {}

    virtual void PropertyGrid(struct mu_Context* gui_context);
    virtual void Toolbar(struct mu_Context* gui_context);
    virtual void GlobalControl(struct mu_Context* gui_context);
    virtual void UserInterface(struct mu_Context* gui_context);

    virtual void Copy();
    virtual void Paste();
    virtual void Undo();
    virtual void Delete();

    void Init(aabb zone, struct undo_context* undo);
    void UndoSnapshot();
    void Serialize(serializer_context* context, bool normalization);
    void Deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization);
    void New();
    void Export(struct GLFWwindow* window);
    void Terminate();

    void SetSnapToGrid(bool b) {m_SnapToGrid = b;}
    void SetGridSubdivision(float f) {m_GridSubdivision = f;}
    void SetPrimimitiveIdDebug(bool flag) {m_PrimitiveIdDebug = flag;}
    void SetAABBDebug(bool flag) {m_AABBDebug = flag;}

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
    void DuplicateSelected();
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    enum state GetState() const {return m_CurrentState;}
    bool SelectPrimitive();
    inline bool SelectedPrimitiveValid() {return m_SelectedPrimitiveIndex < plist_size();}
    inline bool SetSelectedPrimitive(uint32_t index);

private:
    // serialized data 
    float m_SmoothBlend;
    float m_AlphaValue;
    uint32_t m_SelectedPrimitiveIndex;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
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
inline bool PrimitiveEditor::SetSelectedPrimitive(uint32_t index)
{
    assert(index == INVALID_INDEX || index < plist_size());
    bool different = (m_SelectedPrimitiveIndex != index);
    m_SelectedPrimitiveIndex = index;
    log_debug("selected primitive : %d", m_SelectedPrimitiveIndex);
    return different;
}

