#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"
#include "../shaders/common.h"
#include "../system/PushArray.h"
#include "../system/log.h"

//----------------------------------------------------------------------------------------------------------------------------
class Renderer
{
public:
    void Init(MTL::Device* device, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void BeginFrame();
    void Flush(CA::MetalDrawable* drawable);
    void EndFrame();
    void Terminate();

    inline void SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y);
    inline void DrawCircle(float x, float y, float radius, float width, uint32_t color);
    inline void DrawCircleFilled(float x, float y, float radius, uint32_t color);
    
private:
    void BuildComputePSO();
    void BinCommands();
    MTL::Library* BuildShader(const char* path, const char* name);
    
private:
    enum {MAX_COMMANDS = 2<<10};
    enum {MAX_DRAWDATA = Renderer::MAX_COMMANDS * 4};
    
private:
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO;
    
    DynamicBuffer m_DrawCommandsBuffer;
    DynamicBuffer m_DrawDataBuffer;
    MTL::Buffer* m_pCountersBuffer {nullptr};
    MTL::Fence* m_pClearBuffersFence {nullptr};
    MTL::Buffer* m_pHead {nullptr};
    MTL::Buffer* m_pNodes {nullptr};
    MTL::Buffer* m_pTileIndices {nullptr};
    MTL::IndirectCommandBuffer* m_pIndirectCommandBuffer {nullptr};
    MTL::Buffer* m_pIndirectArg {nullptr};
    DynamicBuffer m_BinInputArg;
    DynamicBuffer m_BinOutputArg;
    
    PushArray<draw_command> m_Commands;
    PushArray<float> m_DrawData;
    
    uint32_t m_FrameIndex {0};
    uint32_t m_CurrentClipIndex {0};
    clip_rect m_Clips[MAX_CLIPS];
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
    uint32_t m_NumTilesWidth;
    uint32_t m_NumTilesHeight;
    uint32_t m_NumDrawCommands;
    float m_AAWidth {1.f};
};

inline static void write_float(float* buffer, float a, float b) {buffer[0] = a; buffer[1] = b;}
inline static void write_float(float* buffer, float a, float b, float c) {write_float(buffer, a, b); buffer[2] = c;}
inline static void write_float(float* buffer, float a, float b, float c, float d) {write_float(buffer, a, b, c); buffer[3] = d;}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y)
{
    if (m_CurrentClipIndex < MAX_CLIPS)
    {
        m_CurrentClipIndex++;
        m_Clips[m_CurrentClipIndex] = (clip_rect) {.min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y};
    }
    else
        log_error("too many clip rectangle! maximum is %d", MAX_CLIPS);
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawCircle(float x, float y, float radius, float width, uint32_t color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_CurrentClipIndex;
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
inline void Renderer::DrawCircleFilled(float x, float y, float radius, uint32_t color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = m_CurrentClipIndex;
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
