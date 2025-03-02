
#include "renderer/MetalLayerHelper.h"
#include "App.h"

#include "system/log.h"

#define UNUSED_VARIABLE(a) (void)(a)

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    MetalLayerHelper helper;
    App app;

    helper.Init();

    const GLFWvidmode* videomode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    log_info("screen resolution is %dx%d", videomode->width, videomode->height);

    helper.InitWindow("ToodeeSculpt", 1920, 1080);

    app.Init(helper.GetDevice(), helper.GetGLFWWindow(), 1920, 1080);

    while (!glfwWindowShouldClose(helper.GetGLFWWindow()))
    {
        NS::AutoreleasePool* pPool   = NS::AutoreleasePool::alloc()->init();
        app.Update(helper.GetDrawble());
        pPool->release();
        
        glfwPollEvents();
    }
    
    app.Terminate();
    
    helper.Terminate();

    return 0;
}
