# ToodeeSculpt

Combine [2d sdf primitives](https://iquilezles.org/articles/distfunctions2d/) with smooth operator to sculpt your shape

![2024-11 screenshot](/images/Screenshot%202024-11-25%20at%2016.55.53.png)

###### Work in progress
* only available on macOS Apple Silicon
* lot of bugs, missing features

## Why
* analytical shape, can adapt any resolution
* can produce smooth shapes without tesselation
* lightweight vs PNG image
* fast to render on GPU compared to bezier curves
* out of the box anti-aliasing

## Features
* Primitives : disc, triangle, oriented box, ellipse, 
* Primitive can be rounded
* Boolean operator : union, substraction, [smooth blend](https://iquilezles.org/articles/smin/)
* Intuitive editor, undo support, grid snapping

## GPU driven SDF renderer

This editor renders everything with a gpu-driven pipeline
* only a list of command is uploaded to the GPU
* binning, rasterization, blending is all done on GPU (i.e Compute shader + Draw Indirect)
* pixels are rendered once, all [blending](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-23-high-speed-screen-particles) is done in the shader (a.k.a free alpha blending)
* anti-aliasing based on sdf
* uses the metal API (until sokol_graphics supports gpu-driven features, nudge nudge)

## Planned Dev

* more primitives : arc, pie
* export to PNG, shadertoy shader
* editor : multiple selection, copy/paste, zoom
* save/load
