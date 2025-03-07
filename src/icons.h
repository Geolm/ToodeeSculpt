#ifndef __ICONS_H__
#define __ICONS_H__

#include <stdint.h>
#include "system/aabb.h"
#include "shaders/common.h"

enum icon_type
{
    ICON_CLOSE = 1,     // in sync with microui
    ICON_CHECK,
    ICON_COLLAPSED,
    ICON_EXPANDED,
    ICON_DISC,
    ICON_ORIENTEDBOX,
    ICON_ELLIPSE,
    ICON_TRIANGLE,
    ICON_PIE,
    ICON_ARC,
    ICON_SPLINE,
    ICON_CAPSULE,
    ICON_SCALE,
    ICON_ROTATE,
    ICON_LAYERUP,
    ICON_LAYERDOWN,
    ICON_SETTINGS,
    ICON_TRAPEZOID
};

#ifdef __cplusplus
extern "C" {
#endif

struct renderer;

void DrawIcon(struct renderer* r, aabb box, enum icon_type icon, draw_color primary_color, draw_color secondary_color, float time_in_second);

#ifdef __cplusplus
}
#endif

#endif
