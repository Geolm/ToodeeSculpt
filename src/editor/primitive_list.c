#include "primitive_list.h"
#include "primitive.h"
#include "assert.h"

#define CC_NO_SHORT_NAMES
#include "../system/cc.h"

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
    return cc_size(&list) - 1;
}

// ---------------------------------------------------------------------------------------------------------------------------
uint32_t plist_size(void)
{
    return cc_size(&list);
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
void plist_terminate(void)
{
    cc_cleanup(&list);
}