#pragma once

#include <3ds.h>

namespace lunar3d {
namespace ui {

struct HidInput {
    u32 keysDown = 0;
    u32 keysHeld = 0;
    u32 keysUp = 0;
    touchPosition touch = {};
    circlePosition circlePad = {};
};

} // namespace ui
} // namespace lunar3d
