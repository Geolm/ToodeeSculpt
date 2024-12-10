// ---------------------------------------------------------------------------------------------------------------------------
// constants
#define TILE_SIZE (16)
#define MAX_NODES_COUNT (1<<20)
#define INVALID_INDEX (0xffffffff)
#define MAX_CLIPS (256)
#define MAX_COMMANDS (1<<16)
#define MAX_DRAWDATA (MAX_COMMANDS * 4)

// ---------------------------------------------------------------------------------------------------------------------------
// cpp compatibility
#ifndef __METAL_VERSION__
#pragma once
#define constant
#define atomic_uint uint32_t
#define device
#define command_buffer void*
typedef struct alignas(8) {float x, y;} float2;
#else
using namespace metal;
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
    
    combination_begin = 32,
    combination_end = 33
};

#define COMMAND_TYPE_MASK   (0x3f)
#define PRIMITIVE_FILLED    (0x80)


enum sdf_operator
{
    op_add = 0,
    op_union = 1,
    op_subtraction = 2,
    op_intersection = 3,
    op_last = 4
};

struct draw_color
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
};

struct draw_command
{
    uint8_t type;
    uint8_t clip_index;
    uint8_t op;
    uint8_t custom_data;
    draw_color color;
    uint32_t data_index;
};

static inline uint8_t pack_type(enum command_type type,  bool filled)
{
    uint8_t result = filled ? PRIMITIVE_FILLED : 0;
    result |= (uint8_t)(type&COMMAND_TYPE_MASK); 
    return result;
}

static inline bool primitive_is_filled(uint8_t type)
{
    return (type&PRIMITIVE_FILLED);
}

static inline enum command_type primitive_get_type(uint8_t type)
{
    return (enum command_type)(type&COMMAND_TYPE_MASK);
}

struct tile_node
{
    uint32_t command_index;
    uint32_t next;
};

struct counters
{
    atomic_uint num_nodes;
    atomic_uint num_tiles;
    uint32_t pad[2];
};

struct clip_rect
{
    uint16_t min_x, min_y, max_x, max_y;
};

struct quantized_aabb
{
    uint8_t min_x, min_y, max_x, max_y;
};

struct draw_cmd_arguments
{
    constant draw_command* commands;
    constant quantized_aabb* commands_aabb;
    constant float* draw_data;
    constant uint16_t* font;
    clip_rect clips[MAX_CLIPS];
    uint32_t num_commands;
    uint32_t max_nodes;
    uint16_t num_tile_width;
    uint16_t num_tile_height;
    float aa_width;
    float2 screen_div;
    float2 font_size;
    float font_scale;
    bool debug_output;
};

struct tiles_data
{
    device tile_node* head;
    device tile_node* nodes;
    device uint16_t* tile_indices;
};

struct output_command_buffer
{
    command_buffer cmd_buffer;
};

// ---------------------------------------------------------------------------------------------------------------------------
// cpp compatibility
#ifndef __METAL_VERSION__
#undef constant
#undef atomic_uint
#undef device
#undef command_buffer
#endif
