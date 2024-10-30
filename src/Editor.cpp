#include "renderer/Renderer.h"
#include "Editor.h"


//----------------------------------------------------------------------------------------------------------------------------
void Editor::Init(aabb zone)
{
    m_ExternalZone = m_Zone = zone;
    aabb_grow(&m_ExternalZone, vec2_splat(4.f));
    m_ShapesStack.Init(zone);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseMove(vec2 pos)
{
    m_ShapesStack.OnMouseMove(pos);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseButton(vec2 pos, int button, int action)
{
    m_ShapesStack.OnMouseButton(pos, button, action);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Draw(Renderer& renderer)
{
    renderer.DrawBox(m_ExternalZone, draw_color(0, 0, 0, 0xff));
    renderer.DrawBox(m_Zone, draw_color(0xffffffff));
    m_ShapesStack.Draw(renderer);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::UserInterface(struct mu_Context* gui_context)
{
    m_ShapesStack.UserInterface(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Terminate()
{
    m_ShapesStack.Terminate();
}