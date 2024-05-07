
#include "MetalLayerHelper.h"
#include "App.h"


// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    MetalLayerHelper helper;
    App app;
    
    helper.Init("DistCraft", 1980, 1080);
    
    app.Init(helper.GetDevice(), helper.GetGLFWWindow());
    app.Loop();
    app.Terminate();
    
    helper.Terminate();

    return 0;
}
