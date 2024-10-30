#pragma once

#include "system/Singleton.h"

struct GLFWwindow;
struct GLFWcursor;

//----------------------------------------------------------------------------------------------------------------------------
class MouseCursors : public Singleton<MouseCursors>
{
public:
    enum CursorType
    {
        Arrow = 0,
        IBeam = 1,
        CrossHair = 2,
        Hand = 3,
        HResize = 4,
        VResize = 5,
        Count = 6
    };

    void Init(struct GLFWwindow* window);
    void Set(CursorType cursor);
    void Default();
    void Terminate();


private:
    GLFWwindow* m_pWindow;
    GLFWcursor** m_pCursors;
};