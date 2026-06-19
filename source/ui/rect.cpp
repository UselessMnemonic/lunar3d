#include "ui/rect.hpp"

namespace lunar3d {
namespace ui {

Rect::Rect() : origin(), size() {}

Rect::Rect(Point rectOrigin, Size rectSize) : origin(rectOrigin), size(rectSize) {}

Rect::Rect(float rectX, float rectY, float rectWidth, float rectHeight)
    : origin(rectX, rectY), size(rectWidth, rectHeight) {}

bool Rect::contains(Point point) const {
    return point.x >= origin.x && point.y >= origin.y &&
           point.x < origin.x + size.width && point.y < origin.y + size.height;
}

} // namespace ui
} // namespace lunar3d
