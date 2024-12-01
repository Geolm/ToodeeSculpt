#include "../renderer/Renderer.h"
#include "../system/undo.h"
#include "../system/microui.h"
#include "../system/format.h"
#include "../system/palettes.h"
#include "../system/nfd.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Editor.h"

#define UNUSED_VARIABLE(a) (void)(a)

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
    m_PopupOpen = false;
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnKeyEvent(int key, int scancode, int action, int mods)
{
    UNUSED_VARIABLE(scancode);
    if (key == GLFW_KEY_Z && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        Undo();
    
    if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        Delete();

    if (key == GLFW_KEY_C && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        Copy();

    if (key == GLFW_KEY_V && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        Paste();
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
void Editor::OnMouseButton(int button, int action, int mods)
{
    if (!m_PopupOpen)
        m_PrimitivesStack.OnMouseButton(button, action, mods);
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

        if (!m_PopupOpen)
        {
            int res;
            
            res = mu_button(gui_context, "File");
            if (res&MU_RES_SUBMIT)
                m_MenuBarState = (m_MenuBarState == MenuBar_File) ? MenuBar_None : MenuBar_File;
            else if (res&MU_RES_HOVER)
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_File : MenuBar_None;
            
            res = mu_button(gui_context, "Edit");
            if (res&MU_RES_SUBMIT)
                m_MenuBarState = (m_MenuBarState == MenuBar_Edit) ? MenuBar_None : MenuBar_Edit;
            else if (res&MU_RES_HOVER)
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_Edit : MenuBar_None;

            res = mu_button(gui_context, "Options");
            if (res&MU_RES_SUBMIT)
                m_MenuBarState = (m_MenuBarState == MenuBar_Options) ? MenuBar_None : MenuBar_Options;
            else if (res&MU_RES_HOVER)
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_Options : MenuBar_None;
        }

        if (m_MenuBarState == MenuBar_File)
        {
            if (mu_begin_window_ex(gui_context, "files", 
                mu_rect(0, text_height + padding, row_size + padding, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_button(gui_context, "New")&MU_RES_SUBMIT)
                {
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Load")&MU_RES_SUBMIT)
                {
                    Load();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Save")&MU_RES_SUBMIT)
                {
                    Save();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button(gui_context, "Export")&MU_RES_SUBMIT)
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
                if (mu_button_ex(gui_context, "Undo ~Z", 0, 0)&MU_RES_SUBMIT)
                {
                    Undo();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Delete", 0, 0)&MU_RES_SUBMIT)
                {
                    Delete();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Copy ~C", 0, 0)&MU_RES_SUBMIT)
                {
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Paste ~V", 0, 0)&MU_RES_SUBMIT)
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

    if (m_PopupOpen)
    {
        mu_Rect rect = mu_rect(m_PopupCoord.x, m_PopupCoord.y, m_PopupHalfSize.x * 2.f, m_PopupHalfSize.y * 2.f);
        if (mu_begin_window_ex(gui_context, m_PopupTitle, rect, MU_OPT_HOLDFOCUS|MU_OPT_NOCLOSE|MU_OPT_NOINTERACT|MU_OPT_NOSCROLL|MU_OPT_FORCE_SIZE|MU_OPT_ALIGNCENTER))
        {
            mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
            mu_text(gui_context, m_PopupMessage);
            if (mu_button(gui_context, "ok"))
                m_PopupOpen = false;

            mu_end_window(gui_context);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Popup(const char* title, const char* message)
{
    strncpy(m_PopupTitle, title, POPUP_STRING_LENGTH);
    strncpy(m_PopupMessage, message, POPUP_STRING_LENGTH);
    log_error("%s : %s", title, message);
    m_PopupOpen = true;
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
                Popup("save failure", "the filename might be illegal or you don't have the write access for this folder");
        }
        else
            log_error("buffer too small, cannot save shape");

        free(buffer);
        free(save_path);
    }
    else if (result == NFD_ERROR)
        Popup("save failure", NFD_GetError());
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Load()
{
    nfdchar_t *load_path = NULL;
    nfdresult_t result = NFD_OpenDialog( "tds", NULL, &load_path);

    if (result == NFD_OKAY)
    {
        FILE* f = fopen(load_path, "rb");
        if (f != NULL)
        {
            fseek(f, 0L, SEEK_END);
            size_t file_length = ftell(f);
            fseek(f, 0L, SEEK_SET);

            // don't read too big file, probably not a TDS file
            if (file_length<TDS_FILE_MAXSIZE)
            {
                void* buffer = malloc(file_length);
                fread(buffer, file_length, 1, f);

                serializer_context serializer;
                serializer_init(&serializer, buffer, file_length);

                if (serializer_read_uint32_t(&serializer) == TDS_FOURCC)
                {
                    // check only the major for compatibility
                    if (serializer_read_uint16_t(&serializer) == TDS_MAJOR)
                    {
                        // discard minor version
                        serializer_read_uint16_t(&serializer);
                        m_PrimitivesStack.Deserialize(&serializer);

                        if (serializer_get_status(&serializer) != serializer_no_error)
                            Popup("load failure", "unable to load primitives");
                        else
                            m_PrimitivesStack.UndoSnapshot();
                    }
                    else
                        Popup("load failure", "file too old and not compatible");
                }
                else
                    Popup("load failure", "not a ToodeeSculpt file");

                free(buffer);
            }
            else
                Popup("load failure", "the file is too big to be loaded");

            fclose(f);
        }
        free(load_path);
    }
    else if (result == NFD_ERROR)
        Popup("load failure", NFD_GetError());
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
void Editor::Copy()
{
    m_PrimitivesStack.CopySelected();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Paste()
{
    m_PrimitivesStack.Paste();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Terminate()
{
    undo_terminate(m_pUndoContext);
    m_PrimitivesStack.Terminate();
}
