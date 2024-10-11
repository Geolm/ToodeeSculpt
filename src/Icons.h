#pragma once

#include "renderer/Renderer.h"
#include "system/aabb.h"

enum icon_type
{
    ICON_HEART,
    ICON_STOPWATCH,
    ICON_LOADING,
    ICON_WIFI
};

void DrawIcon(Renderer& renderer, aabb box, icon_type icon, draw_color primaray_color, draw_color secondary_color, float time_in_second);