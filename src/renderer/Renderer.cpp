#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "Renderer.h"
#include "log.h"
#include "shader_reader.h"

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Init(MTL::Device* device)
{
    assert(device!=nullptr);
    m_pDevice = device;
    //m_Library = device->newDefaultLibrary();
    m_pCommandQueue = device->newCommandQueue();

    if (m_pCommandQueue == nullptr)
    {
        log_fatal("can't create a command queue");
        exit(EXIT_FAILURE);
    }

    BuildComputePSO();
}

//----------------------------------------------------------------------------------------------------------------------------
MTL::Library* Renderer::BuildShader(const char* path, const char* name)
{
    char* shader_buffer = read_shader_include(path, name);
    if (shader_buffer == NULL)
    {
        log_fatal("can't find shader %s%s", path, name);
        exit(EXIT_FAILURE);
    }

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = m_pDevice->newLibrary( NS::String::string(shader_buffer, NS::UTF8StringEncoding), nullptr, &pError );
    if (pLibrary == nullptr)
    {
        log_error("%s", pError->localizedDescription()->utf8String());
        exit(EXIT_FAILURE);
    }

    free(shader_buffer);
    return pLibrary;
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BuildComputePSO()
{
    MTL::Library* pComputeLibrary = BuildShader("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "binning.metal");
    MTL::Function* pBinningFunction = pComputeLibrary->newFunction(NS::String::string("bin", NS::UTF8StringEncoding));

    NS::Error* pError = nullptr;
    m_pBinningPSO = m_pDevice->newComputePipelineState(pBinningFunction, &pError);

    if (m_pBinningPSO == nullptr)
    {
        log_error( "%s", pError->localizedDescription()->utf8String() );
        exit(EXIT_FAILURE);
    }

    pComputeLibrary->release();
    pBinningFunction->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Draw(CA::MetalDrawable* pDrawable)
{
    m_pCommandBuffer = m_pCommandQueue->commandBuffer();

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(pDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);
    
    MTL::RenderCommandEncoder* renderCommandEncoder = m_pCommandBuffer->renderCommandEncoder(renderPassDescriptor);
    renderCommandEncoder->endEncoding();

    m_pCommandBuffer->presentDrawable(pDrawable);
    m_pCommandBuffer->commit();
    m_pCommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Terminate()
{
    m_pBinningPSO->release();
    m_pCommandQueue->release();
}

