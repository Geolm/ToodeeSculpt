enum command_type
{
    shape_rect = 0,
    shape_rect_filled = 1,
    shape_circle = 2,
    shape_circle_filled = 3,
    shape_arc = 4,
    shape_arc_filled = 5,
    shape_line = 6,
    shape_triangle = 7,
    shape_triangle_filled = 8,
    combination_begin = 16,
    combination_end = 17
};

enum sdf_operator
{
    op_none = 0,
    op_union = 1,
    op_subtraction = 2,
    op_intersection = 3,
    op_smooth_union = 4
};

struct draw_command
{
    uint8_t type;
    uint8_t clip_index;
    uint8_t op;
    uint8_t custom_data;
    uint32_t color;
    uint32_t data_index;
};

struct tile_node
{
    uint32_t command_index;
    uint32_t next;
};

struct counters
{
    uint32_t num_nodes;  // should be clear to zero each frame start
    uint32_t pad[3];
};
