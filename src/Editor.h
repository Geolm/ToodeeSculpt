#pragma once

#include "ShapesStack.h"

class Renderer;
struct mu_Context;
struct undo_context;

class Editor
{
public:
    void Init(aabb zone);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(vec2 pos, int button, int action);
    void Undo();
    void Delete();
    void Draw(Renderer& renderer);
    void DebugInterface(struct mu_Context* gui_context);
    void UserInterface(struct mu_Context* gui_context);
    void Terminate();

private:
    enum MenuBarState
    {
        MenuBar_None,
        MenuBar_File,
        MenuBar_Edit,
        MenuBar_Options
    };

    void MenuBar(struct mu_Context* gui_context);

private:
    ShapesStack m_ShapesStack;
    aabb m_Zone;
    aabb m_ExternalZone;
    struct undo_context* m_pUndoContext;

    MenuBarState m_MenuBarState;
    int m_SnapToGrid;
    int m_ShowGrid;
    float m_GridSubdivision;
};

