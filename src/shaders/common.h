// ---------------------------------------------------------------------------------------------------------------------------
// renderer constants
#define TILE_SIZE (16)
#define MAX_NODES_COUNT (1<<20)
#define INVALID_INDEX (0xffffffff)
#define MAX_CLIPS (256)
#define MAX_COMMANDS (1<<16)
#define MAX_DRAWDATA (MAX_COMMANDS * 4)

// font constants
#define FONT_TEXTURE_WIDTH 256
#define FONT_TEXTURE_HEIGHT 256
#define FONT_CHAR_FIRST 33
#define FONT_CHAR_LAST	126
#define FONT_CHARS 95
#define FONT_CHAR_WIDTH 21
#define FONT_CHAR_HEIGHT 31
#define FONT_WIDTH 10
#define FONT_HEIGHT 16

// ---------------------------------------------------------------------------------------------------------------------------
// cpp compatibility
#ifndef __METAL_VERSION__
#pragma once
#define constant
#define atomic_uint uint32_t
#define device
#define command_buffer void*
#define texture_half uint64_t
#ifdef __cplusplus
    typedef struct alignas(8) {float x, y;} float2;
#else
    typedef struct {float x, y;} float2;
#endif
#else
using namespace metal;
#define texture_half texture2d<half>
#endif

// packed on 6 bits
enum command_type
{
    primitive_char = 0,
    primitive_aabox = 1,
    primitive_oriented_box = 2,
    primitive_disc = 3,
    primitive_triangle = 4,
    primitive_ellipse = 5,
    primitive_pie = 6,
    primitive_ring = 7,
    primitive_uneven_capsule = 8,
    
    combination_begin = 32,
    combination_end = 33
};

enum primitive_fillmode
{
    fill_solid = 0,
    fill_outline = 1,
    fill_hollow = 2,
    fill_last = 3
};

#define COMMAND_TYPE_MASK   (0x3f)
#define PRIMITIVE_FILLMODE_MASK (0xC0)
#define PRIMITIVE_FILLMODE_SHIFT (6)


enum sdf_operator
{
    op_add = 0,
    op_union = 1,
    op_subtraction = 2,
    op_intersection = 3,
    op_last = 4
};

typedef struct draw_color
{
    uint32_t packed_data;

#if !defined(__METAL_VERSION__) && defined(__cplusplus)
    draw_color() = default;
    explicit draw_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    {
        packed_data = (alpha<<24) | (blue<<16) | (green<<8) | red;
    }
    explicit draw_color(uint32_t color)
    {
        packed_data = color;
    }
    draw_color(uint32_t rgb, uint8_t alpha)
    {
        packed_data = (alpha<<24) | (rgb&0x00ffffff);
    }

    void from_float(float red, float green, float blue, float alpha)
    {
        packed_data = (uint8_t(alpha * 255.f)<<24) | (uint8_t(blue * 255.f)<<16) | (uint8_t(green*255.f)<<8) | uint8_t(red*255.f);
    }
#endif
} draw_color;

static inline draw_color draw_color_from_float(float red, float green, float blue, float alpha)
{
    draw_color color;
    color.packed_data = ((uint8_t)(alpha * 255.f)<<24) | ((uint8_t)(blue * 255.f)<<16) | ((uint8_t)(green*255.f)<<8) | (uint8_t)(red*255.f);
    return color;
}

typedef struct draw_command
{
    uint8_t type;
    uint8_t clip_index;
    uint8_t op;
    uint8_t custom_data;
    draw_color color;
    uint32_t data_index;
} draw_command;

static inline uint8_t pack_type(enum command_type type,  enum primitive_fillmode fillmode)
{
    uint8_t result = ((uint8_t)fillmode) << PRIMITIVE_FILLMODE_SHIFT;
    result |= (uint8_t)(type&COMMAND_TYPE_MASK); 
    return result;
}

static inline bool primitive_is_filled(uint8_t type)
{
    return (type>>PRIMITIVE_FILLMODE_SHIFT) == fill_solid;
}

static inline enum command_type primitive_get_type(uint8_t type)
{
    return (enum command_type)(type&COMMAND_TYPE_MASK);
}

static inline enum primitive_fillmode primitive_get_fillmode(uint8_t type)
{
    return (enum primitive_fillmode)(type>>PRIMITIVE_FILLMODE_SHIFT);
}

typedef struct tile_node
{
    uint32_t command_index;
    uint32_t next;
} tile_node;

typedef struct counters
{
    atomic_uint num_nodes;
    atomic_uint num_tiles;
    uint32_t pad[2];
} counters;

typedef struct clip_rect
{
    uint16_t min_x, min_y, max_x, max_y;
} clip_rect;

typedef struct quantized_aabb
{
    uint8_t min_x, min_y, max_x, max_y;
} quantized_aabb;

typedef struct draw_cmd_arguments
{
    constant draw_command* commands;
    constant quantized_aabb* commands_aabb;
    constant float* draw_data;
    texture_half font;
    clip_rect clips[MAX_CLIPS];
    uint32_t num_commands;
    uint32_t max_nodes;
    uint16_t num_tile_width;
    uint16_t num_tile_height;
    float aa_width;
    float2 screen_div;
    float font_scale;
    draw_color outline_color;
    float outline_width;
    bool culling_debug;
} draw_cmd_arguments;

typedef struct tiles_data
{
    device tile_node* head;
    device tile_node* nodes;
    device uint16_t* tile_indices;
} tiles_data;

typedef struct output_command_buffer
{
    command_buffer cmd_buffer;
} output_command_buffer;

#ifdef __METAL_VERSION__
float2 skew(float2 v) {return float2(-v.y, v.x);}
#endif

// ---------------------------------------------------------------------------------------------------------------------------
// cpp compatibility
#ifndef __METAL_VERSION__
#undef constant
#undef atomic_uint
#undef device
#undef command_buffer
#endif
