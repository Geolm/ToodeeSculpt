#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"


class Renderer
{
public:
    void Init(MTL::Device* device);
    void Draw(CA::MetalDrawable* drawable);
    void Terminate();

private:
    void BuildComputePSO();
    MTL::Library* BuildShader(const char* path, const char* name);

private:
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO;
};

