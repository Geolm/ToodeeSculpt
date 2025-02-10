# Renderer

We built our renderer to draw 2d primitive based on signed distance field (SDF). It's a GPU based renderer where most of the job is done on the GPU via indirect calls. The renderer consists in 4 parts

* creation of the global draw commands list (CPU)
* primitives transform and animation (GPU)
* tile binning and linked list generation (GPU)
* rasterization (GPU)

You can find below more details about the implementation but here are some specific features of this renderer :

* **anti-aliasing** : all primitives are anti-aliased without additionnal rendertarget or a fullscreen effect. It's almost free
* **alpha-blending** : all blending is done in the shader, you can stack as much alpha blended primitive you want. You pay only once the cost of alpha blending
* **no polygons** : perfect curves, curves do not cost more than straight lines
* **boolean operations** on primitives, we also support smooth blend (see https://iquilezles.org/articles/smin/)


## Draw command list

We build the command lists on the CPU, as it's 2d rendering the **order** of commands is important. A command describes a primitive. Here's the list of primitive currently supported :
* disc
* oriented box
* pie
* arc
* uneven capsule
* oriented ellipse
* triangle

Each command also have:
* a fill mode : solid, hollow or outline
* a color RGBA8
* a clip index that reference a clip rectangle
* an boolean operation : add, smooth add, sub, intersection
* an index to the primitive float data

All data the primitive (coordinates, radius, width, ...) is stored in a big float buffer that each command points at.
