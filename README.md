# GPU2DComposer

## GPU driven

Except from maintaining the list of draw, the CPU is not involved in the rendering. All the tile binnning and rasterization is done on the GPU via compute and pixel shaders.

### Type of command
* sdf_box : oriented bounding box
* sdf_disc : circle can be achieved modifier (see below)
* sdf_line
* sdf_arc
* start_combination : allow multiple shape to be combined with operator (see below)
* end_combination


### Distance field modifier
* modifier_none
* modifier_round : add a distance bias to SDF, make shape "round" 
* modifier_outline : allow to draw circle, rectangle, etc... from solid shapes


### Command 

* 

```
struct command
{
  enum command_type type;
  enum sdf_modifier modifier;
  uint8_t clip_index;
  uint8_t pad;
  uint32_t color;

  uint32_t data_index;      // command data are stored in a separate buffer of float
  uint32_t next_command;    // linked-list point to the next command
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
