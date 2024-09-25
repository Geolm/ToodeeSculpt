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
#define constant
#define atomic_uint uint32_t
#define device
#define command_buffer void*
typedef struct alignas(8) {float x, y;} float2;
#else
using namespace metal;
#endif

enum command_type
{
    shape_aabox = 0,
    shape_oriented_box = 1,
    shape_circle = 2,
    shape_circle_filled = 3,
    shape_arc = 4,
    shape_arc_filled = 5,
    shape_triangle = 6,
    shape_triangle_filled = 7,
    shape_char = 8,
    combination_begin = 16,
    combination_end = 17
};

enum sdf_operator
{
    op_none = 0,
    op_union = 1,
    op_subtraction = 2,
    op_intersection = 3
};

struct draw_color
{
    uint32_t packed_data;

#ifndef __METAL_VERSION__
    draw_color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
    {
        packed_data = (alpha<<24) | (blue<<16) | (green<<8) | red;
    }
    explicit draw_color(uint32_t color) 
    {
        packed_data = color;
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
