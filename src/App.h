#pragma once

struct GLFWwindow;
struct mu_Context;
class Editor;
struct renderer;

#include "system/vec2.h"

class App
{
public:
    void Init(MTL::Device* device, GLFWwindow* window);
    void Update(CA::MetalDrawable* drawable);
    void Terminate();

    void OnKeyEvent(int key, int scancode, int action, int mods);
    void OnWindowResize(int width, int height);
    void OnMouseMove(float x, float y);
    void OnMouseButton(int button, int action, int mods);

private:
    void InitGui();
    void RetrieveFolderPath();
    void DrawGui();

private:
    MTL::Device* m_Device {nullptr};
    renderer* m_pRenderer;
    GLFWwindow* m_Window;
    uint32_t m_WindowHeight;
    uint32_t m_WindowWidth;
    float m_ScaleX, m_ScaleY;
    mu_Context* m_pGuiContext {nullptr};
    vec2 m_MousePos;
    size_t m_LogSize;
    uint64_t m_StartTime;
    uint64_t m_LastTime;
    float m_Time;
    float m_DeltaTime;
    char* m_pFolderPath {nullptr};

    Editor* m_pEditor;
};
