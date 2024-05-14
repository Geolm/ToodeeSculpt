enum command_type
{
    sdf_box = 0,
    sdf_disc = 1,
    sdf_capsule = 2,
    sdf_arc = 3,
    start_combination = 16,
    end_combination = 17
};
    
enum command_modifier
{
    modifier_none = 0,
    modifier_round = 1,
    modifier_outline = 2
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
    uint8_t modifier;
    uint8_t clip_index;
    uint8_t op;
    uint32_t color;
    uint32_t data_index;
};