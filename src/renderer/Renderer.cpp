#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "Renderer.h"
#include "../system/log.h"
#include "shader_reader.h"

#define SAFE_RELEASE(p) if (p!=nullptr) p->release();

//----------------------------------------------------------------------------------------------------------------------------
Renderer::Renderer() :
    m_pDevice(nullptr),
    m_pBinningPSO(nullptr),
    m_pCommandBuffer(nullptr),
    m_pCommandQueue(nullptr),
    m_pHead(nullptr),
    m_FrameIndex(0)
{

}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Init(MTL::Device* device, uint32_t width, uint32_t height)
{
    assert(device!=nullptr);
    m_pDevice = device;
    m_pCommandQueue = device->newCommandQueue();

    if (m_pCommandQueue == nullptr)
    {
        log_fatal("can't create a command queue");
        exit(EXIT_FAILURE);
    }

    BuildComputePSO();
    Resize(width, height);

    m_CommandsBuffer.Init(m_pDevice, sizeof(draw_command) * Renderer::MAX_COMMANDS);
    m_DrawDataBuffer.Init(m_pDevice, sizeof(float) * Renderer::MAX_DRAWDATA);
    m_pCountersBuffer = m_pDevice->newBuffer(sizeof(counters), MTL::ResourceStorageModePrivate);
    m_pClearBuffersFence = m_pDevice->newFence();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Resize(uint32_t width, uint32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;
    m_NumTilesWidth = (width + TILE_SIZE - 1) / TILE_SIZE;
    m_NumTilesHeight = (height + TILE_SIZE - 1) / TILE_SIZE;

    SAFE_RELEASE(m_pHead);
    m_pHead = m_pDevice->newBuffer(m_NumTilesWidth * m_NumTilesHeight * sizeof(tile_node), MTL::ResourceStorageModePrivate);
}

//----------------------------------------------------------------------------------------------------------------------------
MTL::Library* Renderer::BuildShader(const char* path, const char* name)
{
    char* shader_buffer = read_shader_include(path, name);
    if (shader_buffer == NULL)
    {
        log_fatal("can't find shader %s%s", path, name);
        return nullptr;
    }

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = m_pDevice->newLibrary( NS::String::string(shader_buffer, NS::UTF8StringEncoding), nullptr, &pError );

    free(shader_buffer);

    if (pLibrary == nullptr)
        log_error("%s", pError->localizedDescription()->utf8String());
    
    return pLibrary;
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BuildComputePSO()
{
    SAFE_RELEASE(m_pBinningPSO);

    MTL::Library* pComputeLibrary = BuildShader("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "binning.metal");
    if (pComputeLibrary != nullptr)
    {
        MTL::Function* pBinningFunction = pComputeLibrary->newFunction(NS::String::string("bin", NS::UTF8StringEncoding));

        NS::Error* pError = nullptr;
        m_pBinningPSO = m_pDevice->newComputePipelineState(pBinningFunction, &pError);

        if (m_pBinningPSO == nullptr)
            log_error( "%s", pError->localizedDescription()->utf8String());

        pComputeLibrary->release();
        pBinningFunction->release();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BeginFrame()
{
    m_FrameIndex++;
    m_Commands.Set(m_CommandsBuffer.Map(m_FrameIndex), sizeof(draw_command) * Renderer::MAX_COMMANDS);
    m_DrawData.Set(m_DrawDataBuffer.Map(m_FrameIndex), sizeof(float) * Renderer::MAX_DRAWDATA);
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::EndFrame()
{
    m_CommandsBuffer.Unmap(m_FrameIndex, 0, m_Commands.GetNumElements() * sizeof(draw_command));
    m_DrawDataBuffer.Unmap(m_FrameIndex, 0, m_DrawData.GetNumElements() * sizeof(float));
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BinCommands()
{
    // clear buffers
    MTL::BlitCommandEncoder* pBlitEncoder = m_pCommandBuffer->blitCommandEncoder();
    pBlitEncoder->fillBuffer(m_pCountersBuffer, NS::Range(0, m_pCountersBuffer->length()), 0);
    pBlitEncoder->fillBuffer(m_pHead, NS::Range(0, m_pHead->length()), 0xff);
    pBlitEncoder->updateFence(m_pClearBuffersFence);
    pBlitEncoder->endEncoding();

    // run binning shader
    /*
    MTL::ComputeCommandEncoder* pComputeEncoder = m_pCommandBuffer->computeCommandEncoder();
    pComputeEncoder->waitForFence(m_pClearBuffersFence);
    pComputeEncoder->setComputePipelineState(m_pBinningPSO);

    
    MTL::Size gridSize = MTL::Size(m_NumTilesWidth, m_NumTilesHeight, 1);
    MTL::Size threadgroupSize(m_pBinningPSO->maxTotalThreadsPerThreadgroup(), 1, 1);

    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);
    pComputeEncoder->endEncoding();*/
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Flush(CA::MetalDrawable* pDrawable)
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

    BinCommands();

    m_pCommandBuffer->presentDrawable(pDrawable);
    m_pCommandBuffer->commit();
    m_pCommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Terminate()
{
    m_CommandsBuffer.Terminate();
    m_DrawDataBuffer.Terminate();
    SAFE_RELEASE(m_pCountersBuffer);
    SAFE_RELEASE(m_pClearBuffersFence);
    SAFE_RELEASE(m_pBinningPSO);
    SAFE_RELEASE(m_pHead);
    SAFE_RELEASE(m_pCommandQueue);
}

