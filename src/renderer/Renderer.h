#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"
#include "../shaders/common.h"
#include "../system/PushArray.h"

//----------------------------------------------------------------------------------------------------------------------------
class Renderer
{
public:
    Renderer();
    void Init(MTL::Device* device);
    void BeginFrame();
    void Flush(CA::MetalDrawable* drawable);
    void EndFrame();
    void Terminate();

    // Shapes
    inline void DrawCircle(float x, float y, float radius, float width, uint32_t color);
    void DrawBox(float x, float y, float radius, uint32_t color);
    void DrawRectangle(float x_min, float y_min, float x_max, float y_max, float width, uint32_t color);

private:
    void BuildComputePSO();
    MTL::Library* BuildShader(const char* path, const char* name);

private:
    enum {MAX_COMMANDS = 2<<10};
    enum {MAX_DRAWDATA = Renderer::MAX_COMMANDS * 4};

private:
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO;

    DynamicBuffer m_CommandsBuffer;
    DynamicBuffer m_DrawDataBuffer;

    PushArray<draw_command> m_Commands;
    PushArray<float> m_DrawData;

    uint32_t m_FrameIndex;
    uint32_t m_CurrentClipIndex;
};

inline static void write_float(float* buffer, float a, float b) {buffer[0] = a; buffer[1] = b;}
inline static void write_float(float* buffer, float a, float b, float c) {write_float(buffer, a, b); buffer[2] = c;}
inline static void write_float(float* buffer, float a, float b, float c, float d) {write_float(buffer, a, b, c); buffer[3] = d;}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawCircle(float x, float y, float radius, float width, uint32_t color)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_CurrentClipIndex;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->modifier = modifier_outline;
        cmd->op = op_none;
        cmd->type = sdf_disc;

        float* data = m_DrawData.NewMultiple(4);
        if (data != nullptr)
            write_float(data,  x, y, radius, width);
        else
            m_Commands.RemoveLast();
    }
}

