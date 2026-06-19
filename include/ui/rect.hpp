#pragma once

#include "ui/point.hpp"
#include "ui/size.hpp"

namespace lunar3d {
namespace ui {

struct Rect {
    Rect();
    Rect(Point origin, Size size);
    Rect(float x, float y, float width, float height);

    Point origin;
    Size size;

    bool contains(Point point) const;
};

} // namespace ui
} // namespace lunar3d
