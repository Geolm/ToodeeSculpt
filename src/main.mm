
#include "MetalApp.h"



// ---------------------------------------------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) 
{
    MetalApp app;
    
    app.Init("GPU2dComposer", 1980, 1080);
    app.Run();
    app.Cleanup();
    


    return 0;
}
