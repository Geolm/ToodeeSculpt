# GPU2DComposer


## In a nutshell

* GPU Driven, culling, rasterization, blending all done on the GPU using compute shaders and indirect draw
* "free" alpha blending for the shapes since everything is computed in the pixel shader (no r/w access). Only the final blend uses the raster operation pipeline (ROP)
* anti-aliasing based on sdf (no multisampling)
* boolean operation to combine shapes

## Implementation details

### Supported shapes
* oriented bounding box
* disc, circle can be achieved modifier (see below)
* capsule
* arc
  
### Distance field modifier
* round modifier : add a distance bias to SDF, make shape "round" 
* outline modifier: allow to draw circle, rectangle, etc... from solid shapes

### Commands

Draw commands are store in a buffer and set to the GPU

A command contains:
* the type of command
* the SDF modifier
* the clip index that points to an entry in the clip array
* a 32bits color
* some float data (center, radius, points... number of float is depending on the shapes)

### SDF operators

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

## Renderer

### Step 1 : Tiles binning

The windows is divided in tiles (32x32 for example). A compute shader generates a linked list of draw commands for each tile by testing intersection between shapes and the tile aabb.

### Step 2 : Draw indirect preparation

### Step 3 : Rasterization


