
#include "MetalLayerHelper.h"
#include "App.h"

#define UNUSED_VARIABLE(a) (void)(a)

// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    UNUSED_VARIABLE(argc);
    UNUSED_VARIABLE(argv);

    MetalLayerHelper helper;
    App app;

    const GLFWvidmode* videomode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    UNUSED_VARIABLE(videomode);
    
    helper.Init("ToodeeSculpt", 1920, 1080);

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
