#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "Renderer.h"
#include "shader_reader.h"
#include "../system/microui.h"
#include "../system/aabb.h"

#define SAFE_RELEASE(p) if (p!=nullptr) p->release();
#define TEXT_BUFFER_SIZE (1024)

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

    m_DrawCommandsBuffer.Init(m_pDevice, sizeof(draw_command) * MAX_COMMANDS);
    m_DrawDataBuffer.Init(m_pDevice, sizeof(float) * MAX_DRAWDATA);
    m_CommandsAABBBuffer.Init(m_pDevice, sizeof(quantized_aabb) * MAX_COMMANDS);
    m_pCountersBuffer = m_pDevice->newBuffer(sizeof(counters), MTL::ResourceStorageModePrivate);
    m_pNodes = m_pDevice->newBuffer(sizeof(tile_node) * MAX_NODES_COUNT, MTL::ResourceStorageModePrivate);
    m_pFont = m_pDevice->newBuffer(font9x16data, sizeof(font9x16data), MTL::ResourceStorageModeShared);
    m_pClearBuffersFence = m_pDevice->newFence();
    m_pWriteIcbFence = m_pDevice->newFence();

    MTL::IndirectCommandBufferDescriptor* pIcbDesc = MTL::IndirectCommandBufferDescriptor::alloc()->init();
    pIcbDesc->setCommandTypes(MTL::IndirectCommandTypeDraw);
    pIcbDesc->setInheritBuffers(true);
    pIcbDesc->setInheritPipelineState(true);
    pIcbDesc->setMaxVertexBufferBindCount(2);
    pIcbDesc->setMaxFragmentBufferBindCount(2);
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
    {
        log_error("error while compiling : %s\n%s", name, pError->localizedDescription()->utf8String());
    }
    
    return pLibrary;
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BuildPSO()
{
    SAFE_RELEASE(m_pBinningPSO);
    SAFE_RELEASE(m_pDrawPSO);
    SAFE_RELEASE(m_pWriteIcbPSO);

    MTL::Library* pLibrary = BuildShader("/Users/geolm/Code/Geolm/GPU2DComposer/src/shaders/", "binning.metal");
    if (pLibrary != nullptr)
    {
        MTL::Function* pBinningFunction = pLibrary->newFunction(NS::String::string("bin", NS::UTF8StringEncoding));
        NS::Error* pError = nullptr;
        m_pBinningPSO = m_pDevice->newComputePipelineState(pBinningFunction, &pError);

        if (m_pBinningPSO == nullptr)
        {
            log_error( "%s", pError->localizedDescription()->utf8String());
            return;
        }

        MTL::ArgumentEncoder* inputArgumentEncoder = pBinningFunction->newArgumentEncoder(0);
        MTL::ArgumentEncoder* outputArgumentEncoder = pBinningFunction->newArgumentEncoder(1);

        m_DrawCommandsArg.Init(m_pDevice, inputArgumentEncoder->encodedLength());
        m_BinOutputArg.Init(m_pDevice, outputArgumentEncoder->encodedLength());

        inputArgumentEncoder->release();
        outputArgumentEncoder->release();
        pBinningFunction->release();

        MTL::Function* pWriteIcbFunction = pLibrary->newFunction(NS::String::string("write_icb", NS::UTF8StringEncoding));
        m_pWriteIcbPSO = m_pDevice->newComputePipelineState(pWriteIcbFunction, &pError);
        if (m_pWriteIcbPSO == nullptr)
        {
            log_error( "%s", pError->localizedDescription()->utf8String());
            return;
        }

        MTL::ArgumentEncoder* indirectArgumentEncoder = pWriteIcbFunction->newArgumentEncoder(1);
        m_pIndirectArg = m_pDevice->newBuffer(indirectArgumentEncoder->encodedLength(), MTL::ResourceStorageModeShared);
        indirectArgumentEncoder->setArgumentBuffer(m_pIndirectArg, 0);
        indirectArgumentEncoder->setIndirectCommandBuffer(m_pIndirectCommandBuffer, 0);

        indirectArgumentEncoder->release();
        pWriteIcbFunction->release();
        pLibrary->release();
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
    assert(m_CombinationAABB == nullptr);
    m_FrameIndex++;
    m_ClipsCount = 0;

    m_Commands.Set(m_DrawCommandsBuffer.Map(m_FrameIndex), sizeof(draw_command) * MAX_COMMANDS);
    m_CommandsAABB.Set(m_CommandsAABBBuffer.Map(m_FrameIndex), sizeof(quantized_aabb) * MAX_COMMANDS);
    m_DrawData.Set(m_DrawDataBuffer.Map(m_FrameIndex), sizeof(float) * MAX_DRAWDATA);
    SetClipRect(0, 0, (uint16_t) m_ViewportWidth, (uint16_t) m_ViewportHeight);
    ResetCanvas();
    m_CombinationAABB = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::EndFrame()
{
    assert(m_CombinationAABB == nullptr);
    m_DrawCommandsBuffer.Unmap(m_FrameIndex, 0, m_Commands.GetNumElements() * sizeof(draw_command));
    m_CommandsAABBBuffer.Unmap(m_FrameIndex, 0, m_CommandsAABB.GetNumElements() * sizeof(quantized_aabb));
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
    args->commands_aabb = (quantized_aabb*) m_CommandsAABBBuffer.GetBuffer(m_FrameIndex)->gpuAddress();
    args->commands = (draw_command*) m_DrawCommandsBuffer.GetBuffer(m_FrameIndex)->gpuAddress();
    args->draw_data = (float*) m_DrawDataBuffer.GetBuffer(m_FrameIndex)->gpuAddress();
    args->font = (uint16_t*) m_pFont->gpuAddress();
    memcpy(args->clips, m_Clips, sizeof(m_Clips));
    args->max_nodes = MAX_NODES_COUNT;
    args->num_commands = m_NumDrawCommands;
    args->num_tile_height = m_NumTilesHeight;
    args->num_tile_width = m_NumTilesWidth;
    args->screen_div = (float2) {.x = 1.f / (float)m_ViewportWidth, .y = 1.f / (float) m_ViewportHeight};
    args->font_scale = m_FontScale;
    args->font_size = (float2) {FONT_WIDTH, FONT_HEIGHT};
    m_DrawCommandsArg.Unmap(m_FrameIndex, 0, sizeof(draw_cmd_arguments));

    tiles_data* output = (tiles_data*) m_BinOutputArg.Map(m_FrameIndex);
    output->head = (tile_node*) m_pHead->gpuAddress();
    output->nodes = (tile_node*) m_pNodes->gpuAddress();
    output->tile_indices = (uint16_t*) m_pTileIndices->gpuAddress();
    m_BinOutputArg.Unmap(m_FrameIndex, 0, sizeof(tiles_data));

    pComputeEncoder->setBuffer(m_DrawCommandsArg.GetBuffer(m_FrameIndex), 0, 0);
    pComputeEncoder->setBuffer(m_BinOutputArg.GetBuffer(m_FrameIndex), 0, 1);
    pComputeEncoder->setBuffer(m_pCountersBuffer, 0, 2);

    pComputeEncoder->useResource(m_CommandsAABBBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(m_DrawCommandsBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(m_DrawDataBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(m_pHead, MTL::ResourceUsageRead|MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(m_pNodes, MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(m_pTileIndices, MTL::ResourceUsageWrite);

    MTL::Size gridSize = MTL::Size(m_NumTilesWidth, m_NumTilesHeight, 1);

    NS::UInteger w = m_pBinningPSO->threadExecutionWidth();
    NS::UInteger h = m_pBinningPSO->maxTotalThreadsPerThreadgroup() / w;
    MTL::Size threadgroupSize(w, h, 1);
    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);

    pComputeEncoder->setComputePipelineState(m_pWriteIcbPSO);
    pComputeEncoder->setBuffer(m_pCountersBuffer, 0, 0);
    pComputeEncoder->setBuffer(m_pIndirectArg, 0, 1);
    pComputeEncoder->useResource(m_pIndirectCommandBuffer, MTL::ResourceUsageWrite);
    pComputeEncoder->dispatchThreads(MTL::Size(1, 1, 1), MTL::Size(1, 1, 1));
    pComputeEncoder->updateFence(m_pWriteIcbFence);
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
        pRenderEncoder->waitForFence(m_pWriteIcbFence, MTL::RenderStageVertex|MTL::RenderStageFragment|MTL::RenderStageMesh|MTL::RenderStageObject);
        pRenderEncoder->setCullMode(MTL::CullModeNone);
        pRenderEncoder->setDepthStencilState(m_pDepthStencilState);
        pRenderEncoder->setVertexBuffer(m_DrawCommandsArg.GetBuffer(m_FrameIndex), 0, 0);
        pRenderEncoder->setVertexBuffer(m_pTileIndices, 0, 1);
        pRenderEncoder->setFragmentBuffer(m_DrawCommandsArg.GetBuffer(m_FrameIndex), 0, 0);
        pRenderEncoder->setFragmentBuffer(m_BinOutputArg.GetBuffer(m_FrameIndex), 0, 1);
        pRenderEncoder->useResource(m_DrawCommandsArg.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_DrawCommandsBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_DrawDataBuffer.GetBuffer(m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pHead, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pNodes, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pTileIndices, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pIndirectCommandBuffer, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(m_pFont, MTL::ResourceUsageRead);
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
void Renderer::UserInterface(struct mu_Context* gui_context)
{
    char buffer[TEXT_BUFFER_SIZE];

    if (mu_begin_window_ex(gui_context, "Renderer Debug View", mu_rect(1550, 0, 300, 600), MU_OPT_NOCLOSE))
    {
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_text(gui_context, "frame count");
        snprintf(buffer, TEXT_BUFFER_SIZE, "%6d", m_FrameIndex);
        mu_text(gui_context, buffer);
        mu_text(gui_context, "draw cmd count");
        snprintf(buffer, TEXT_BUFFER_SIZE, "%6d/%d", m_Commands.GetNumElements(), m_Commands.GetMaxElements());
        mu_text(gui_context, buffer);
        mu_text(gui_context, "draw data buffer");
        snprintf(buffer, TEXT_BUFFER_SIZE, "%6d/%d", m_DrawData.GetNumElements(), m_DrawData.GetMaxElements());
        mu_text(gui_context, buffer);
        mu_text(gui_context, "aa width");
        mu_slider(gui_context, &m_AAWidth, 0.f, 4.f);

        mu_end_window(gui_context);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::Terminate()
{
    m_DrawCommandsBuffer.Terminate();
    m_DrawDataBuffer.Terminate();
    m_CommandsAABBBuffer.Terminate();
    m_DrawCommandsArg.Terminate();
    m_BinOutputArg.Terminate();
    SAFE_RELEASE(m_pFont);
    SAFE_RELEASE(m_pDepthStencilState);
    SAFE_RELEASE(m_pCountersBuffer);
    SAFE_RELEASE(m_pClearBuffersFence);
    SAFE_RELEASE(m_pWriteIcbFence);
    SAFE_RELEASE(m_pBinningPSO);
    SAFE_RELEASE(m_pDrawPSO);
    SAFE_RELEASE(m_pWriteIcbPSO);
    SAFE_RELEASE(m_pHead);
    SAFE_RELEASE(m_pNodes);
    SAFE_RELEASE(m_pTileIndices);
    SAFE_RELEASE(m_pIndirectArg);
    SAFE_RELEASE(m_pIndirectCommandBuffer);
    SAFE_RELEASE(m_pCommandQueue);
}

//----------------------------------------------------------------------------------------------------------------------------
void write_float(float* buffer, float value) {*buffer = value;};
template<typename...Args>
void write_float(float* buffer, float value, Args ... args)
{
    *buffer = value;
    write_float(++buffer, args...);
}

template<typename T>
void canvas_to_screen(float scale, T& var){var *= scale;}

template<typename T, typename... Args>
void canvas_to_screen(float scale, T& var, Args & ... args)
{
    var *= scale;
    canvas_to_screen(scale, args...);
}

template<class T> void swap(T& a, T& b) {T tmp = a; a = b; b = tmp;}
template<class T> T min(T a, T b) {return (a<b) ? a : b;}
template<class T> T max(T a, T b) {return (a>b) ? a : b;}

//----------------------------------------------------------------------------------------------------------------------------
static inline void write_aabb(quantized_aabb* box, float min_x, float min_y, float max_x, float max_y)
{
    min_x = max(min_x, 0.f);
    min_y = max(min_y, 0.f);
    max_x = max(max_x, 0.f);
    max_y = max(max_y, 0.f);

    box->min_x = uint8_t(uint32_t(min_x) / TILE_SIZE);
    box->min_y = uint8_t(uint32_t(min_y) / TILE_SIZE);
    box->max_x = uint8_t(uint32_t(max_x) / TILE_SIZE);
    box->max_y = uint8_t(uint32_t(max_y) / TILE_SIZE);
}

//----------------------------------------------------------------------------------------------------------------------------
static inline void merge_aabb(quantized_aabb* merge, const quantized_aabb* other)
{
    if (merge != nullptr)
    {
        merge->min_x = min(merge->min_x, other->min_x);
        merge->min_y = min(merge->min_y, other->min_y);
        merge->max_x = max(merge->max_x, other->max_x);
        merge->max_y = max(merge->max_y, other->max_y);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
static inline quantized_aabb invalid_aabb()
{
    return (quantized_aabb)
    {
        .min_x = UINT8_MAX,
        .min_y = UINT8_MAX,
        .max_x = 0,
        .max_y = 0
    };
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::SetCanvas(float width, float height)
{
    assert(width >= FLT_EPSILON && height >= FLT_EPSILON);
    m_CanvasScale = max((float) m_ViewportWidth / width, (float) m_ViewportHeight / height);
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::BeginCombination(float smooth_value)
{
    assert(m_CombinationAABB == nullptr);
    assert(smooth_value >= 0.f);

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->type = combination_begin;
        cmd->data_index = m_DrawData.GetNumElements();

        float* k = m_DrawData.NewElement(); // just one float for the smooth
        m_CombinationAABB = m_CommandsAABB.NewElement();
        
        if (m_CombinationAABB != nullptr && k != nullptr)
        {
            smooth_value *= m_CanvasScale;
            *k = smooth_value;
            m_SmoothValue = smooth_value;

            // reserve a aabb that we're going to update depending on the coming shapes
            *m_CombinationAABB = invalid_aabb();
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::EndCombination()
{
    assert(m_CombinationAABB != nullptr);

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->type = combination_end;
        cmd->data_index = m_DrawData.GetNumElements();;

        // we put also the smooth value as we traverse the list in reverse order on the gpu
        float* k = m_DrawData.NewElement(); 

        quantized_aabb* aabb = m_CommandsAABB.NewElement();
        if (aabb != nullptr && k != nullptr)
        {
            *aabb = *m_CombinationAABB;
            *k = m_SmoothValue;
            m_CombinationAABB = nullptr;
            m_SmoothValue = 0.f;
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawCircle(float x, float y, float radius, float width, draw_color color, sdf_operator op)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = shape_circle;

        float* data = m_DrawData.NewMultiple(4);
        quantized_aabb* aabb = m_CommandsAABB.NewElement();
        if (data != nullptr && aabb != nullptr)
        {
            canvas_to_screen(m_CanvasScale, x, y, radius, width);
            float max_radius = radius + width * .5f + m_AAWidth + m_SmoothValue;
            write_float(data,  x, y, radius, width * .5f);
            write_aabb(aabb, x - max_radius, y - max_radius, x + max_radius, y + max_radius);
            merge_aabb(m_CombinationAABB, aabb);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawCircleFilled(float x, float y, float radius, draw_color color, sdf_operator op)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = shape_circle_filled;

        float* data = m_DrawData.NewMultiple(3);
        quantized_aabb* aabb = m_CommandsAABB.NewElement();
        if (data != nullptr && aabb != nullptr)
        {
            canvas_to_screen(m_CanvasScale, x, y, radius);
            float max_radius = radius + m_AAWidth + m_SmoothValue;
            write_float(data,  x, y, radius);
            write_aabb(aabb, x - max_radius, y - max_radius, x + max_radius, y + max_radius);
            merge_aabb(m_CombinationAABB, aabb);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawOrientedBox(float x0, float y0, float x1, float y1, float width, float roundness, draw_color color, sdf_operator op)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = shape_oriented_box;

        float* data = m_DrawData.NewMultiple(6);
        quantized_aabb* aabox = m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            canvas_to_screen(m_CanvasScale, x0, y0, x1, y1, width, roundness);
            aabb bb = aabb_from_rounded_obb((vec2){x0, y0}, (vec2){x1, y1}, width, roundness + m_AAWidth + m_SmoothValue);
            write_float(data,  x0, y0, x1, y1, width, roundness);
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(m_CombinationAABB, aabox);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawBox(float x0, float y0, float x1, float y1, draw_color color)
{
    if (x0>x1) swap(x0, x1);
    if (y0>y1) swap(y0, y1);

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_aabox;

        float* data = m_DrawData.NewMultiple(4);
        quantized_aabb* aabox = m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            canvas_to_screen(m_CanvasScale, x0, y0, x1, y1);
            write_float(data, x0, y0, x1, y1);
            write_aabb(aabox, x0, y0, x1, y1);
            merge_aabb(m_CombinationAABB, aabox);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawChar(float x, float y, char c, draw_color color)
{
    if (c < FONT_CHAR_FIRST || c > FONT_CHAR_LAST)
        return;

    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op_none;
        cmd->type = shape_char;
        cmd->custom_data = (uint8_t) (c - FONT_CHAR_FIRST);

        float* data = m_DrawData.NewMultiple(2);
        quantized_aabb* aabox = m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            write_float(data, x, y);
            write_aabb(aabox, x, y, x + FONT_WIDTH, y + FONT_HEIGHT);
            merge_aabb(m_CombinationAABB, aabox);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawTriangleFilled(vec2 p0, vec2 p1, vec2 p2, float roundness, draw_color color, sdf_operator op)
{
    draw_command* cmd = m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = shape_triangle_filled;

        float* data = m_DrawData.NewMultiple(7);
        quantized_aabb* aabox = m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            canvas_to_screen(m_CanvasScale, p0, p1, p2);
            roundness *= m_CanvasScale;

            aabb bb = aabb_from_triangle(p0, p1, p2);
            aabb_grow(&bb, vec2_splat(roundness + m_AAWidth + m_SmoothValue));
            write_float(data,  p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, roundness);
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(m_CombinationAABB, aabox);
            return;
        }
        m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void Renderer::DrawText(float x, float y, const char* text, draw_color color)
{
    canvas_to_screen(m_CanvasScale, x, y);
    const float font_spacing = (FONT_WIDTH + FONT_SPACING) * m_FontScale;
    for(const char *c = text; *c != 0; c++)
    {
        DrawChar(x, y, *c, color);
        x += font_spacing;
    }
}

