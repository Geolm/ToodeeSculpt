#pragma once

#include "../Metal.hpp"
#include <assert.h>

// ---------------------------------------------------------------------------------------------------------------------------
class DynamicBuffer
{
public:
    void Init(MTL::Device* device, NS::UInteger length);
    MTL::Buffer* GetBuffer(uint32_t currentFrameIndex) {return m_Buffers[GetIndex(currentFrameIndex)];}
    NS::UInteger GetLength() const {return m_Buffers[0]->length();}
    void* Map(uint32_t currentFrameIndex);
    void Unmap(uint32_t currentFrameIndex, NS::UInteger location, NS::UInteger length);
    void Terminate();

private:
    uint32_t GetIndex(uint32_t currentFrameIndex) {return currentFrameIndex % DynamicBuffer::MaxInflightBuffers;}

public:
    enum {MaxInflightBuffers = 3};

private:
    MTL::Buffer* m_Buffers[MaxInflightBuffers];
    bool m_SharedMemory;
};

// ---------------------------------------------------------------------------------------------------------------------------
inline void DynamicBuffer::Init(MTL::Device* device, NS::UInteger length)
{
    m_SharedMemory = device->supportsFamily(MTL::GPUFamilyApple7);
    for(uint32_t i=0; i<DynamicBuffer::MaxInflightBuffers; ++i)
    {
        m_Buffers[i] = device->newBuffer(length, m_SharedMemory ? MTL::ResourceStorageModeShared : MTL::ResourceStorageModeManaged);
        assert(m_Buffers[i] != nullptr);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
inline void* DynamicBuffer::Map(uint32_t currentFrameIndex)
{
    return m_Buffers[GetIndex(currentFrameIndex)]->contents();
}

// ---------------------------------------------------------------------------------------------------------------------------
inline void DynamicBuffer::Unmap(uint32_t currentFrameIndex, NS::UInteger location, NS::UInteger length) 
{
    if (!m_SharedMemory)
        m_Buffers[GetIndex(currentFrameIndex)]->didModifyRange(NS::Range(location, length));
}

// ---------------------------------------------------------------------------------------------------------------------------
inline void DynamicBuffer::Terminate()
{
    for(uint32_t i=0; i<DynamicBuffer::MaxInflightBuffers; ++i)
        m_Buffers[i]->release();
}
