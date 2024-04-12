# GPU2DComposer

## GPU driven

Except from maintaining the list of draw, the CPU is not involved in the rendering. All the tile binnning and rasterization is done on the GPU via compute and pixel shaders.

### Draw commands

Draw commands are stored in an array, order is important.

```C
enum command_type : uint8_t
{
  sdf_box,
  sdf_disc,
  sdf_line,
  sdf_arc,
  start_combination,
  end_combination
};

enum sdf_modifier : uint8_t
{
  modifier_none,
  modifier_round,
  modifier_onion
};

// 32 bytes command
struct command
{
  enum command_type type;
  enum sdf_modifier modifier;
  uint16_t pad;
  
  float2 p0, p1;
  float radius, value;
  uint32_t color;
};

```

### Tiles binning

### Rasterization

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
float opXor(float d1, float d2 )
{
    return max(min(d1,d2),-max(d1,d2));
}
```

## WebGPU native

### Why this API?

WebGPU is a clean, not overly too complicated API (looking at you vulkan) that runs on all platforms and supports compute shaders and indirect draw/dispatch needed for a gpu-driven pipeline.
