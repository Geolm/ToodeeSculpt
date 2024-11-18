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
    m_MenuBarState = MenuBar_None;
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
    MenuBar(gui_context);
    m_ShapesStack.UserInterface(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::MenuBar(struct mu_Context* gui_context)
{
    int text_height = gui_context->style->title_height;
    int padding = gui_context->style->padding;
    int row_size = gui_context->text_width(NULL, NULL, 10);
    int window_options = MU_OPT_NOSCROLL|MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOTITLE;

    if (mu_begin_window_ex(gui_context, "shape combiner", mu_rect(0, 0, row_size*3 + 20, text_height + padding), window_options))
    {
        mu_layout_row(gui_context, 3, (int[]) {row_size, row_size,  row_size}, 0);
        
        if (mu_button(gui_context, "File"))
            m_MenuBarState = (m_MenuBarState == MenuBar_File) ? MenuBar_None : MenuBar_File;
        
        if (mu_button(gui_context, "Edit"))
            m_MenuBarState = (m_MenuBarState == MenuBar_Edit) ? MenuBar_None : MenuBar_Edit;

        if (mu_button(gui_context, "View"))
            m_MenuBarState = (m_MenuBarState == MenuBar_View) ? MenuBar_None : MenuBar_View;

        if (m_MenuBarState == MenuBar_File)
        {
            if (mu_begin_window_ex(gui_context, "files", mu_rect(0, text_height, row_size + padding, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_button(gui_context, "New"))
                {
                    
                }
                if (mu_button(gui_context, "Load"))
                {
                    
                }
                if (mu_button(gui_context, "Save"))
                {
                    
                }
                if (mu_button(gui_context, "Export"))
                {
                    
                }
                mu_end_window(gui_context);
            }
        }

        mu_end_window(gui_context);
    }
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
