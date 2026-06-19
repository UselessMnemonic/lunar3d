#pragma once

namespace lunar3d {
namespace ui {

enum class TouchAction {
    Down,
    Move,
    Up,
};

struct TouchEvent {
    TouchAction action = TouchAction::Down;
    int x = 0;
    int y = 0;
};

} // namespace ui
} // namespace lunar3d
