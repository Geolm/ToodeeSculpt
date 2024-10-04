#pragma once

#include <assert.h>
#include <stdint.h>

// this class does not allocate memory
// basically it's just an helper to 

template<class T>
class PushArray
{
public:
    PushArray() : m_pData(nullptr), m_NumElements(0), m_MaxElements(0) {}

    void Set(void* buffer, uint32_t length_in_bytes)
    {
        assert(buffer != nullptr);
        m_MaxElements = length_in_bytes / sizeof(T);
        m_NumElements = 0;
        m_pData = (T*) buffer;
    }

    T* NewElement()
    {
        if (m_NumElements < m_MaxElements)
            return &m_pData[m_NumElements++];

        return nullptr;
    }

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

    void RemoveLast()
    {
        if (m_NumElements>0)
            m_NumElements--;
    }

    void Reset() 
    {
        m_NumElements = 0;
    }

    uint32_t GetNumElements() const {return m_NumElements;}
    uint32_t GetMaxElements() const {return m_MaxElements;}

private:
    T* m_pData;
    uint32_t m_NumElements;
    uint32_t m_MaxElements;
};

