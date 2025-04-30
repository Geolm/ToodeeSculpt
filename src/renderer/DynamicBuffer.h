#pragma once

#include "Metal.hpp"
#include <assert.h>

// ---------------------------------------------------------------------------------------------------------------------------
template<typename T>
class DynamicBuffer
{
public:
    enum {MaxInflightBuffers = 3};

private:
    uint32_t GetIndex(uint32_t currentFrameIndex) {return currentFrameIndex % DynamicBuffer::MaxInflightBuffers;}

    MTL::Buffer* m_Buffers[MaxInflightBuffers];
    T* m_pData {nullptr};
    size_t m_NumElements {0};
    size_t m_MaxElements {0};


public:
    // ---------------------------------------------------------------------------------------------------------------------------
    DynamicBuffer()
    {
        for(uint32_t i=0; i<DynamicBuffer::MaxInflightBuffers; ++i)
            m_Buffers[i] = nullptr;
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    void Init(MTL::Device* device, NS::UInteger length)
    {
        for(uint32_t i=0; i<DynamicBuffer::MaxInflightBuffers; ++i)
            m_Buffers[i] = device->newBuffer(length, MTL::ResourceStorageModeShared);
    
        m_pData = nullptr;
        m_NumElements = 0;
        m_MaxElements = length / sizeof(T);
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    T* Map(uint32_t currentFrameIndex)
    {
        m_pData = (T*)m_Buffers[GetIndex(currentFrameIndex)]->contents();
        m_NumElements = 0;
        return m_pData;
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    T* NewElement()
    {
        if (m_NumElements < m_MaxElements)
            return &m_pData[m_NumElements++];

        return nullptr;
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    T* NewMultiple(uint32_t count)
    {
        T* output = nullptr;
        if (m_NumElements + count < m_MaxElements)
        {
            output = &m_pData[m_NumElements];
            m_NumElements += count;
        }
        return output;
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    void RemoveLast()
    {
        if (m_NumElements>0)
            m_NumElements--;
    }

    // ---------------------------------------------------------------------------------------------------------------------------
    void Terminate()
    {
        for(uint32_t i=0; i<DynamicBuffer::MaxInflightBuffers; ++i)
        {
            if (m_Buffers[i] != nullptr)
            {
                m_Buffers[i]->release();
                m_Buffers[i] = nullptr;
            }
        }
    }

    size_t GetNumElements() const {return m_NumElements;}
    size_t GetMaxElements() const {return m_MaxElements;}
    MTL::Buffer* GetBuffer(uint32_t currentFrameIndex) {return m_Buffers[GetIndex(currentFrameIndex)];}
    NS::UInteger GetLength() const {return m_Buffers[0]->length();}
};
