# GPU2DComposer

## GPU driven

Except from maintaining the list of draw, the CPU is not involved in the rendering. All the tile binnning and rasterization is done on the GPU via compute and pixel shaders.

### Draw commands

### Tiles binning

### Rasterization

### SDF operators

We support 3 operators : union, substraction and intersection with 2 flavors : sharp or smooth. See this [article](https://iquilezles.org/articles/smin/) by Inigo Quilez if you want to learn about smooth minimum.

## WebGPU native

### Why this API?

WebGPU is a clean, not overly too complicated API (looking at you vulkan) that runs on all platforms and supports compute shaders and indirect draw/dispatch needed for a gpu-driven pipeline.
