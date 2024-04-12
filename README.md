# GPU2DComposer

## GPU driven

Except from maintaining the list of draw, the CPU is not involved in the rendering. All the tile binnning and rasterization is done on the GPU via compute and pixel shaders.

### Draw commands

### Tiles binning

### Rasterization

### SDF operators

## WebGPU native

### Why this API?

WebGPU is a clean, not overly too complicated API (looking at you vulkan) than run on all platform and supports compute shaders, indirect draw/dispatch that are needed for gpu-driven pipeline.
