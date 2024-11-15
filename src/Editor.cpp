#include "renderer/Renderer.h"
#include "Editor.h"
#include "system/undo.h"
#include "system/microui.h"
#include "system/format.h"


//----------------------------------------------------------------------------------------------------------------------------
void Editor::Init(aabb zone)
{
    m_ExternalZone = m_Zone = zone;
    aabb_grow(&m_ExternalZone, vec2_splat(4.f));
    m_pUndoContext = undo_init(1<<18, 1<<10);
    m_ShapesStack.Init(zone, m_pUndoContext);
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
void Editor::DebugInterface(struct mu_Context* gui_context)
{
    if (mu_header(gui_context, "Undo"))
    {
        float undo_buffer_stat, undo_states_stat;
        undo_stats(m_pUndoContext, &undo_buffer_stat, &undo_states_stat);
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_text(gui_context, "buffer");
        mu_text(gui_context, format("%3.2f%%", undo_buffer_stat));
        mu_text(gui_context, "states");
        mu_text(gui_context, format("%3.2f%%", undo_states_stat));
        mu_text(gui_context, "stack");
        mu_text(gui_context, format("%d", undo_get_num_states(m_pUndoContext)));
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::UserInterface(struct mu_Context* gui_context)
{
    m_ShapesStack.UserInterface(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Undo()
{
    m_ShapesStack.Undo();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Terminate()
{
    undo_terminate(m_pUndoContext);
    m_ShapesStack.Terminate();
}
