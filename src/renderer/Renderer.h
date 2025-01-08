#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"
#include "../shaders/common.h"
#include "../system/PushArray.h"
#include "../system/log.h"
#include "../system/aabb.h"
#include "font9x16.h"

struct mu_Context;

//----------------------------------------------------------------------------------------------------------------------------
class Renderer
{
public:
    void Init(MTL::Device* device, uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);
    void ReloadShaders();
    void BeginFrame();
    void Flush(CA::MetalDrawable* drawable);
    void DebugInterface(struct mu_Context* gui_context);
    void EndFrame();
    void Terminate();

    inline void SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y);
    inline void ResetCanvas() {m_CanvasScale = 1.f;}
    inline void SetCulllingDebug(bool b) {m_CullingDebug = b;}
    void SetCanvas(float width, float height);

    void BeginCombination(float smooth_value);
    void EndCombination();

    inline void DrawCircle(vec2 center, float radius, float width, draw_color color, sdf_operator op = op_union);
    inline void DrawCircleFilled(vec2 center, float radius, draw_color color, sdf_operator op = op_union);
    inline void DrawOrientedBox(vec2 p0, vec2 p1, float width, float thickness, draw_color color, sdf_operator op = op_union);
    inline void DrawOrientedBoxFilled(vec2 p0, vec2 p1, float width, float roundness, draw_color color, sdf_operator op = op_union);
    inline void DrawEllipse(vec2 p0, vec2 p1, float width, float thickness, draw_color color, sdf_operator op = op_union);
    inline void DrawEllipseFilled(vec2 p0, vec2 p1, float width, draw_color color, sdf_operator op = op_union);
    inline void DrawTriangle(vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, sdf_operator op = op_union);
    inline void DrawTriangleFilled(vec2 p0, vec2 p1, vec2 p2, float roundness, draw_color color, sdf_operator op = op_union);
    inline void DrawPie(vec2 center, vec2 point, float aperture, float width, draw_color color, sdf_operator op = op_union);
    inline void DrawPieFilled(vec2 center, vec2 point, float aperture, draw_color color, sdf_operator op = op_union);
    inline void DrawRingFilled(vec2 center, vec2 direction, float aperture, float radius, float thickness, draw_color color, sdf_operator op = op_union);
    inline void DrawUnevenCapsuleFilled(vec2 p0, vec2 p1, float radius0, float radius1, draw_color color, sdf_operator op = op_union);
    void DrawRing(vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, sdf_operator op = op_union);
    void DrawRingFilled(vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, sdf_operator op = op_union);
    
    void DrawBox(float x0, float y0, float x1, float y1, draw_color color);
    void DrawBox(aabb box, draw_color color) {DrawBox(box.min.x, box.min.y, box.max.x, box.max.y, color);}
    void DrawChar(float x, float y, char c, draw_color color);
    void DrawText(float x, float y, const char* text, draw_color color);
    inline void DrawText(vec2 coord, const char* text, draw_color color) {DrawText(coord.x, coord.y, text, color);}

private:
    void PrivateDrawDisc(vec2 center, float radius, float thickness, draw_color color, sdf_operator op);
    void PrivateDrawOrientedBox(vec2 p0, vec2 p1, float width, float roundness, float thickness, draw_color color, sdf_operator op);
    void PrivateDrawEllipse(vec2 p0, vec2 p1, float width, float thickness, draw_color color, sdf_operator op);
    void PrivateDrawTriangle(vec2 p0, vec2 p1, vec2 p2, float roundness, float thickness, draw_color color, sdf_operator op);
    void PrivateDrawPie(vec2 center, vec2 point, float aperture, float thickness, draw_color color, sdf_operator op);
    void PrivateDrawRing(vec2 center, vec2 direction, float aperture, float radius, float thickness, draw_color color, sdf_operator op, bool filled);
    void PrivateDrawUnevenCapsule(vec2 p0, vec2 p1, float radius0, float radius1, float thickness, draw_color color, sdf_operator op);
    void BuildDepthStencilState();
    void BuildPSO();
    void BinCommands();

#ifdef SHADERS_IN_EXECUTABLE
    MTL::Library* BuildShader(const char* shader_buffer, const char* name);
#else
    MTL::Library* BuildShader(const char* path, const char* name);
#endif

private:
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO {nullptr};
    MTL::ComputePipelineState* m_pWriteIcbPSO {nullptr};
    MTL::RenderPipelineState* m_pDrawPSO {nullptr};
    MTL::DepthStencilState* m_pDepthStencilState {nullptr};
    
    DynamicBuffer m_DrawCommandsBuffer;
    DynamicBuffer m_CommandsAABBBuffer;
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
    PushArray<quantized_aabb> m_CommandsAABB;
    
    uint32_t m_FrameIndex {0};
    uint32_t m_ClipsCount {0};
    clip_rect m_Clips[MAX_CLIPS];
    uint32_t m_ViewportWidth;
    uint32_t m_ViewportHeight;
    uint32_t m_NumTilesWidth;
    uint32_t m_NumTilesHeight;
    uint32_t m_NumDrawCommands;
    float m_AAWidth {1.41421356237f};
    float m_FontScale {1.f};
    float m_CanvasScale {1.f};
    float m_SmoothValue {0.f};
    bool m_CullingDebug {false};
    quantized_aabb* m_CombinationAABB {nullptr};

    // stats
    uint32_t m_PeakNumDrawCommands {0};
};

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::SetClipRect(uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y)
{
    // avoid redundant clip rect
    if (m_ClipsCount>0)
    {
        uint32_t index = m_ClipsCount-1;
        if (m_Clips[index].min_x == min_x && m_Clips[index].min_y == min_y &&
            m_Clips[index].max_x == max_x && m_Clips[index].max_y == max_y)
            return;
    }

    if (m_ClipsCount < MAX_CLIPS)
        m_Clips[m_ClipsCount++] = (clip_rect) {.min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y};
    else
        log_error("too many clip rectangle! maximum is %d", MAX_CLIPS);
}

//----------------------------------------------------------------------------------------------------------------------------
inline void Renderer::DrawOrientedBox(vec2 p0, vec2 p1, float width, float thickness, draw_color color, sdf_operator op)
{
    PrivateDrawOrientedBox(p0, p1, width, 0.f, thickness, color, op);
}

inline void Renderer::DrawOrientedBoxFilled(vec2 p0, vec2 p1, float width, float roundness, draw_color color, sdf_operator op)
{
    PrivateDrawOrientedBox(p0, p1, width, roundness, -1.f, color, op);
}

inline void Renderer::DrawCircle(vec2 center, float radius, float thickness, draw_color color, sdf_operator op)
{
    PrivateDrawDisc(center, radius, thickness, color, op);
}

inline void Renderer::DrawCircleFilled(vec2 center, float radius, draw_color color, sdf_operator op)
{
    PrivateDrawDisc(center, radius, -1.f, color, op);
}

inline void Renderer::DrawEllipse(vec2 p0, vec2 p1, float width, float thickness, draw_color color, sdf_operator op)
{
    PrivateDrawEllipse(p0, p1, width, thickness, color, op);
}

inline void Renderer::DrawEllipseFilled(vec2 p0, vec2 p1, float width, draw_color color, sdf_operator op)
{
    PrivateDrawEllipse(p0, p1, width, -1.f, color, op);
}

inline void Renderer::DrawTriangle(vec2 p0, vec2 p1, vec2 p2, float thickness, draw_color color, sdf_operator op)
{
    PrivateDrawTriangle(p0, p1, p2, 0.f, thickness, color, op);
}

inline void Renderer::DrawTriangleFilled(vec2 p0, vec2 p1, vec2 p2, float roundness, draw_color color, sdf_operator op)
{
    PrivateDrawTriangle(p0, p1, p2, roundness, -1.f, color, op);
}

inline void Renderer::DrawPie(vec2 center, vec2 point, float aperture, float width, draw_color color, sdf_operator op)
{
    PrivateDrawPie(center, point, aperture, width, color, op);
}

inline void Renderer::DrawPieFilled(vec2 center, vec2 point, float aperture, draw_color color, sdf_operator op)
{
    PrivateDrawPie(center, point, aperture, -1.f, color, op);
}

inline void Renderer::DrawRingFilled(vec2 center, vec2 direction, float aperture, float radius, float thickness, draw_color color, sdf_operator op)
{
    PrivateDrawRing(center, direction, aperture, radius, thickness, color, op, true);
}

inline void Renderer::DrawUnevenCapsuleFilled(vec2 p0, vec2 p1, float radius0, float radius1, draw_color color, sdf_operator op)
{
    PrivateDrawUnevenCapsule(p0, p1, radius0, radius1, -1.f, color, op);
}
