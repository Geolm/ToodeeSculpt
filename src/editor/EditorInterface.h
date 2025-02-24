#pragma once

#include "../system/vec2.h"

struct mu_Context;
struct renderer;

class EditorInterface
{
public:
    EditorInterface() {}
    virtual ~EditorInterface() {}

    virtual void OnKeyEvent(int key, int scancode, int action, int mods) = 0;
    virtual void OnMouseMove(vec2 pos) = 0;
    virtual void OnMouseButton(int button, int action, int mods) = 0;
    virtual void Draw(struct renderer* context) = 0;
    virtual void DebugInterface(struct mu_Context* gui_context) = 0;

    virtual void PropertyGrid(struct mu_Context* gui_context) = 0;
    virtual void Toolbar(struct mu_Context* gui_context) = 0;
    virtual void GlobalControl(struct mu_Context* gui_context) = 0;
    virtual void UserInterface(struct mu_Context* gui_context) = 0;

    virtual void Copy() = 0;
    virtual void Paste() = 0;
    virtual void Undo() = 0;
    virtual void Delete() = 0;
};