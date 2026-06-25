#include "ui/components/label.hpp"

namespace lunar3d {
namespace ui {
namespace components {

Label::Label() {}

Label::Label(const char* text, Point position, float scale, u32 color)
    : position_(position), scale_(scale), color_(color) {
    setText(text);
}

Label::~Label() {
    C2D_TextBufDelete(textBuffer_);
}

void Label::setText(const char* text) {
    C2D_TextBufClear(textBuffer_);
    C2D_TextParse(&text_, textBuffer_, text);
    C2D_TextOptimize(&text_);
}

void Label::setPosition(Point position) {
    position_ = position;
}

void Label::setScale(float scale) {
    scale_ = scale;
}

void Label::setColor(u32 color) {
    color_ = color;
}

bool Label::render(C3D_RenderTarget& target) const {
    (void)target;
    C2D_DrawText(&text_, C2D_WithColor, position_.x, position_.y, 0.0f, scale_, scale_, color_);
    return true;
}

} // namespace components
} // namespace ui
} // namespace lunar3d
