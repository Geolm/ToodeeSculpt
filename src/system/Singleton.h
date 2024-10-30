#pragma once

#include <assert.h>

template<typename T>
class Singleton
{
public:
    void operator =(const Singleton& other) = delete;

    static T& CreateInstance()
    {
        assert(m_sInstance == nullptr);
        m_sInstance = new T;
        return *m_sInstance;
    }
    
    static T& GetInstance() {assert(m_sInstance != nullptr); return *m_sInstance;}

    static void DeleteInstance()
    {
        assert(m_sInstance != nullptr);
        delete m_sInstance;
    }

private:
    static T* m_sInstance;
};

template<typename T>
T* Singleton<T>::m_sInstance = nullptr;
