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
    void OnMouseButton(int button, int action);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void Undo();
    size_t GetSerializedDataLength();
    void Serialize(serializer_context* context);
    void Deserialize(serializer_context* context);
    void DeleteSelected();
    void Terminate();

    void SetSnapToGrid(bool b) {m_SnapToGrid = b;}
    void SetGridSubdivision(float f) {m_GridSubdivision = f;}

private:
    
    enum state
    {
        IDLE,
        PRIMITIVE_SELECTED,
        ADDING_POINTS,
        SET_WIDTH,
        SET_ROUNDNESS,
        CREATE_PRIMITIVE,
        MOVING_POINT,
        MOVING_PRIMITIVE
    };

private:
    void ContextualMenu(struct mu_Context* gui_context);
    void SetState(enum state new_state);
    enum state GetState() const {return m_CurrentState;}
    void UndoSnapshot();
    inline bool SelectedPrimitiveValid() {return m_SelectedPrimitiveIndex < cc_size(&m_Primitives);}
    

private:
    // serialized data 
    cc_vec(Primitive) m_Primitives;
    float m_SmoothBlend;
    float m_AlphaValue;

    // ui
    aabb m_EditionZone;
    vec2 m_ContextualMenuPosition;
    bool m_NewPrimitiveContextualMenuOpen;
    int m_SDFOperationComboBox;
    vec2 m_MousePosition;

    // primitive creation
    state m_CurrentState;
    uint32_t m_CurrentPoint;
    vec2 m_PrimitivePoints[PRIMITIVE_MAXPOINTS];
    vec2 m_Reference;
    float m_Width;
    float m_Roundness;
    command_type m_PrimitiveType;
    uint32_t m_SelectedPrimitiveIndex;
    struct undo_context* m_pUndoContext;
    vec2* m_pGrabbedPoint;
    bool m_SnapToGrid;
    float m_GridSubdivision;
};

