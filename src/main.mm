
#include "MetalLayerHelper.h"
#include "App.h"


// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    MetalLayerHelper helper;
    App app;
    
    helper.Init("FieldCraft", 1980/2, 1080/2);

    app.Init(helper.GetDevice(), helper.GetGLFWWindow());

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
