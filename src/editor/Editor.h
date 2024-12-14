#pragma once

#include "PrimitivesStack.h"

class Renderer;
struct mu_Context;
struct undo_context;

#define POPUP_STRING_LENGTH (2048)

class Editor
{
public:
    void Init(aabb zone, const char* folder_path);
    void OnKeyEvent(int key, int scancode, int action, int mods);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action, int mods);
    void Draw(Renderer& renderer);
    void DebugInterface(struct mu_Context* gui_context);
    void UserInterface(struct mu_Context* gui_context);
    void Terminate();

    void Popup(const char* title, const char* message);

    void New();
    void Copy();
    void Paste();
    void Load();
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

    // menubar
    MenuBarState m_MenuBarState;
    int m_SnapToGrid;
    int m_ShowGrid;
    float m_GridSubdivision;
    int m_CullingDebug;
    int m_LogLevel;
    int m_LogLevelCombo;

    // load/save
    const char* m_pFolderPath;

    // popup
    bool m_PopupOpen;
    char m_PopupTitle[POPUP_STRING_LENGTH];
    char m_PopupMessage[POPUP_STRING_LENGTH];
};

