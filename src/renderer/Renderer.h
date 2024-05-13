#pragma once

#include "../Metal.hpp"
#include "DynamicBuffer.h"


class Renderer
{
public:
    void Init(MTL::Device* device);
    void Draw(CA::MetalDrawable* drawable);

private:
    MTL::Device* m_Device;
    MTL::CommandQueue* m_CommandQueue;
    MTL::CommandBuffer* m_CommandBuffer;
    MTL::Library* m_Library;
};

