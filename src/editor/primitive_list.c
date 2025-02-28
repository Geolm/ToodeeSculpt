#include "primitive_list.h"
#include "primitive.h"
#include "export.h"
#include <assert.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define CC_NO_SHORT_NAMES
#include "../system/cc.h"
#include "../system/log.h"
#include "../system/format.h"

const size_t clipboard_buffer_size = (1<<18);
static cc_vec(struct primitive) list;

// ---------------------------------------------------------------------------------------------------------------------------
void plist_init(uint32_t reservation)
{
    cc_init(&list);
    cc_reserve(&list, reservation);
}

// ---------------------------------------------------------------------------------------------------------------------------
struct primitive* plist_get(uint32_t index)
{
    assert(index < cc_size(&list));
    return cc_get(&list, index);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_clear(void)
{
    cc_clear(&list);
}

// ---------------------------------------------------------------------------------------------------------------------------
uint32_t plist_last(void)
{
    return plist_size() - 1;
}

// ---------------------------------------------------------------------------------------------------------------------------
uint32_t plist_size(void)
{
    return (uint32_t)(cc_size(&list));
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_push(struct primitive* p)
{
    cc_push(&list, *p);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_erase(uint32_t index)
{
    assert(index < cc_size(&list));
    cc_erase(&list, index);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_insert(uint32_t index, struct primitive* p)
{
    assert(index < cc_size(&list));
    cc_insert(&list, index, *p);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_resize(uint32_t new_size)
{
    cc_resize(&list, new_size);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_serialize(serializer_context* context, bool normalization, const aabb* edition_zone)
{
    size_t array_size = cc_size(&list);
    serializer_write_size_t(context, array_size);
    for(uint32_t i=0; i<array_size; ++i)
    {
        struct primitive p = *cc_get(&list, i);
        if (normalization)
            primitive_normalize(&p, edition_zone);

        primitive_serialize(&p, context);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_deserialize(serializer_context* context, uint16_t major, uint16_t minor, bool normalization, const aabb* edition_zone)
{
    size_t array_size = serializer_read_size_t(context);
    log_debug("%d primitives found", array_size);

    plist_resize((uint32_t)array_size);
    for(uint32_t i=0; i<array_size; ++i)
    {
        struct primitive* p = plist_get(i);
        primitive_deserialize(p, context, major, minor);
        if (normalization)
            primitive_expand(p, edition_zone);
        primitive_update_aabb(p);
    }
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_export(struct GLFWwindow* window, float smooth_blend, const aabb* edition_zone)
{
    struct string_buffer clipboard = string_buffer_init(clipboard_buffer_size);

    shadertoy_start(&clipboard);

    float normalized_smooth_blend = smooth_blend / aabb_get_size(edition_zone).x;
    for(uint32_t i=0; i<plist_size(); ++i)
    {
        struct primitive p = *plist_get(i);
        primitive_normalize(&p, edition_zone);
        shadertoy_export_primitive(&clipboard, &p, i, normalized_smooth_blend);
    }

    shadertoy_finalize(&clipboard);
    glfwSetClipboardString(window, clipboard.buffer);

    string_buffer_terminate(&clipboard);
}

// ---------------------------------------------------------------------------------------------------------------------------
void plist_terminate(void)
{
    cc_cleanup(&list);
}
