#ifndef __ICONS_H__
#define __ICONS_H__

#include <stdint.h>
#include "system/aabb.h"
#include "shaders/common.h"

enum icon_type
{
    ICON_CLOSE,
    ICON_CHECK,
    ICON_COLLAPSED,
    ICON_EXPANDED
};

#ifdef __cplusplus
extern "C" {
#endif

struct renderer;

void DrawIcon(struct renderer* r, aabb box, enum icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second);

#ifdef __cplusplus
}
#endif

#endif
