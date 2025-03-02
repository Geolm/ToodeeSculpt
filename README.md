# ToodeeSculpt

Combine [2d sdf primitives](https://iquilezles.org/articles/distfunctions2d/) with smooth operator to sculpt your shape

![2024-11 screenshot](/images/Screenshot%202025-03-02%20at%2016.23.08.png)


## Why SDF?
* analytical shape, can adapt any resolution
* can produce smooth shapes without generating a ton of triangles
* lightweight vs PNG image
* fast to render on GPU compared to bezier curves
* out of the box anti-aliasing

## Features
* Primitives : disc, triangle, oriented box, ellipse, pie, arc
* Primitive can be rounded, outline
* Boolean operator : union, substraction, [smooth blend](https://iquilezles.org/articles/smin/)
* Intuitive editor, undo support, grid snapping, copy/paste, rotation
* Save/load

## GPU driven SDF renderer

This editor renders everything with a gpu-driven pipeline
* only a list of command is uploaded to the GPU
* binning, rasterization, blending is all done on GPU (i.e Compute shader + Draw Indirect)
* pixels are rendered once, all [blending](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-23-high-speed-screen-particles) is done in the shader (a.k.a free alpha blending)
* anti-aliasing based on sdf
* uses the metal API (until sokol_graphics supports gpu-driven features, nudge nudge)

More details to come in the documentation.

## How to build
### Prerequisite
* a macOS dev environment (xcode tools)
* glfw installed via homebrew
* cmake
### Compilation
* open a terminal in the folder
* mkdir build
* cd build
* cmake -DCMAKE_BUILD_TYPE=Release ..
* cmake --build .
* ./ToodeeSculpt

It only has been tested on a M2 Max running macOS 12.7.1