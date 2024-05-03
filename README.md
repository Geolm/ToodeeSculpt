# GPU2DComposer


## In a nutshell

* GPU Driven, culling, rasterization, blending all done on the GPU using compute shaders and indirect draw
* "free" alpha blending for the shapes since everything is computed in the pixel shader (no r/w access). Only the final blend uses the raster operation pipeline (ROP)
* anti-aliasing based on sdf (no multisampling)
* boolean operation to combine shapes

## Type of command
* sdf_box : oriented bounding box
* sdf_disc : circle can be achieved modifier (see below)
* sdf_line
* sdf_arc
* start_combination : allow multiple shape to be combined with operator (see below)
* end_combination


## Distance field modifier
* modifier_none
* modifier_round : add a distance bias to SDF, make shape "round" 
* modifier_outline : allow to draw circle, rectangle, etc... from solid shapes

## Command 

A command contains
* the type of command
* the SDF modifier
* the clip index that points to an entry in the clip array
* a 32bits color

## Global arrays

* commands data (a big array of float)
* clipping rectangle array

## Tiles binning

## Rasterization

## SDF operators

We support 3 operators : union, substraction and intersection with 2 flavors : sharp or smooth. See this [article](https://iquilezles.org/articles/smin/) by Inigo Quilez if you want to learn about smooth minimum.

```C

float opUnion( float d1, float d2 )
{
    return min(d1,d2);
}
float opSubtraction( float d1, float d2 )
{
    return max(-d1,d2);
}
float opIntersection( float d1, float d2 )
{
    return max(d1,d2);
}
```
