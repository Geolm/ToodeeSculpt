#pragma once

#include "../system/vec2.h"

struct mu_Context;
struct renderer;

class BaseEditor
{
public:
    BaseEditor() : m_Active(false) {}
    virtual ~BaseEditor() {}

    virtual void OnKeyEvent(int key, int scancode, int action, int mods) {}
    virtual void OnMouseMove(vec2 pos) {}
    virtual void OnMouseButton(int button, int action, int mods) {}
    virtual void Draw(struct renderer* context) {}
    virtual void DebugInterface(struct mu_Context* gui_context) {}
    virtual void UserInterface(struct mu_Context* gui_context) {}

    virtual void Copy() {}
    virtual void Paste() {}
    virtual void Undo() {}
    virtual void Delete() {}
    
    bool IsActive() const {return m_Active;}
    void SetActive(bool value) {m_Active = value;}

private:
    bool m_Active;
};