#pragma once

#include "PrimitivesStack.h"

class Renderer;
struct mu_Context;
struct undo_context;

class Editor
{
public:
    void Init(aabb zone, const char* folder_path);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action);
    void Draw(Renderer& renderer);
    void DebugInterface(struct mu_Context* gui_context);
    void UserInterface(struct mu_Context* gui_context);
    void Terminate();

    void Save();
    void Undo();
    void Delete();

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
    PrimitivesStack m_PrimitivesStack;
    aabb m_Zone;
    aabb m_ExternalZone;
    vec2 m_PopupCoord;
    vec2 m_PopupHalfSize;
    struct undo_context* m_pUndoContext;

    MenuBarState m_MenuBarState;
    int m_SnapToGrid;
    int m_ShowGrid;
    float m_GridSubdivision;

    // load/save
    const char* m_pFolderPath;
    bool m_SaveFailed;
    bool m_PopupOpen;
};

