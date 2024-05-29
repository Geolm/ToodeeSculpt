#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "Renderer.h"
#include "shader_reader.h"

#define SAFE_RELEASE(p) if (p!=nullptr) p->release();

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

    m_DrawCommandsBuffer.Init(m_pDevice, sizeof(draw_command) * Renderer::MAX_COMMANDS);
    m_DrawDataBuffer.Init(m_pDevice, sizeof(float) * Renderer::MAX_DRAWDATA);
    m_pCountersBuffer = m_pDevice->newBuffer(sizeof(counters), MTL::ResourceStorageModePrivate);
    m_pNodes = m_pDevice->newBuffer(sizeof(tile_node) * MAX_NODES_COUNT, MTL::ResourceStorageModePrivate);
    m_pClearBuffersFence = m_pDevice->newFence();
    m_pBinningFence = m_pDevice->newFence();

    MTL::IndirectCommandBufferDescriptor* pIcbDesc = MTL::IndirectCommandBufferDescriptor::alloc()->init();
    pIcbDesc->setCommandTypes(MTL::IndirectCommandTypeDraw);
    pIcbDesc->setInheritBuffers(false);
    pIcbDesc->setMaxVertexBufferBindCount(2);
    pIcbDesc->setMaxFragmentBufferBindCount(2);
    pIcbDesc->setInheritPipelineState(true);
    m_pIndirectCommandBuffer = m_pDevice->newIndirectCommandBuffer(pIcbDesc, 1, MTL::ResourceStorageModePrivate);
    pIcbDesc->release();

    m_Semaphore = dispatch_semaphore_create(DynamicBuffer::MaxInflightBuffers);

    BuildPSO();
    BuildDepthStencilState();
    Resize(width, height);
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Resize(uint32_t width, uint32_t height)
{
    log_info("resizing the viewport to %dx%d", width, height);
    m_ViewportWidth = width;
    m_ViewportHeight = height;
    m_NumTilesWidth = (width + TILE_SIZE - 1) / TILE_SIZE;
    m_NumTilesHeight = (height + TILE_SIZE - 1) / TILE_SIZE;

    SAFE_RELEASE(m_pHead);
    SAFE_RELEASE(m_pTileIndices);
    m_pHead = m_pDevice->newBuffer(m_NumTilesWidth * m_NumTilesHeight * sizeof(tile_node), MTL::ResourceStorageModePrivate);
    m_pTileIndices = m_pDevice->newBuffer(m_NumTilesWidth * m_NumTilesHeight * sizeof(uint16_t), MTL::ResourceStorageModePrivate);
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::ReloadShaders()
{
    log_info("reloading shaders");
    BuildPSO();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BuildDepthStencilState()
{
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();

    pDsDesc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionAlways);
    pDsDesc->setDepthWriteEnabled(false);

    m_pDepthStencilState = m_pDevice->newDepthStencilState(pDsDesc);

    pDsDesc->release();
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
void Renderer::BuildPSO()
{
    SAFE_RELEASE(m_pBinningPSO);
    SAFE_RELEASE(m_pDrawPSO);

    MTL::Library* pLibrary = BuildShader("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "binning.metal");
    if (pLibrary != nullptr)
    {
        MTL::Function* pBinningFunction = pLibrary->newFunction(NS::String::string("bin", NS::UTF8StringEncoding));
        NS::Error* pError = nullptr;
        m_pBinningPSO = m_pDevice->newComputePipelineState(pBinningFunction, &pError);

        if (m_pBinningPSO == nullptr)
            log_error( "%s", pError->localizedDescription()->utf8String());

        MTL::ArgumentEncoder* inputArgumentEncoder = pBinningFunction->newArgumentEncoder(0);
        MTL::ArgumentEncoder* outputArgumentEncoder = pBinningFunction->newArgumentEncoder(1);
        MTL::ArgumentEncoder* indirectArgumentEncoder = pBinningFunction->newArgumentEncoder(3);

        m_DrawCommandsArg.Init(m_pDevice, inputArgumentEncoder->encodedLength());
        m_BinOutputArg.Init(m_pDevice, outputArgumentEncoder->encodedLength());
        m_pIndirectArg = m_pDevice->newBuffer(indirectArgumentEncoder->encodedLength(), MTL::ResourceStorageModeShared);

        indirectArgumentEncoder->setArgumentBuffer(m_pIndirectArg, 0);
        indirectArgumentEncoder->setIndirectCommandBuffer(m_pIndirectCommandBuffer, 0);

        inputArgumentEncoder->release();
        outputArgumentEncoder->release();
        indirectArgumentEncoder->release();

        pLibrary->release();
        pBinningFunction->release();
    }

    pLibrary = BuildShader("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "rasterizer.metal");
    if (pLibrary != nullptr)
    {
        MTL::Function* pVertexFunction = pLibrary->newFunction(NS::String::string("tile_vs", NS::UTF8StringEncoding));
        MTL::Function* pFragmentFunction = pLibrary->newFunction(NS::String::string("tile_fs", NS::UTF8StringEncoding));
        NS::Error* pError = nullptr;

        MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
        pDesc->setVertexFunction(pVertexFunction);
        pDesc->setFragmentFunction(pFragmentFunction);
        pDesc->setSupportIndirectCommandBuffers(true);

        MTL::RenderPipelineColorAttachmentDescriptor *pRenderbufferAttachment = pDesc->colorAttachments()->object(0);
        pRenderbufferAttachment->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
        pRenderbufferAttachment->setBlendingEnabled(true);
        pRenderbufferAttachment->setSourceRGBBlendFactor(MTL::BlendFactorOne);
        pRenderbufferAttachment->setDestinationRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        pRenderbufferAttachment->setRgbBlendOperation(MTL::BlendOperationAdd);
        pRenderbufferAttachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
        pRenderbufferAttachment->setDestinationAlphaBlendFactor(MTL::BlendFactorZero);
        pRenderbufferAttachment->setAlphaBlendOperation(MTL::BlendOperationAdd);

        m_pDrawPSO = m_pDevice->newRenderPipelineState( pDesc, &pError );

        if (m_pDrawPSO == nullptr)
            log_error( "%s", pError->localizedDescription()->utf8String());

        pVertexFunction->release();
        pFragmentFunction->release();
        pDesc->release();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BeginFrame()
{
    m_FrameIndex++;
    m_ClipsCount = 0;

    m_Commands.Set(m_DrawCommandsBuffer.Map(m_FrameIndex), sizeof(draw_command) * Renderer::MAX_COMMANDS);
    m_DrawData.Set(m_DrawDataBuffer.Map(m_FrameIndex), sizeof(float) * Renderer::MAX_DRAWDATA);
    SetClipRect(0, 0, (uint16_t) m_ViewportWidth, (uint16_t) m_ViewportHeight);
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::EndFrame()
{
    m_DrawCommandsBuffer.Unmap(m_FrameIndex, 0, m_Commands.GetNumElements() * sizeof(draw_command));
    m_DrawDataBuffer.Unmap(m_FrameIndex, 0, m_DrawData.GetNumElements() * sizeof(float));
    m_NumDrawCommands = m_Commands.GetNumElements();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BinCommands()
{
    if (m_pBinningPSO == nullptr)
        return;

    // clear buffers
    MTL::BlitCommandEncoder* pBlitEncoder = m_pCommandBuffer->blitCommandEncoder();
    pBlitEncoder->fillBuffer(m_pCountersBuffer, NS::Range(0, m_pCountersBuffer->length()), 0);
    pBlitEncoder->fillBuffer(m_pHead, NS::Range(0, m_pHead->length()), 0xff);
    pBlitEncoder->resetCommandsInBuffer(m_pIndirectCommandBuffer, NS::Range(0, 1));
    pBlitEncoder->updateFence(m_pClearBuffersFence);
    pBlitEncoder->endEncoding();

    // run binning shader
    MTL::ComputeCommandEncoder* pComputeEncoder = m_pCommandBuffer->computeCommandEncoder();
    pComputeEncoder->waitForFence(m_pClearBuffersFence);
    pComputeEncoder->setComputePipelineState(m_pBinningPSO);

    draw_cmd_arguments* args = (draw_cmd_arguments*) m_DrawCommandsArg.Map(m_FrameIndex);
    args->aa_width = m_AAWidth;
    args->commands = (draw_command*) m_DrawCommandsBuffer.GetBuffer(m_FrameIndex)->gpuAddress();
    args->draw_data = (float*) m_DrawDataBuffer.GetBuffer(m_FrameIndex)->gpuAddress();
    memcpy(args->clips, m_Clips, sizeof(m_Clips));
    args->max_nodes = MAX_NODES_COUNT;
    args->num_commands = m_NumDrawCommands;
    args->num_tile_height = m_NumTilesHeight;
    args->num_tile_width = m_NumTilesWidth;
    args->tile_size = TILE_SIZE;
    args->screen_div = (float2) {.x = 1.f / (float)m_ViewportWidth, .y = 1.f / (float) m_ViewportHeight};
    m_DrawCommandsArg.Unmap(m_FrameIndex, 0, sizeof(draw_cmd_arguments));

    tiles_data* output = (tiles_data*) m_BinOutputArg.Map(m_FrameIndex);
    output->head = (tile_node*) m_pHead->gpuAddress();
    output->nodes = (tile_node*) m_pNodes->gpuAddress();
    output->tile_indices = (uint16_t*) m_pTileIndices->gpuAddress();
    m_BinOutputArg.Unmap(m_FrameIndex, 0, sizeof(tiles_data));

    pComputeEncoder->setBuffer(m_DrawCommandsArg.GetBuffer(m_FrameIndex), 0, 0);
    pComputeEncoder->setBuffer(m_BinOutputArg.GetBuffer(m_FrameIndex), 0, 1);
    pComputeEncoder->setBuffer(m_pCountersBuffer, 0, 2);

    pComputeEncoder->setBuffer(m_pIndirectArg, 0, 3);
    pComputeEncoder->useResource(m_DrawCommandsBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(m_DrawDataBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(m_pHead, MTL::ResourceUsageRead|MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(m_pNodes, MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(m_pTileIndices, MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(m_pIndirectCommandBuffer, MTL::ResourceUsageWrite);
    
    MTL::Size gridSize = MTL::Size(m_NumTilesWidth, m_NumTilesHeight, 1);
    MTL::Size threadgroupSize(m_pBinningPSO->maxTotalThreadsPerThreadgroup(), 1, 1);

    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);
    pComputeEncoder->updateFence(m_pBinningFence);
    pComputeEncoder->endEncoding();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Flush(CA::MetalDrawable* pDrawable)
{
    m_pCommandBuffer = m_pCommandQueue->commandBuffer();

    dispatch_semaphore_wait(m_Semaphore, DISPATCH_TIME_FOREVER);

    BinCommands();

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(pDrawable->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* pRenderEncoder = m_pCommandBuffer->renderCommandEncoder(renderPassDescriptor);

    if (m_pDrawPSO != nullptr && m_pBinningPSO != nullptr)
    {
        pRenderEncoder->waitForFence(m_pBinningFence, MTL::RenderStageVertex|MTL::RenderStageFragment|MTL::RenderStageMesh|MTL::RenderStageObject);
        pRenderEncoder->setCullMode(MTL::CullModeNone);
        pRenderEncoder->setDepthStencilState(m_pDepthStencilState);
        pRenderEncoder->useResource(m_DrawCommandsArg.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_DrawCommandsBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_DrawDataBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pHead, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pNodes, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pTileIndices, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pIndirectCommandBuffer, MTL::ResourceUsageRead);
        pRenderEncoder->setRenderPipelineState(m_pDrawPSO);
        pRenderEncoder->executeCommandsInBuffer(m_pIndirectCommandBuffer, NS::Range(0, 1));
        pRenderEncoder->endEncoding();
    }

    Renderer* pRenderer = this;
    m_pCommandBuffer->addCompletedHandler(^void( MTL::CommandBuffer* pCmd )
    {
        dispatch_semaphore_signal( pRenderer->m_Semaphore );
    });

    m_pCommandBuffer->presentDrawable(pDrawable);
    m_pCommandBuffer->commit();
    m_pCommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Terminate()
{
    m_DrawCommandsBuffer.Terminate();
    m_DrawDataBuffer.Terminate();
    m_DrawCommandsArg.Terminate();
    m_BinOutputArg.Terminate();
    SAFE_RELEASE(m_pDepthStencilState);
    SAFE_RELEASE(m_pCountersBuffer);
    SAFE_RELEASE(m_pClearBuffersFence);
    SAFE_RELEASE(m_pBinningFence);
    SAFE_RELEASE(m_pBinningPSO);
    SAFE_RELEASE(m_pDrawPSO);
    SAFE_RELEASE(m_pHead);
    SAFE_RELEASE(m_pNodes);
    SAFE_RELEASE(m_pTileIndices);
    SAFE_RELEASE(m_pIndirectArg);
    SAFE_RELEASE(m_pIndirectCommandBuffer);
    SAFE_RELEASE(m_pCommandQueue);
}

