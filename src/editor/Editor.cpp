#include "../renderer/Renderer.h"
#include "../system/undo.h"
#include "../system/microui.h"
#include "../system/format.h"
#include "../system/palettes.h"
#include "../system/nfd.h"
#include "Editor.h"


//----------------------------------------------------------------------------------------------------------------------------
void Editor::Init(aabb zone, const char* folder_path)
{
    m_ExternalZone = m_Zone = zone;
    aabb_grow(&m_ExternalZone, vec2_splat(4.f));
    m_PopupHalfSize = (vec2) {250.f, 50.f};
    m_PopupCoord = vec2_sub(aabb_get_center(&m_Zone), m_PopupHalfSize);
    m_pUndoContext = undo_init(1<<18, 1<<10);
    m_PrimitivesStack.Init(zone, m_pUndoContext);
    m_MenuBarState = MenuBar_None;
    m_SnapToGrid = 0;
    m_ShowGrid = 0;
    m_GridSubdivision = 20.f;
    m_pFolderPath = folder_path;
    m_SaveFailed = false;
    m_PopupOpen = false;
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseMove(vec2 pos)
{
    if (!m_PopupOpen)
    {
        m_PrimitivesStack.OnMouseMove(pos);

        if (aabb_test_point(&m_ExternalZone, pos))
            m_MenuBarState = MenuBar_None;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseButton(int button, int action)
{
    if (!m_PopupOpen)
        m_PrimitivesStack.OnMouseButton(button, action);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Draw(Renderer& renderer)
{
    renderer.DrawBox(m_ExternalZone, draw_color(0, 0, 0, 0xff));
    renderer.DrawBox(m_Zone, draw_color(0xffffffff));

    if (m_ShowGrid)
    {
        vec2 step = vec2_scale(m_Zone.max - m_Zone.min, 1.f / m_GridSubdivision);
        
        for(float x = m_Zone.min.x; x<m_Zone.max.x; x += step.x)
            renderer.DrawBox(x, m_Zone.min.y, x+1, m_Zone.max.y, draw_color(na16_light_grey, 64));
        
        for(float y = m_Zone.min.y; y<m_Zone.max.y; y += step.y)
            renderer.DrawBox(m_Zone.min.x, y, m_Zone.max.x, y+1, draw_color(na16_light_grey, 128));
    }

    renderer.SetClipRect((int)m_Zone.min.x, (int)m_Zone.min.y, (int)m_Zone.max.x, (int)m_Zone.max.y);
    m_PrimitivesStack.Draw(renderer);
    renderer.SetClipRect(0, 0, UINT16_MAX, UINT16_MAX);
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
    m_PrimitivesStack.UserInterface(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::MenuBar(struct mu_Context* gui_context)
{
    int text_height = gui_context->style->title_height;
    int padding = gui_context->style->padding;
    int row_size = gui_context->text_width(NULL, NULL, 10);
    int window_options = MU_OPT_NOSCROLL|MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOTITLE;

    if (mu_begin_window_ex(gui_context, "toodeesculpt menubar", mu_rect(0, 0, row_size*3 + 20, text_height + padding), window_options))
    {
        mu_layout_row(gui_context, 3, (int[]) {row_size, row_size,  row_size}, 0);
        
        if (mu_button(gui_context, "File") && !m_PopupOpen)
            m_MenuBarState = (m_MenuBarState == MenuBar_File) ? MenuBar_None : MenuBar_File;
        
        if (mu_button(gui_context, "Edit") && !m_PopupOpen)
            m_MenuBarState = (m_MenuBarState == MenuBar_Edit) ? MenuBar_None : MenuBar_Edit;

        if (mu_button(gui_context, "Options") && !m_PopupOpen)
            m_MenuBarState = (m_MenuBarState == MenuBar_Options) ? MenuBar_None : MenuBar_Options;

        if (m_MenuBarState == MenuBar_File)
        {
            if (mu_begin_window_ex(gui_context, "files", 
                mu_rect(0, text_height + padding, row_size + padding, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_button(gui_context, "New"))
                {
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Load"))
                {
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Save"))
                {
                    Save();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Export"))
                {
                    m_MenuBarState = MenuBar_None;
                }
                mu_end_window(gui_context);
            }
        }

        if (m_MenuBarState == MenuBar_Edit)
        {
            if (mu_begin_window_ex(gui_context, "edit", 
                mu_rect(row_size + padding, text_height + padding, row_size + padding, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_button_ex(gui_context, "Undo ~Z", 0, 0))
                {
                    Undo();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Delete", 0, 0))
                {
                    Delete();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Copy ~C", 0, 0))
                {
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Paste ~V", 0, 0))
                {
                    m_MenuBarState = MenuBar_None;
                }
                
                mu_end_window(gui_context);
            }
        }

        if (m_MenuBarState == MenuBar_Options)
        {
            if (mu_begin_window_ex(gui_context, "edit", 
                mu_rect(row_size * 2 + 10, text_height + padding, 250.f, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_checkbox(gui_context, "Snap to grid", &m_SnapToGrid))
                    m_PrimitivesStack.SetSnapToGrid((bool)m_SnapToGrid);

                mu_checkbox(gui_context, "Show grid", &m_ShowGrid);
                mu_layout_row(gui_context, 2, (int[]) {100, -1}, 0);
                mu_label(gui_context, "Grid sub");

                if (mu_slider_ex(gui_context, &m_GridSubdivision, 4.f, 50.f, 1.f, "%2.0f", 0)&MU_RES_SUBMIT)
                    m_PrimitivesStack.SetGridSubdivision(m_GridSubdivision);
                
                mu_end_window(gui_context);
            }
        }
        mu_end_window(gui_context);
    }

    if (m_SaveFailed)
    {
        mu_Rect rect = mu_rect(m_PopupCoord.x, m_PopupCoord.y, m_PopupHalfSize.x * 2.f, m_PopupHalfSize.y * 2.f);
        if (mu_begin_window_ex(gui_context, "save failure", rect, MU_OPT_HOLDFOCUS|MU_OPT_NOCLOSE|MU_OPT_NOINTERACT|MU_OPT_NOSCROLL|MU_OPT_FORCE_SIZE|MU_OPT_ALIGNCENTER))
        {
            mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
            mu_text(gui_context, "the filename might be illegal or you don't have the write access for this folder");
            if (mu_button(gui_context, "ok"))
            {
                m_SaveFailed = false;
                m_PopupOpen = false;
            }
            mu_end_window(gui_context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Save()
{
    nfdchar_t *save_path = NULL;
    nfdresult_t result = NFD_SaveDialog( "tds", m_pFolderPath, &save_path );
    if (result == NFD_OKAY)
    {
        // prepare the file in memory
        size_t length = m_PrimitivesStack.GetSerializedDataLength() + sizeof(uint32_t) + 2 * sizeof(uint16_t);
        void* buffer = malloc(length);
        serializer_context serializer;

        serializer_init(&serializer, buffer, length);
        serializer_write_uint32_t(&serializer, TDS_FOURCC);
        serializer_write_uint16_t(&serializer, TDS_MAJOR);
        serializer_write_uint16_t(&serializer, TDS_MINOR);
        m_PrimitivesStack.Serialize(&serializer);

        if (serializer_get_status(&serializer) == serializer_no_error)
        {
            FILE* f = fopen(save_path, "wb");
            if (f != NULL)
            {
                fwrite(serializer.buffer, length, 1, f);
                fclose(f);
            }
            else
            {
                log_error("cannot open/create file : %s", save_path);
                m_PopupOpen = true;
                m_SaveFailed = true;
            }
        }
        else
        {
            log_error("cannot save shape");
        }

        free(buffer);
        free(save_path);
    }
    else if (result == NFD_ERROR)
    {
        log_error("file dialog error: %s", NFD_GetError());
        m_PopupOpen = true;
        m_SaveFailed = true;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Undo()
{
    m_PrimitivesStack.Undo();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Delete()
{
    m_PrimitivesStack.DeleteSelected();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Terminate()
{
    undo_terminate(m_pUndoContext);
    m_PrimitivesStack.Terminate();
}
