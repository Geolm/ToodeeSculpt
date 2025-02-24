#pragma once

#include "PrimitiveEditor.h"

struct undo_context;
struct GLFWwindow;

#define POPUP_STRING_LENGTH (2048)

class Editor
{
public:
    void OnKeyEvent(int key, int scancode, int action, int mods);
    void OnMouseMove(vec2 pos);
    void OnMouseButton(int button, int action, int mods);
    void Draw(struct renderer* context);
    void DebugInterface(struct mu_Context* gui_context);
    void UserInterface(struct mu_Context* gui_context);

    void Init(struct GLFWwindow* window, aabb zone, const char* folder_path);
    void New();
    void Load();
    void Save();
    void Copy();
    void Paste();
    void Undo();
    void Delete();
    void Terminate();

    bool IsDebugWindowOpen() const {return m_WindowDebugOpen == 1;}

private:
    enum MenuBarState
    {
        MenuBar_None,
        MenuBar_File,
        MenuBar_Edit,
        MenuBar_Options
    };

    void Popup(const char* title, const char* message);
    void MenuBar(struct mu_Context* gui_context);

private:
    PrimitiveEditor m_PrimitiveEditor;
    aabb m_Zone;
    aabb m_ExternalZone;
    vec2 m_PopupCoord;
    vec2 m_PopupHalfSize;
    EditorInterface* m_pActiveEditor {nullptr};
    struct undo_context* m_pUndoContext;
    struct GLFWwindow* m_Window;

    // menubar
    MenuBarState m_MenuBarState;
    int m_SnapToGrid;
    int m_ShowGrid;
    float m_GridSubdivision;
    int m_CullingDebug;
    int m_AABBDebug;
    int m_LogLevel;
    int m_LogLevelCombo;
    int m_WindowDebugOpen;

    // load/save
    const char* m_pFolderPath;

    // popup
    bool m_PopupOpen;
    char m_PopupTitle[POPUP_STRING_LENGTH];
    char m_PopupMessage[POPUP_STRING_LENGTH];
};

