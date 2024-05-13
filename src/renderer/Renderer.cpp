#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "Renderer.h"
#include "log.h"

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Init(MTL::Device* device)
{
    assert(device!=nullptr);
    m_Device = device;
    //m_Library = device->newDefaultLibrary();
    m_CommandQueue = device->newCommandQueue();

    if (m_CommandQueue == nullptr)
    {
        log_fatal("can't create a command queue");
        exit(EXIT_FAILURE);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Draw(CA::MetalDrawable* drawable)
{
    m_CommandBuffer = m_CommandQueue->commandBuffer();

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(drawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);
    
    MTL::RenderCommandEncoder* renderCommandEncoder = m_CommandBuffer->renderCommandEncoder(renderPassDescriptor);
    renderCommandEncoder->endEncoding();

    m_CommandBuffer->presentDrawable(drawable);
    m_CommandBuffer->commit();
    m_CommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();
}
