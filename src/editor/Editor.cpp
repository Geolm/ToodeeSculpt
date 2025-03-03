#include "../renderer/renderer.h"
#include "../system/undo.h"
#include "../system/microui.h"
#include "../system/format.h"
#include "../system/palettes.h"
#include "../system/nfd.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "Editor.h"
#include "tds.h"

#define UNUSED_VARIABLE(a) (void)(a)

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Init(struct GLFWwindow* window, aabb zone, const char* folder_path)
{
    m_ExternalZone = m_Zone = zone;
    aabb_grow(&m_ExternalZone, vec2_splat(4.f));
    m_PopupHalfSize = (vec2) {250.f, 50.f};
    m_PopupCoord = vec2_sub(aabb_get_center(&m_Zone), m_PopupHalfSize);
    m_pUndoContext = undo_init(1<<20, 1<<10);
    m_PrimitiveEditor.Init(window, zone, m_pUndoContext);
    m_MenuBarState = MenuBar_None;
    m_SnapToGrid = 0;
    m_ShowGrid = 0;
    m_GridSubdivision = 20.f;
    m_pFolderPath = folder_path;
    m_PopupOpen = false;
    m_CullingDebug = false;
    m_AABBDebug = false;
    m_LogLevel = 0;
    m_LogLevelCombo = 0;
    m_WindowDebugOpen = 0;
    m_Window = window;
    m_pActiveEditor = &m_PrimitiveEditor;
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

    if (key == GLFW_KEY_G && action == GLFW_PRESS && mods&GLFW_MOD_SUPER)
        m_ShowGrid = !m_ShowGrid;
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseMove(vec2 pos)
{
    if (!m_PopupOpen)
    {
        m_pActiveEditor->OnMouseMove(pos);
        if (aabb_test_point(&m_ExternalZone, pos))
            m_MenuBarState = MenuBar_None;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::OnMouseButton(int button, int action, int mods)
{
    if (!m_PopupOpen)
        m_pActiveEditor->OnMouseButton(button, action, mods);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Draw(struct renderer* context)
{
    if (!m_CullingDebug)
    {
        renderer_draw_aabb(context, m_ExternalZone, draw_color(0, 0, 0, 0xff));
        renderer_draw_aabb(context, m_Zone, draw_color(0xffffffff));
        renderer_set_culling_debug(context, false);
    }
    else
    renderer_set_culling_debug(context, true);

    if (m_ShowGrid)
    {
        vec2 step = vec2_scale(m_Zone.max - m_Zone.min, 1.f / m_GridSubdivision);
        
        for(float x = m_Zone.min.x; x<m_Zone.max.x; x += step.x)
            renderer_draw_box(context, x, m_Zone.min.y, x+1, m_Zone.max.y, draw_color(na16_light_grey, 128));

        for(float y = m_Zone.min.y; y<m_Zone.max.y; y += step.y)
            renderer_draw_box(context, m_Zone.min.x, y, m_Zone.max.x, y+1, draw_color(na16_light_grey, 128));
    }

    m_pActiveEditor->Draw(context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::DebugInterface(struct mu_Context* gui_context)
{
    if (mu_header_ex(gui_context, "Undo", MU_OPT_EXPANDED))
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
    int window_options = MU_OPT_NOCLOSE|MU_OPT_NORESIZE;
    if (mu_begin_window_ex(gui_context, "Global control", mu_rect(10, 100, 400, 200), window_options))
    {
        if (m_pActiveEditor != nullptr)
            m_pActiveEditor->GlobalControl(gui_context);

        mu_end_window(gui_context);
    }

    if (mu_begin_window_ex(gui_context, "Property grid", mu_rect(10, 400, 400, 600), window_options))
    {
        if (m_pActiveEditor != nullptr)
            m_pActiveEditor->PropertyGrid(gui_context);

        mu_end_window(gui_context);
    }

    if (mu_begin_window_ex(gui_context, "Toolbar", mu_rect(1500, 50, 225, 400), window_options))
    {
        if (m_pActiveEditor != nullptr)
            m_pActiveEditor->Toolbar(gui_context);

        mu_end_window(gui_context);
    }

    if (m_pActiveEditor != nullptr)
        m_pActiveEditor->UserInterface(gui_context);

    MenuBar(gui_context);
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::MenuBar(struct mu_Context* gui_context)
{
    int text_height = gui_context->style->title_height;
    int padding = gui_context->style->padding;
    int row_size = gui_context->text_width(NULL, NULL, 10);
    int window_options = MU_OPT_NOSCROLL|MU_OPT_FORCE_SIZE|MU_OPT_NOINTERACT|MU_OPT_NOTITLE;

    if (mu_begin_window_ex(gui_context, "toodeesculpt menubar", mu_rect(0, 0, row_size*3 + 15, text_height + padding), window_options))
    {
        mu_layout_row(gui_context, 3, (int[]) {row_size, row_size,  row_size}, 0);

        if (!m_PopupOpen)
        {
            if (mu_button(gui_context, "File"))
                m_MenuBarState = (m_MenuBarState == MenuBar_File) ? MenuBar_None : MenuBar_File;
            
            if (mu_mouse_over(gui_context, gui_context->last_rect))
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_File : MenuBar_None;

            if (mu_button(gui_context, "Edit"))
                m_MenuBarState = (m_MenuBarState == MenuBar_Edit) ? MenuBar_None : MenuBar_Edit;

            if (mu_mouse_over(gui_context, gui_context->last_rect))
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_Edit : MenuBar_None;

            if (mu_button(gui_context, "Options"))
                m_MenuBarState = (m_MenuBarState == MenuBar_Options) ? MenuBar_None : MenuBar_Options;

            if (mu_mouse_over(gui_context, gui_context->last_rect))
                m_MenuBarState = (m_MenuBarState != MenuBar_None) ? MenuBar_Options : MenuBar_None;
        }

        if (m_MenuBarState == MenuBar_File)
        {
            if (mu_begin_window_ex(gui_context, "files", 
                mu_rect(0, text_height + padding, row_size + padding, text_height * 4 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                if (mu_button_ex(gui_context, "New", 0, 0))
                {
                    New();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Load", 0, 0))
                {
                    Load();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Save", 0, 0))
                {
                    Save();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Export", 0, 0))
                {
                    m_PrimitiveEditor.Export();
                    m_MenuBarState = MenuBar_None;
                    Popup("Export to shadertoy", "You can now paste the shader code in shadertoy");
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
                    Copy();
                    m_MenuBarState = MenuBar_None;
                }
                if (mu_button_ex(gui_context, "Paste ~V", 0, 0))
                {
                    Paste();
                    m_MenuBarState = MenuBar_None;
                }
                mu_end_window(gui_context);
            }
        }

        if (m_MenuBarState == MenuBar_Options)
        {
            if (mu_begin_window_ex(gui_context, "options", 
                mu_rect(row_size * 2 + 10, text_height + padding, 250.f, text_height * 6 + padding), window_options))
            {
                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);

                mu_checkbox(gui_context, "Culling debug", &m_CullingDebug);

                if (mu_checkbox(gui_context, "AABB debug", &m_AABBDebug))
                    m_PrimitiveEditor.SetAABBDebug((bool)m_AABBDebug);


                if (mu_checkbox(gui_context, "Snap to grid", &m_SnapToGrid))
                    m_PrimitiveEditor.SetSnapToGrid((bool)m_SnapToGrid);

                mu_checkbox(gui_context, "Show grid", &m_ShowGrid);
                mu_layout_row(gui_context, 2, (int[]) {100, -1}, 0);

                mu_label(gui_context, "Grid sub");
                if (mu_slider_ex(gui_context, &m_GridSubdivision, 4.f, 50.f, 1.f, "%2.0f", 0)&MU_RES_SUBMIT)
                    m_PrimitiveEditor.SetGridSubdivision(m_GridSubdivision);

                mu_layout_row(gui_context, 1, (int[]) {-1}, 0);
                mu_checkbox(gui_context, "Debug window", &m_WindowDebugOpen);
                
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
            if (mu_button(gui_context, "ok")&MU_RES_SUBMIT)
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
    log_debug("%s : %s", title, message);
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
        size_t length = TDS_FILE_MAXSIZE;
        void* buffer = malloc(length);
        serializer_context serializer;

        serializer_init(&serializer, buffer, length);
        serializer_write_uint32_t(&serializer, TDS_FOURCC);
        serializer_write_uint16_t(&serializer, TDS_MAJOR);
        serializer_write_uint16_t(&serializer, TDS_MINOR);
        m_PrimitiveEditor.Serialize(&serializer, tds_normalizion_support(TDS_MAJOR, TDS_MINOR));

        if (serializer_get_status(&serializer) == serializer_no_error)
        {
            FILE* f = fopen(save_path, "wb");
            if (f != NULL)
            {
                fwrite(serializer.buffer, serializer.position, 1, f);
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
            log_debug("opening file '%s'", load_path);
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
                    uint16_t major = serializer_read_uint16_t(&serializer);

                    // check only the major for compatibility
                    if (major == TDS_MAJOR)
                    {
                        // discard minor version
                        uint16_t minor = serializer_read_uint16_t(&serializer);
                        log_debug("loading file version %d.%03d", major, minor);

                        m_PrimitiveEditor.Deserialize(&serializer, major, minor, tds_normalizion_support(major, minor));

                        if (serializer_get_status(&serializer) != serializer_no_error)
                            Popup("load failure", "unable to load primitives");
                        else
                            m_PrimitiveEditor.UndoSnapshot();
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
    m_pActiveEditor->Undo();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Delete()
{
    m_pActiveEditor->Delete();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::New()
{
    m_PrimitiveEditor.New();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Copy()
{
    m_pActiveEditor->Copy();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Paste()
{
    m_pActiveEditor->Paste();
}

//----------------------------------------------------------------------------------------------------------------------------
void Editor::Terminate()
{
    undo_terminate(m_pUndoContext);
    m_PrimitiveEditor.Terminate();
}
