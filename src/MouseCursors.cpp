#include "MouseCursors.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <assert.h>

//----------------------------------------------------------------------------------------------------------------------------
void MouseCursors::Init(struct GLFWwindow* window)
{
    m_pWindow = window;

    m_pCursors = (GLFWcursor**) malloc(sizeof(void*) * CursorType::Count);
    m_pCursors[CursorType::Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_pCursors[CursorType::IBeam] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    m_pCursors[CursorType::CrossHair] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    m_pCursors[CursorType::Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    m_pCursors[CursorType::HResize] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    m_pCursors[CursorType::VResize] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
}

//----------------------------------------------------------------------------------------------------------------------------
void MouseCursors::Set(CursorType cursor)
{
    uint32_t index = (uint32_t) cursor;
    assert(index < CursorType::Count);
    glfwSetCursor(m_pWindow, m_pCursors[index]);
}

//----------------------------------------------------------------------------------------------------------------------------
void MouseCursors::Default()
{
    glfwSetCursor(m_pWindow, NULL);
}

//----------------------------------------------------------------------------------------------------------------------------
void MouseCursors::Terminate()
{
    for(uint32_t i=0; i<CursorType::Count; ++i)
        glfwDestroyCursor(m_pCursors[i]);

    free(m_pCursors);
}
