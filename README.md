# ToodeeSculpt

Combine [2d sdf primitives](https://iquilezles.org/articles/distfunctions2d/) with smooth operator to sculpt your shape

* the editor
![2024-11 screenshot](/images/Screenshot%202025-03-02%20at%2016.23.08.png)

* same shader exported in shadertoy
![alt text](images/shader_toy_export.png)


## Features
* Primitives : disc, triangle, oriented box, ellipse, pie, arc, spline, capsule
* Primitive can be rounded, rendered with outline
* Boolean operator : union, substraction, [smooth blend](https://iquilezles.org/articles/smin/)
* Intuitive editor, undo support, grid snapping, copy/paste, rotation, scaling
* Load/save, export to shadertoy

## Specialized SDF renderer

* Everything is rendered with just **one** drawcall
* Primitives are binned per tile on the GPU, you don't pay the cost of far away primitives (as you do with your typical shadertoy shader)
* Pixels are written once, alpha blending is done in the shader (a.k.a free alpha blending)
* Anti-aliasing based on sdf

Find more details in the the [documentation of the renderer](/doc/renderer.md)

## Next steps
* Primitive animation
* More export options (PNG, sprite for in-house engine)
* Better user experience
* More primitives, more options

## How to build
### Prerequisite
* a macOS dev environment (xcode tools)
* glfw (brew install glfw)
* cmake
### Compilation
* open a terminal in the folder
* mkdir build
* cd build
* cmake -DCMAKE_BUILD_TYPE=Release ..
* cmake --build .
* ./ToodeeSculpt

It only has been tested on a M2 Max studio and a M1 Pro mac bookpro running macOS 14.x
Currently, there is no plan to support another platform. 


### Libraries used (in alphabetic particular order)

* [convenient containers](https://github.com/JacksonAllan/CC)
* [metal-cpp](https://developer.apple.com/metal/cpp/)
* [microui](https://github.com/rxi/microui)
* [nativefiledialog](https://github.com/mlabbe/nativefiledialog)
* [sokol_time](https://github.com/floooh/sokol/blob/master/sokol_time.h)
* [spng](https://github.com/randy408/libspng)
* [whereami](https://github.com/gpakosz/whereami)

## Thanks

* [commit mono font](https://github.com/eigilnikolajsen/commit-mono)
* [inigo quilez 2d sdf](https://iquilezles.org/articles/distfunctions2d/)
* the bc4 encoder from [stb_dxt.h](https://github.com/nothings/stb/blob/master/stb_dxt.h)
* the spline of biarcs code inspired by [this paper](https://cad-journal.net/files/vol_18/CAD_18(1)_2021_66-85.pdf)
* color palettes from [lospec website](https://lospec.com/palette-list/)