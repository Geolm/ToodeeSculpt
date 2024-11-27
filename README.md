# ToodeeSculpt

Combine [2d sdf primitives](https://iquilezles.org/articles/distfunctions2d/) with smooth operator to sculpt your shape

![2024-11 screenshot](/images/Screenshot%202024-11-25%20at%2016.55.53.png)


## Features
* Primitives : disc, triangle, oriented box, ellipse, _arc_, _pie_
* Primitive can be rounded
* Boolean operator : union, substraction, _intersection_
* [Smooth blend](https://iquilezles.org/articles/smin/)
* _Export to shader, image_

_Italic_ denotes planned features.

## GPU driven SDF renderer

This editor render everything with a gpu-driven renderer
* only a list of command is uploaded to the GPU
* binning, rasterization, blending is all done on GPU (i.e Compute shader + Draw Indirect)
* pixel are rendered just once, all blending is done in the shader (a.k.a free alpha blending)
* anti-aliasing based on sdf
