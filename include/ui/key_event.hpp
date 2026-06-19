#pragma once

#include <3ds/types.h>

namespace lunar3d {
namespace ui {

enum class KeyAction {
    Down,
    Up,
};

struct KeyEvent {
    KeyAction action = KeyAction::Down;
    u32 key = 0;
};

} // namespace ui
} // namespace lunar3d
