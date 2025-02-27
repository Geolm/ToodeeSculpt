#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include "../Metal.hpp"

#include "renderer.h"
#include "shader_reader.h"
#include "../system/microui.h"
#include "../system/format.h"
#include "../system/arc.h"
#include "DynamicBuffer.h"
#include "../system/PushArray.h"
#include "../system/log.h"
#include "../system/ortho.h"
#include "commitmono_21_31.h"

#ifdef SHADERS_IN_EXECUTABLE
#include "../shaders/binning.h"
#include "../shaders/rasterizer.h"
#endif

#define SAFE_RELEASE(p) if (p!=nullptr) p->release();
#define SHADER_PATH "/Users/geolm/Code/Geolm/ToodeeSculpt/src/shaders/"
#define UNUSED_VARIABLE(a) (void)(a)

const float small_float = 0.001f;

template<class T> void swap(T& a, T& b) {T tmp = a; a = b; b = tmp;}
template<class T> T min(T a, T b) {return (a<b) ? a : b;}
template<class T> T max(T a, T b) {return (a>b) ? a : b;}

struct renderer
{
    MTL::Device* m_pDevice;
    MTL::CommandQueue* m_pCommandQueue;
    MTL::CommandBuffer* m_pCommandBuffer;
    MTL::ComputePipelineState* m_pBinningPSO {nullptr};
    MTL::ComputePipelineState* m_pWriteIcbPSO {nullptr};
    MTL::RenderPipelineState* m_pDrawPSO {nullptr};
    MTL::DepthStencilState* m_pDepthStencilState {nullptr};
    
    DynamicBuffer m_DrawCommandsBuffer;
    DynamicBuffer m_CommandsAABBBuffer;
    DynamicBuffer m_DrawDataBuffer;
    MTL::Buffer* m_pCountersBuffer {nullptr};
    MTL::Fence* m_pClearBuffersFence {nullptr};
    MTL::Fence* m_pWriteIcbFence {nullptr};
    dispatch_semaphore_t m_Semaphore;
    MTL::Buffer* m_pHead {nullptr};
    MTL::Buffer* m_pNodes {nullptr};
    MTL::Buffer* m_pTileIndices {nullptr};
    MTL::IndirectCommandBuffer* m_pIndirectCommandBuffer {nullptr};
    MTL::Buffer* m_pIndirectArg {nullptr};
    DynamicBuffer m_DrawCommandsArg;
    DynamicBuffer m_BinOutputArg;
    MTL::Texture *m_pFontTexture {nullptr};
    
    PushArray<draw_command> m_Commands;
    PushArray<float> m_DrawData;
    PushArray<quantized_aabb> m_CommandsAABB;
    
    uint32_t m_FrameIndex {0};
    uint32_t m_ClipsCount {0};
    clip_rect m_Clips[MAX_CLIPS];
    uint32_t m_WindowWidth;
    uint32_t m_WindowHeight;
    uint32_t m_NumTilesWidth;
    uint32_t m_NumTilesHeight;
    uint32_t m_NumDrawCommands;
    float m_AAWidth {1.41421356237f};
    float m_FontScale {1.f};
    float m_SmoothValue {0.f};
    float m_OutlineWidth {1.f};
    bool m_CullingDebug {false};
    struct view_proj m_ViewProj;
    float m_CameraScale {1.f};
    vec2 m_CameraPosition {.x = 0.f, .y = 0.f};
    quantized_aabb* m_CombinationAABB {nullptr};

    // stats
    uint32_t m_PeakNumDrawCommands {0};
};


void renderer_build_pso(struct renderer* r);
void renderer_build_font_texture(struct renderer* r);
void renderer_reload_shaders(struct renderer* r);
void renderer_build_depthstencil_state(struct renderer* r);

//----------------------------------------------------------------------------------------------------------------------------
struct renderer* renderer_init(void* device, uint32_t width, uint32_t height)
{
    renderer* r = new renderer;

    assert(device!=nullptr);
    r->m_pDevice = (MTL::Device*)device;
    r->m_pCommandQueue = r->m_pDevice->newCommandQueue();

    if (r->m_pCommandQueue == nullptr)
    {
        log_fatal("can't create a command queue");
        exit(EXIT_FAILURE);
    }

    r->m_DrawCommandsBuffer.Init(r->m_pDevice, sizeof(draw_command) * MAX_COMMANDS);
    r->m_DrawDataBuffer.Init(r->m_pDevice, sizeof(float) * MAX_DRAWDATA);
    r->m_CommandsAABBBuffer.Init(r->m_pDevice, sizeof(quantized_aabb) * MAX_COMMANDS);
    r->m_pCountersBuffer = r->m_pDevice->newBuffer(sizeof(counters), MTL::ResourceStorageModePrivate);
    r->m_pNodes = r->m_pDevice->newBuffer(sizeof(tile_node) * MAX_NODES_COUNT, MTL::ResourceStorageModePrivate);
    r->m_pClearBuffersFence = r->m_pDevice->newFence();
    r->m_pWriteIcbFence = r->m_pDevice->newFence();

    MTL::IndirectCommandBufferDescriptor* pIcbDesc = MTL::IndirectCommandBufferDescriptor::alloc()->init();
    pIcbDesc->setCommandTypes(MTL::IndirectCommandTypeDraw);
    pIcbDesc->setInheritBuffers(true);
    pIcbDesc->setInheritPipelineState(true);
    pIcbDesc->setMaxVertexBufferBindCount(2);
    pIcbDesc->setMaxFragmentBufferBindCount(2);
    r->m_pIndirectCommandBuffer = r->m_pDevice->newIndirectCommandBuffer(pIcbDesc, 1, MTL::ResourceStorageModePrivate);
    pIcbDesc->release();

    r->m_Semaphore = dispatch_semaphore_create(DynamicBuffer::MaxInflightBuffers);

    renderer_build_pso(r);
    renderer_build_font_texture(r);
    renderer_build_depthstencil_state(r);
    renderer_resize(r, width, height);

    return r;
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_resize(struct renderer* r, uint32_t width, uint32_t height)
{
    log_info("resizing the viewport to %dx%d", width, height);
    r->m_WindowWidth = width;
    r->m_WindowHeight = height;
    r->m_NumTilesWidth = (width + TILE_SIZE - 1) / TILE_SIZE;
    r->m_NumTilesHeight = (height + TILE_SIZE - 1) / TILE_SIZE;

    SAFE_RELEASE(r->m_pHead);
    SAFE_RELEASE(r->m_pTileIndices);
    r->m_pHead = r->m_pDevice->newBuffer(r->m_NumTilesWidth * r->m_NumTilesHeight * sizeof(tile_node), MTL::ResourceStorageModePrivate);
    r->m_pTileIndices = r->m_pDevice->newBuffer(r->m_NumTilesWidth * r->m_NumTilesHeight * sizeof(uint16_t), MTL::ResourceStorageModePrivate);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_reload_shaders(struct renderer* r)
{
#ifndef SHADERS_IN_EXECUTABLE
    log_info("reloading shaders");
    renderer_build_pso(r);
#else
    UNUSED_VARIABLE(r);
#endif
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_build_depthstencil_state(struct renderer* r)
{
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();

    pDsDesc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionAlways);
    pDsDesc->setDepthWriteEnabled(false);

    r->m_pDepthStencilState = r->m_pDevice->newDepthStencilState(pDsDesc);

    pDsDesc->release();
}

//----------------------------------------------------------------------------------------------------------------------------
#ifdef SHADERS_IN_EXECUTABLE
MTL::Library* renderer_build_shader(struct renderer* r, const uint8_t* shader_buffer, const char* name)
{
    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = r->m_pDevice->newLibrary( NS::String::string((const char*)shader_buffer, NS::UTF8StringEncoding), nullptr, &pError );

    if (pLibrary == nullptr)
    {
        log_error("error while compiling : %s\n%s", name, pError->localizedDescription()->utf8String());
    }
    return pLibrary;
}
#else
MTL::Library* renderer_build_shader(struct renderer* r, const char* path, const char* name)
{
    char* shader_buffer = read_shader_include(path, name);
    if (shader_buffer == NULL)
    {
        log_fatal("can't find shader %s%s", path, name);
        return nullptr;
    }

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = r->m_pDevice->newLibrary( NS::String::string(shader_buffer, NS::UTF8StringEncoding), nullptr, &pError );

    free(shader_buffer);

    if (pLibrary == nullptr)
    {
        log_error("error while compiling : %s\n%s", name, pError->localizedDescription()->utf8String());
    }
    return pLibrary;
}

#endif

//----------------------------------------------------------------------------------------------------------------------------
void renderer_build_pso(struct renderer* r)
{
    SAFE_RELEASE(r->m_pBinningPSO);
    SAFE_RELEASE(r->m_pDrawPSO);
    SAFE_RELEASE(r->m_pWriteIcbPSO);

#ifdef SHADERS_IN_EXECUTABLE
    MTL::Library* pLibrary = renderer_build_shader(r, binning_shader, "binning");
#else
    MTL::Library* pLibrary = renderer_build_shader(r, SHADER_PATH, "binning.metal");
#endif
    if (pLibrary != nullptr)
    {
        MTL::Function* pBinningFunction = pLibrary->newFunction(NS::String::string("bin", NS::UTF8StringEncoding));
        NS::Error* pError = nullptr;
        r->m_pBinningPSO = r->m_pDevice->newComputePipelineState(pBinningFunction, &pError);

        if (r->m_pBinningPSO == nullptr)
        {
            log_error( "%s", pError->localizedDescription()->utf8String());
            return;
        }

        MTL::ArgumentEncoder* inputArgumentEncoder = pBinningFunction->newArgumentEncoder(0);
        MTL::ArgumentEncoder* outputArgumentEncoder = pBinningFunction->newArgumentEncoder(1);

        r->m_DrawCommandsArg.Init(r->m_pDevice, inputArgumentEncoder->encodedLength());
        r->m_BinOutputArg.Init(r->m_pDevice, outputArgumentEncoder->encodedLength());

        inputArgumentEncoder->release();
        outputArgumentEncoder->release();
        pBinningFunction->release();

        MTL::Function* pWriteIcbFunction = pLibrary->newFunction(NS::String::string("write_icb", NS::UTF8StringEncoding));
        r->m_pWriteIcbPSO = r->m_pDevice->newComputePipelineState(pWriteIcbFunction, &pError);
        if (r->m_pWriteIcbPSO == nullptr)
        {
            log_error( "%s", pError->localizedDescription()->utf8String());
            return;
        }

        MTL::ArgumentEncoder* indirectArgumentEncoder = pWriteIcbFunction->newArgumentEncoder(1);
        r->m_pIndirectArg = r->m_pDevice->newBuffer(indirectArgumentEncoder->encodedLength(), MTL::ResourceStorageModeShared);
        indirectArgumentEncoder->setArgumentBuffer(r->m_pIndirectArg, 0);
        indirectArgumentEncoder->setIndirectCommandBuffer(r->m_pIndirectCommandBuffer, 0);

        indirectArgumentEncoder->release();
        pWriteIcbFunction->release();
        pLibrary->release();
    }

#ifdef SHADERS_IN_EXECUTABLE
    pLibrary = renderer_build_shader(r, rasterizer_shader, "rasterizer");
#else
    pLibrary = renderer_build_shader(r, SHADER_PATH, "rasterizer.metal");
#endif
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

        r->m_pDrawPSO = r->m_pDevice->newRenderPipelineState( pDesc, &pError );

        if (r->m_pDrawPSO == nullptr)
            log_error( "%s", pError->localizedDescription()->utf8String());

        pVertexFunction->release();
        pFragmentFunction->release();
        pDesc->release();
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_build_font_texture(struct renderer* r)
{
    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth(FONT_TEXTURE_WIDTH);
    pTextureDesc->setHeight(FONT_TEXTURE_HEIGHT);
    pTextureDesc->setPixelFormat(MTL::PixelFormatBC4_RUnorm);
    pTextureDesc->setTextureType(MTL::TextureType2D);
    pTextureDesc->setMipmapLevelCount(1);
    pTextureDesc->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

    r->m_pFontTexture = r->m_pDevice->newTexture(pTextureDesc);
    r->m_pFontTexture->replaceRegion( MTL::Region( 0, 0, 0, FONT_TEXTURE_WIDTH, FONT_TEXTURE_HEIGHT, 1 ), 0, commitmono_21_31, (FONT_TEXTURE_WIDTH/4) * 8);
    pTextureDesc->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_begin_frame(struct renderer* r)
{
    assert(r->m_CombinationAABB == nullptr);
    r->m_FrameIndex++;
    r->m_ClipsCount = 0;

    r->m_Commands.Set(r->m_DrawCommandsBuffer.Map(r->m_FrameIndex), sizeof(draw_command) * MAX_COMMANDS);
    r->m_CommandsAABB.Set(r->m_CommandsAABBBuffer.Map(r->m_FrameIndex), sizeof(quantized_aabb) * MAX_COMMANDS);
    r->m_DrawData.Set(r->m_DrawDataBuffer.Map(r->m_FrameIndex), sizeof(float) * MAX_DRAWDATA);
    renderer_set_cliprect(r, 0, 0, (uint16_t) r->m_WindowWidth, (uint16_t) r->m_WindowHeight);
    renderer_set_viewport(r, (float)r->m_WindowWidth, (float)r->m_WindowHeight);
    renderer_set_camera(r, vec2_zero(), 1.f);
    r->m_CombinationAABB = nullptr;
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_end_frame(struct renderer* r)
{
    assert(r->m_CombinationAABB == nullptr);
    r->m_DrawCommandsBuffer.Unmap(r->m_FrameIndex, 0, r->m_Commands.GetNumElements() * sizeof(draw_command));
    r->m_CommandsAABBBuffer.Unmap(r->m_FrameIndex, 0, r->m_CommandsAABB.GetNumElements() * sizeof(quantized_aabb));
    r->m_DrawDataBuffer.Unmap(r->m_FrameIndex, 0, r->m_DrawData.GetNumElements() * sizeof(float));
    r->m_NumDrawCommands = r->m_Commands.GetNumElements();
    r->m_PeakNumDrawCommands = max(r->m_PeakNumDrawCommands, r->m_NumDrawCommands);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_bin_commands(struct renderer* r)
{
    if (r->m_pBinningPSO == nullptr)
        return;

    // clear buffers
    MTL::BlitCommandEncoder* pBlitEncoder = r->m_pCommandBuffer->blitCommandEncoder();
    pBlitEncoder->fillBuffer(r->m_pCountersBuffer, NS::Range(0, r->m_pCountersBuffer->length()), 0);
    pBlitEncoder->fillBuffer(r->m_pHead, NS::Range(0, r->m_pHead->length()), 0xff);
    pBlitEncoder->resetCommandsInBuffer(r->m_pIndirectCommandBuffer, NS::Range(0, 1));
    pBlitEncoder->updateFence(r->m_pClearBuffersFence);
    pBlitEncoder->endEncoding();

    // run binning shader
    MTL::ComputeCommandEncoder* pComputeEncoder = r->m_pCommandBuffer->computeCommandEncoder();
    pComputeEncoder->waitForFence(r->m_pClearBuffersFence);
    pComputeEncoder->setComputePipelineState(r->m_pBinningPSO);

    draw_cmd_arguments* args = (draw_cmd_arguments*) r->m_DrawCommandsArg.Map(r->m_FrameIndex);
    args->aa_width = r->m_AAWidth;
    args->commands_aabb = (quantized_aabb*) r->m_CommandsAABBBuffer.GetBuffer(r->m_FrameIndex)->gpuAddress();
    args->commands = (draw_command*) r->m_DrawCommandsBuffer.GetBuffer(r->m_FrameIndex)->gpuAddress();
    args->draw_data = (float*) r->m_DrawDataBuffer.GetBuffer(r->m_FrameIndex)->gpuAddress();
    args->font = (texture_half) r->m_pFontTexture->gpuResourceID()._impl;
    memcpy(args->clips, r->m_Clips, sizeof(r->m_Clips));
    args->max_nodes = MAX_NODES_COUNT;
    args->num_commands = r->m_NumDrawCommands;
    args->num_tile_height = r->m_NumTilesHeight;
    args->num_tile_width = r->m_NumTilesWidth;
    args->screen_div = (float2) {.x = 1.f / (float)r->m_WindowWidth, .y = 1.f / (float) r->m_WindowHeight};
    args->font_scale = r->m_FontScale;
    args->outline_width = r->m_OutlineWidth;
    args->outline_color = draw_color(0xff000000);
    args->culling_debug = r->m_CullingDebug;
    r->m_DrawCommandsArg.Unmap(r->m_FrameIndex, 0, sizeof(draw_cmd_arguments));

    tiles_data* output = (tiles_data*) r->m_BinOutputArg.Map(r->m_FrameIndex);
    output->head = (tile_node*) r->m_pHead->gpuAddress();
    output->nodes = (tile_node*) r->m_pNodes->gpuAddress();
    output->tile_indices = (uint16_t*) r->m_pTileIndices->gpuAddress();
    r->m_BinOutputArg.Unmap(r->m_FrameIndex, 0, sizeof(tiles_data));

    pComputeEncoder->setBuffer(r->m_DrawCommandsArg.GetBuffer(r->m_FrameIndex), 0, 0);
    pComputeEncoder->setBuffer(r->m_BinOutputArg.GetBuffer(r->m_FrameIndex), 0, 1);
    pComputeEncoder->setBuffer(r->m_pCountersBuffer, 0, 2);

    pComputeEncoder->useResource(r->m_CommandsAABBBuffer.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(r->m_DrawCommandsBuffer.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(r->m_DrawDataBuffer.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
    pComputeEncoder->useResource(r->m_pHead, MTL::ResourceUsageRead|MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(r->m_pNodes, MTL::ResourceUsageWrite);
    pComputeEncoder->useResource(r->m_pTileIndices, MTL::ResourceUsageWrite);

    MTL::Size gridSize = MTL::Size(r->m_NumTilesWidth, r->m_NumTilesHeight, 1);

    NS::UInteger w = r->m_pBinningPSO->threadExecutionWidth();
    NS::UInteger h = r->m_pBinningPSO->maxTotalThreadsPerThreadgroup() / w;
    MTL::Size threadgroupSize(w, h, 1);
    pComputeEncoder->dispatchThreads(gridSize, threadgroupSize);

    pComputeEncoder->setComputePipelineState(r->m_pWriteIcbPSO);
    pComputeEncoder->setBuffer(r->m_pCountersBuffer, 0, 0);
    pComputeEncoder->setBuffer(r->m_pIndirectArg, 0, 1);
    pComputeEncoder->useResource(r->m_pIndirectCommandBuffer, MTL::ResourceUsageWrite);
    pComputeEncoder->dispatchThreads(MTL::Size(1, 1, 1), MTL::Size(1, 1, 1));
    pComputeEncoder->updateFence(r->m_pWriteIcbFence);
    pComputeEncoder->endEncoding();
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_flush(struct renderer* r, void* drawable)
{
    r->m_pCommandBuffer = r->m_pCommandQueue->commandBuffer();

    dispatch_semaphore_wait(r->m_Semaphore, DISPATCH_TIME_FOREVER);

    renderer_bin_commands(r);

    MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();
    MTL::RenderPassColorAttachmentDescriptor* cd = renderPassDescriptor->colorAttachments()->object(0);
    cd->setTexture(((CA::MetalDrawable*)drawable)->texture());
    cd->setLoadAction(MTL::LoadActionClear);
    cd->setClearColor(MTL::ClearColor(41.0f/255.0f, 42.0f/255.0f, 48.0f/255.0f, 1.0));
    cd->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* pRenderEncoder = r->m_pCommandBuffer->renderCommandEncoder(renderPassDescriptor);

    if (r->m_pDrawPSO != nullptr && r->m_pBinningPSO != nullptr)
    {
        pRenderEncoder->waitForFence(r->m_pWriteIcbFence, MTL::RenderStageVertex|MTL::RenderStageFragment|MTL::RenderStageMesh|MTL::RenderStageObject);
        pRenderEncoder->setCullMode(MTL::CullModeNone);
        pRenderEncoder->setDepthStencilState(r->m_pDepthStencilState);
        pRenderEncoder->setVertexBuffer(r->m_DrawCommandsArg.GetBuffer(r->m_FrameIndex), 0, 0);
        pRenderEncoder->setVertexBuffer(r->m_pTileIndices, 0, 1);
        pRenderEncoder->setFragmentBuffer(r->m_DrawCommandsArg.GetBuffer(r->m_FrameIndex), 0, 0);
        pRenderEncoder->setFragmentBuffer(r->m_BinOutputArg.GetBuffer(r->m_FrameIndex), 0, 1);
        pRenderEncoder->useResource(r->m_DrawCommandsArg.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_DrawCommandsBuffer.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_DrawDataBuffer.GetBuffer(r->m_FrameIndex), MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_pHead, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_pNodes, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_pTileIndices, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_pIndirectCommandBuffer, MTL::ResourceUsageRead);
        pRenderEncoder->useResource(r->m_pFontTexture, MTL::ResourceUsageRead);
        pRenderEncoder->setRenderPipelineState(r->m_pDrawPSO);
        pRenderEncoder->executeCommandsInBuffer(r->m_pIndirectCommandBuffer, NS::Range(0, 1));
        pRenderEncoder->endEncoding();
    }

    r->m_pCommandBuffer->addCompletedHandler(^void( MTL::CommandBuffer* pCmd )
    {
        UNUSED_VARIABLE(pCmd);
        dispatch_semaphore_signal( r->m_Semaphore );
    });

    r->m_pCommandBuffer->presentDrawable((CA::MetalDrawable*)drawable);
    r->m_pCommandBuffer->commit();
    r->m_pCommandBuffer->waitUntilCompleted();
    
    renderPassDescriptor->release();
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_debug_interface(struct renderer* r, struct mu_Context* gui_context)
{
    if (mu_header(gui_context, "Renderer"))
    {
        mu_layout_row(gui_context, 2, (int[]) { 150, -1 }, 0);
        mu_text(gui_context, "frame count");
        mu_text(gui_context, format("%6d", r->m_FrameIndex));
        mu_text(gui_context, "draw cmd count");
        mu_text(gui_context, format("%6d/%d", r->m_Commands.GetNumElements(), r->m_Commands.GetMaxElements()));
        mu_text(gui_context, "peak cmd count");
        mu_text(gui_context, format("%6d", r->m_PeakNumDrawCommands));
        mu_text(gui_context, "draw data buffer");
        mu_text(gui_context, format("%6d/%d", r->m_DrawData.GetNumElements(), r->m_DrawData.GetMaxElements()));
        mu_text(gui_context, "aa width");
        mu_slider(gui_context, &r->m_AAWidth, 0.f, 4.f);
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_terminate(struct renderer* r)
{
    r->m_DrawCommandsBuffer.Terminate();
    r->m_DrawDataBuffer.Terminate();
    r->m_CommandsAABBBuffer.Terminate();
    r->m_DrawCommandsArg.Terminate();
    r->m_BinOutputArg.Terminate();
    SAFE_RELEASE(r->m_pDepthStencilState);
    SAFE_RELEASE(r->m_pCountersBuffer);
    SAFE_RELEASE(r->m_pClearBuffersFence);
    SAFE_RELEASE(r->m_pWriteIcbFence);
    SAFE_RELEASE(r->m_pBinningPSO);
    SAFE_RELEASE(r->m_pDrawPSO);
    SAFE_RELEASE(r->m_pWriteIcbPSO);
    SAFE_RELEASE(r->m_pHead);
    SAFE_RELEASE(r->m_pNodes);
    SAFE_RELEASE(r->m_pTileIndices);
    SAFE_RELEASE(r->m_pIndirectArg);
    SAFE_RELEASE(r->m_pIndirectCommandBuffer);
    SAFE_RELEASE(r->m_pCommandQueue);
    SAFE_RELEASE(r->m_pFontTexture);
    delete r;
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
void distance_screen_space(float scale, T& var){var *= scale;}

template<typename T, typename... Args>
void distance_screen_space(float scale, T& var, Args & ... args)
{
    var *= scale;
    distance_screen_space(scale, args...);
}

//----------------------------------------------------------------------------------------------------------------------------
static inline void write_aabb(quantized_aabb* box, float min_x, float min_y, float max_x, float max_y)
{
    min_x = max(min_x, 0.f);
    min_y = max(min_y, 0.f);
    max_x = max(max_x, 0.f);
    max_y = max(max_y, 0.f);
    box->min_x = uint8_t(min(uint32_t(min_x) / TILE_SIZE, (uint32_t)UINT8_MAX));
    box->min_y = uint8_t(min(uint32_t(min_y) / TILE_SIZE, (uint32_t)UINT8_MAX));
    box->max_x = uint8_t(min(uint32_t(max_x) / TILE_SIZE, (uint32_t)UINT8_MAX));
    box->max_y = uint8_t(min(uint32_t(max_y) / TILE_SIZE, (uint32_t)UINT8_MAX));
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
void renderer_begin_combination(struct renderer* r, float smooth_value)
{
    assert(r->m_CombinationAABB == nullptr);
    assert(smooth_value >= 0.f);

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->type = pack_type(combination_begin, fill_solid);
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;

        float* k = r->m_DrawData.NewElement(); // just one float for the smooth
        r->m_CombinationAABB = r->m_CommandsAABB.NewElement();
        
        if (r->m_CombinationAABB != nullptr && k != nullptr)
        {
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), smooth_value);
            *k = smooth_value;
            r->m_SmoothValue = smooth_value;

            // reserve a aabb that we're going to update depending on the coming shapes
            *r->m_CombinationAABB = invalid_aabb();
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_end_combination(struct renderer* r, bool outline)
{
    assert(r->m_CombinationAABB != nullptr);

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->type = pack_type(combination_end, outline ? fill_outline : fill_solid);
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;

        // we put also the smooth value as we traverse the list in reverse order on the gpu
        float* k = r->m_DrawData.NewElement(); 

        quantized_aabb* aabb = r->m_CommandsAABB.NewElement();
        if (aabb != nullptr && k != nullptr)
        {
            *aabb = *r->m_CombinationAABB;
            *k = r->m_SmoothValue;
            r->m_CombinationAABB = nullptr;
            r->m_SmoothValue = 0.f;
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_disc(struct renderer* r, vec2 center, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    thickness *= .5f;
    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_disc, fillmode);

        float* data = r->m_DrawData.NewMultiple((fillmode == fill_hollow) ? 4 : 3);
        quantized_aabb* aabb = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabb != nullptr)
        {
            center = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, center);
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), radius, thickness);

            float max_radius = radius + r->m_AAWidth + r->m_SmoothValue;

            if (fillmode == fill_hollow)
            {
                max_radius += thickness;
                write_float(data, center.x, center.y, radius, thickness);
            }
            else
                write_float(data, center.x, center.y, radius);

            write_aabb(aabb, center.x - max_radius, center.y - max_radius, center.x + max_radius, center.y + max_radius);
            merge_aabb(r->m_CombinationAABB, aabb);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_orientedbox(struct renderer* r, vec2 p0, vec2 p1, float width, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    if (vec2_similar(p0, p1, small_float))
        return;

    thickness *= .5f;

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_oriented_box, fillmode);

        float* data = r->m_DrawData.NewMultiple(6);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            float roundness_thickness = (fillmode == fill_hollow) ? thickness : roundness;

            p0 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p0);
            p1 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p1);
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), width, roundness_thickness);

            aabb bb = aabb_from_rounded_obb(p0, p1, width, roundness_thickness + r->m_AAWidth + r->m_SmoothValue);
            write_float(data, p0.x, p0.y, p1.x, p1.y, width, roundness_thickness);
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_line(struct renderer* r, vec2 p0, vec2 p1, float width, draw_color color, enum sdf_operator op)
{
    renderer_draw_orientedbox(r, p0, p1, width, 0.f, 0.f, fill_solid, color, op);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_arrow(struct renderer* r, vec2 p0, vec2 p1, float width, float arrow_length_percentage, draw_color color)
{
    vec2 delta = vec2_scale(vec2_sub(p0, p1), arrow_length_percentage);
    vec2 arrow_edge0 = vec2_add(p1, vec2_add(delta, vec2_skew(delta)));
    vec2 arrow_edge1 = vec2_add(p1, vec2_sub(delta, vec2_skew(delta)));

    renderer_draw_line(r, p0, p1, width, color, op_union);
    renderer_draw_line(r, p1, arrow_edge0, width, color, op_union);
    renderer_draw_line(r, p1, arrow_edge1, width, color, op_union);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_arrow_filled(struct renderer* r, vec2 p0, vec2 p1, float width, float arrow_length_percentage, draw_color color)
{
    vec2 delta = vec2_scale(vec2_sub(p0, p1), arrow_length_percentage);
    vec2 arrow_edge0 = vec2_add(p1, vec2_add(delta, vec2_skew(delta)));
    vec2 arrow_edge1 = vec2_add(p1, vec2_sub(delta, vec2_skew(delta)));

    renderer_draw_line(r, p0, p1, width, color, op_union);
    renderer_draw_triangle(r, p1, arrow_edge0, arrow_edge1, 0.f, 0.f, fill_solid, color, op_union);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_double_arrow(struct renderer* r, vec2 p0, vec2 p1, float width, float arrow_length_percentage, draw_color color)
{
    vec2 delta = vec2_scale(vec2_sub(p0, p1), arrow_length_percentage);
    vec2 arrow_edge0 = vec2_add(p1, vec2_add(delta, vec2_skew(delta)));
    vec2 arrow_edge1 = vec2_add(p1, vec2_sub(delta, vec2_skew(delta)));

    renderer_draw_line(r, p0, p1, width, color, op_union);
    renderer_draw_line(r, p1, arrow_edge0, width, color, op_union);
    renderer_draw_line(r, p1, arrow_edge1, width, color, op_union);

    arrow_edge0 = vec2_sub(p0, vec2_add(delta, vec2_skew(delta)));
    arrow_edge1 = vec2_sub(p0, vec2_sub(delta, vec2_skew(delta)));
    renderer_draw_line(r, p0, arrow_edge0, width, color, op_union);
    renderer_draw_line(r, p0, arrow_edge1, width, color, op_union);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_ellipse(struct renderer* r, vec2 p0, vec2 p1, float width, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    if (vec2_similar(p0, p1, small_float))
        return;

    if (width <= small_float)
        renderer_draw_orientedbox(r, p0, p1, 0.f, 0.f, -1.f, fill_solid, color, op);
    else
    {
        thickness = float_max(thickness * .5f, 0.f);
        draw_command* cmd = r->m_Commands.NewElement();
        if (cmd != nullptr)
        {
            cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
            cmd->color = color;
            cmd->data_index = r->m_DrawData.GetNumElements();
            cmd->op = op;
            cmd->type = pack_type(primitive_ellipse, fillmode);

            float* data = r->m_DrawData.NewMultiple((fillmode == fill_hollow) ? 6 : 5);
            quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
            if (data != nullptr && aabox != nullptr)
            {
                p0 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p0);
                p1 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p1);
                distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), width, thickness);

                aabb bb = aabb_from_rounded_obb(p0, p1, width, r->m_AAWidth + r->m_SmoothValue + thickness);
                if (fillmode == fill_hollow)
                    write_float(data, p0.x, p0.y, p1.x, p1.y, width, thickness);
                else
                    write_float(data, p0.x, p0.y, p1.x, p1.y, width);

                write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
                merge_aabb(r->m_CombinationAABB, aabox);
                return;
            }
            r->m_Commands.RemoveLast();
        }
        log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_triangle(struct renderer* r, vec2 p0, vec2 p1, vec2 p2, float roundness, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    // exclude invalid triangle
    if (vec2_similar(p0, p1, small_float) || vec2_similar(p2, p1, small_float) || vec2_similar(p0, p2, small_float))
        return;

    thickness *= .5f;

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_triangle, fillmode);

        float* data = r->m_DrawData.NewMultiple(7);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            p0 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p0);
            p1 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p1);
            p2 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p2);
            
            float roundness_thickness = (fillmode != fill_hollow) ? roundness : thickness;
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), roundness_thickness);

            aabb bb = aabb_from_triangle(p0, p1, p2);
            aabb_grow(&bb, vec2_splat(roundness_thickness + r->m_AAWidth + r->m_SmoothValue));
            write_float(data, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, roundness_thickness);
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_pie(struct renderer* r, vec2 center, vec2 point, float aperture, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    if (vec2_similar(center, point, small_float))
        return;

    if (aperture <= small_float)
        return;
    
    aperture = float_clamp(aperture, 0.f, VEC2_PI);
    thickness = float_max(thickness * .5f, 0.f);

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_pie, fillmode);

        float* data = r->m_DrawData.NewMultiple((fillmode != fill_hollow) ? 7 : 8);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            center = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, center);
            point = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, point);
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale),  thickness);

            vec2 direction = point - center;
            float radius = vec2_normalize(&direction);
            
            aabb bb = aabb_from_circle(center, radius);
            aabb_grow(&bb, vec2_splat(thickness + r->m_AAWidth + r->m_SmoothValue));

            if (fillmode != fill_hollow)
                write_float(data, center.x, center.y, radius, direction.x, direction.y, sinf(aperture), cosf(aperture));
            else
                write_float(data, center.x, center.y, radius, direction.x, direction.y, sinf(aperture), cosf(aperture), thickness);
                
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_arc_from_circle(struct renderer* r, vec2 p0, vec2 p1, vec2 p2, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    vec2 center, direction;
    float aperture, radius;
    arc_from_points(p0, p1, p2, &center, &direction, &aperture, &radius);

    // colinear points
    if (radius<0.f)
    {
        renderer_draw_orientedbox(r, p0, p2, thickness, 0.f, -1.f, fill_solid, color, op);
        return;
    }

    renderer_draw_arc(r, center, direction, aperture, radius, thickness, fillmode, color, op);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_arc(struct renderer* r, vec2 center, vec2 direction, float aperture, float radius, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    // don't support fill_hollow
    if (fillmode == fill_hollow)
        fillmode = fill_solid;

    aperture = float_clamp(aperture, 0.f, VEC2_PI);
    thickness = float_max(thickness, 0.f);

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_ring, fillmode);

        float* data = r->m_DrawData.NewMultiple(8);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            center = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, center);
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), radius, thickness);

            aabb bb = aabb_from_circle(center, radius);
            aabb_grow(&bb, vec2_splat(thickness + r->m_AAWidth + r->m_SmoothValue));

            write_float(data, center.x, center.y, radius, direction.x, direction.y, sinf(aperture), cosf(aperture), thickness);
            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_unevencapsule(struct renderer* r, vec2 p0, vec2 p1, float radius0, float radius1, float thickness, enum primitive_fillmode fillmode, draw_color color, enum sdf_operator op)
{
    float delta = vec2_distance(p0, p1);

    if (radius0>radius1 && radius0 > radius1 + delta)
    {
        renderer_draw_disc(r, p0, radius0, thickness, fillmode, color, op);
        return;
    }

    if (radius1>radius0 && radius1 > radius0 + delta)
    {
        renderer_draw_disc(r, p1, radius1, thickness, fillmode, color, op);
        return;
    }

    thickness = float_max(thickness * .5f, 0.f);
    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op;
        cmd->type = pack_type(primitive_uneven_capsule, fillmode);

        float* data = r->m_DrawData.NewMultiple((fillmode != fill_hollow) ? 6 : 7);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            p0 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p0);
            p1 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p1);
            distance_screen_space(ortho_get_radius_scale(&r->m_ViewProj, r->m_CameraScale), radius0, radius1);

            aabb bb = aabb_from_capsule(p0, p1, float_max(radius0, radius1));
            aabb_grow(&bb, vec2_splat(r->m_AAWidth + r->m_SmoothValue + thickness));

            if (fillmode != fill_hollow)
                write_float(data, p0.x, p0.y, p1.x, p1.y, radius0, radius1);
            else
                write_float(data, p0.x, p0.y, p1.x, p1.y, radius0, radius1, thickness);

            write_aabb(aabox, bb.min.x, bb.min.y, bb.max.x, bb.max.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_box(struct renderer* r, float x0, float y0, float x1, float y1, draw_color color)
{
    if (x0>x1) swap(x0, x1);
    if (y0>y1) swap(y0, y1);

    vec2 p0 = vec2_set(x0, y0);
    vec2 p1 = vec2_set(x1, y1);

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op_add;
        cmd->type = pack_type(primitive_aabox, fill_solid);

        float* data = r->m_DrawData.NewMultiple(4);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            p0 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p0);
            p1 = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, p1);
            write_float(data, p0.x, p0.y, p1.x, p1.y);
            write_aabb(aabox, p0.x, p0.y, p1.x, p1.y);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_aabb(struct renderer* r, aabb box, draw_color color)
{
    renderer_draw_box(r, box.min.x, box.min.y, box.max.x, box.max.y, color);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_char(struct renderer* r, float x, float y, char c, draw_color color)
{
    if (c < FONT_CHAR_FIRST || c > FONT_CHAR_LAST)
        return;

    draw_command* cmd = r->m_Commands.NewElement();
    if (cmd != nullptr)
    {
        cmd->clip_index = (uint8_t) r->m_ClipsCount-1;
        cmd->color = color;
        cmd->data_index = r->m_DrawData.GetNumElements();
        cmd->op = op_union;
        cmd->type = primitive_char;
        cmd->custom_data = (uint8_t) (c - FONT_CHAR_FIRST);

        float* data = r->m_DrawData.NewMultiple(2);
        quantized_aabb* aabox = r->m_CommandsAABB.NewElement();
        if (data != nullptr && aabox != nullptr)
        {
            write_float(data, x, y);
            write_aabb(aabox, x, y, x + FONT_WIDTH, y + FONT_HEIGHT);
            merge_aabb(r->m_CombinationAABB, aabox);
            return;
        }
        r->m_Commands.RemoveLast();
    }
    log_warn("out of draw commands/draw data buffer, expect graphical artefacts");
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_draw_text(struct renderer* r, float x, float y, const char* text, draw_color color)
{
    vec2 p = ortho_transform_point(&r->m_ViewProj, r->m_CameraPosition, r->m_CameraScale, vec2_set(x, y));
    const float font_spacing = FONT_WIDTH * r->m_FontScale;
    for(const char *c = text; *c != 0; c++)
    {
        renderer_draw_char(r, p.x, p.y, *c, color);
        p.x += font_spacing;
    }
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_set_cliprect(struct renderer* r, uint16_t min_x, uint16_t min_y, uint16_t max_x, uint16_t max_y)
{
    // avoid redundant clip rect
    if (r->m_ClipsCount>0)
    {
        uint32_t index = r->m_ClipsCount-1;
        if (r->m_Clips[index].min_x == min_x && r->m_Clips[index].min_y == min_y &&
            r->m_Clips[index].max_x == max_x && r->m_Clips[index].max_y == max_y)
            return;
    }

    if (r->m_ClipsCount < MAX_CLIPS)
        r->m_Clips[r->m_ClipsCount++] = (clip_rect) {.min_x = min_x, .min_y = min_y, .max_x = max_x, .max_y = max_y};
    else
        log_error("too many clip rectangle! maximum is %d", MAX_CLIPS);
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_set_culling_debug(struct renderer* r, bool b)
{
    r->m_CullingDebug = b;
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_set_viewport(struct renderer* r, float width, float height)
{
    ortho_set_viewport(&r->m_ViewProj, vec2_set(r->m_WindowWidth, r->m_WindowHeight), vec2_set(width, height));
}

//----------------------------------------------------------------------------------------------------------------------------
void renderer_set_camera(struct renderer* r, vec2 position, float scale)
{
    r->m_CameraPosition = position;
    r->m_CameraScale = scale;
}

