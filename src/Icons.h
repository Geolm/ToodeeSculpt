#pragma once

#include "renderer/Renderer.h"
#include "system/aabb.h"

enum icon_type
{
    ICON_CLOSE,
    ICON_CHECK,
    ICON_COLLAPSED,
    ICON_EXPANDED
};

void DrawIcon(Renderer& renderer, aabb box, icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second);