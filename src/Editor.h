#pragma once

#include "ShapesStack.h"

class Renderer;
struct mu_Context;


class Editor
{
public:
    void Init(aabb zone);
    void OnMouseButton(float x, float y, int button, int action);
    void Draw(Renderer& renderer);
    void UserInterface(struct mu_Context* gui_context);
    void Terminate();

private:
    ShapesStack m_ShapesStack;
    aabb m_Zone;
    aabb m_ExternalZone;
};

