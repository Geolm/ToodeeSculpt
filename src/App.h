#pragma once

#include "renderer/Renderer.h"

struct GLFWwindow;
struct mu_Context;

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
    void LogUserInterface();
    void DrawGui();

private:
    MTL::Device* m_Device {nullptr};
    Renderer m_Renderer;
    GLFWwindow* m_Window;
    uint32_t m_ViewportHeight;
    uint32_t m_ViewportWidth;
    mu_Context* m_pGuiContext {nullptr};
    float m_MouseX, m_MouseY;
    char* m_pLogBuffer;
    size_t m_LogSize;
    uint64_t m_StartTime;
    float m_Time;
};
