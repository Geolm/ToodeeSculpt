#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"
#include "../shaders/common.h"
#include "../system/PushArray.h"
#include "../system/log.h"
#include "font9x16.h"

//----------------------------------------------------------------------------------------------------------------------------
class Renderer
{
public:
    void Init(MTL::Device* device, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void ReloadShaders();
    void BeginFrame();
    void Flush(CA::MetalDrawable* drawable);
    void EndFrame();
    void Terminate();

    inline void SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y);
    inline void DrawCircle(float x, float y, float radius, float width, draw_color color);
    inline void DrawCircleFilled(float x, float y, float radius, draw_color color);
    inline void DrawLine(float x0, float y0, float x1, float y1, float width, draw_color color);
    inline void DrawBox(float x0, float y0, float x1, float y1, draw_color color);
    inline void DrawChar(float x, float y, char c, draw_color color);
    inline void DrawText(float x, float y, const char* text, draw_color color);

private:
    void BuildDepthStencilState();
    void BuildPSO();
    void BinCommands();
    MTL::Library* BuildShader(const char* path, const char* name);

private:
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO {nullptr};
    MTL::ComputePipelineState* m_pWriteIcbPSO {nullptr};
    MTL::RenderPipelineState* m_pDrawPSO {nullptr};
    MTL::DepthStencilState* m_pDepthStencilState {nullptr};
    
    DynamicBuffer m_DrawCommandsBuffer;
    DynamicBuffer m_DrawDataBuffer;
    MTL::Buffer* m_pCountersBuffer {nullptr};
    MTL::Fence* m_pClearBuffersFence {nullptr};
    MTL::Fence* m_pWriteIcbFence {nullptr};
    dispatch_semaphore_t m_Semaphore;
    MTL::Buffer* m_pHead {nullptr};
    MTL::Buffer* m_pNodes {nullptr};
    MTL::Buffer* m_pTileIndices {nullptr};
    MTL::Buffer* m_pFont {nullptr};
    MTL::IndirectCommandBuffer* m_pIndirectCommandBuffer {nullptr};
    MTL::Buffer* m_pIndirectArg {nullptr};
    DynamicBuffer m_DrawCommandsArg;
    DynamicBuffer m_BinOutputArg;
    
    PushArray<draw_command> m_Commands;
    PushArray<float> m_DrawData;
    
    uint32_t m_FrameIndex {0};
    uint32_t m_ClipsCount {0};
    clip_rect m_Clips[MAX_CLIPS];
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
    uint32_t m_NumTilesWidth;
    uint32_t m_NumTilesHeight;
    uint32_t m_NumDrawCommands;
    float m_AAWidth {1.f};
    float m_FontScale {1.f};
};

inline static void write_float(float* buffer, float a, float b) {buffer[0] = a; buffer[1] = b;}
inline static void write_float(float* buffer, float a, float b, float c) {write_float(buffer, a, b); buffer[2] = c;}
inline static void write_float(float* buffer, float a, float b, float c, float d) {write_float(buffer, a, b, c); buffer[3] = d;}
inline static void write_float(float* buffer, float a, float b, float c, float d, float e) {write_float(buffer, a, b, c, d); buffer[4] = e;}

template<class T> void swap(T& a, T& b) {T tmp = a; a = b; b = tmp;}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y)
{
    if (m_ClipsCount < MAX_CLIPS)
        m_Clips[m_ClipsCount++] = (clip_rect) {.min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y};
    else
        log_error("too many clip rectangle! maximum is %d", MAX_CLIPS);
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawCircle(float x, float y, float radius, float width, draw_color color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_circle;

        float* data = m_DrawData.NewMultiple(4);
        if (data != nullptr)
            write_float(data,  x, y, radius, width);
        else
            m_Commands.RemoveLast();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawCircleFilled(float x, float y, float radius, draw_color color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_circle_filled;

        float* data = m_DrawData.NewMultiple(3);
        if (data != nullptr)
            write_float(data,  x, y, radius);
        else
            m_Commands.RemoveLast();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawLine(float x0, float y0, float x1, float y1, float width, draw_color color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_line;

        float* data = m_DrawData.NewMultiple(5);
        if (data != nullptr)
            write_float(data,  x0, y0, x1, y1, width);
        else
            m_Commands.RemoveLast();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawBox(float x0, float y0, float x1, float y1, draw_color color)
{
    if (x0>x1) swap(x0, x1);
    if (y0>y1) swap(y0, y1);

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_box;

        float* data = m_DrawData.NewMultiple(4);
        if (data != nullptr)
            write_float(data, x0, y0, x1, y1);
        else
            m_Commands.RemoveLast();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawChar(float x, float y, char c, draw_color color)
{
    if (c < FONT_CHAR_FIRST || c > FONT_CHAR_LAST)
        return;

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_char;
        cmd->custom_data = (uint8_t) (c - FONT_CHAR_FIRST);

        float* data = m_DrawData.NewMultiple(2);
        if (data != nullptr)
            write_float(data, x, y);
        else
            m_Commands.RemoveLast();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawText(float x, float y, const char* text, draw_color color)
{
    const float font_spacing = (FONT_WIDTH + FONT_SPACING) * m_FontScale;
    for(const char *c = text; *c != 0; c++)
    {
        DrawChar(x, y, *c, color);
        x += font_spacing;
    }
}

