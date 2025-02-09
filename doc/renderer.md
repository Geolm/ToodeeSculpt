# Renderer

We built our renderer to draw 2d shapes based on signed distance field (SDF). It's a GPU based renderer where most of the job is done on the GPU via indirect calls. The renderer consists in 4 parts

* creation of the global draw commands list (CPU)
* shapes transform and animation (GPU)
* tile binning and linked list generation (GPU)
* rasterization (GPU)

As we build a linked list for each tile of the screen, 