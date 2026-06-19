#include "ui/components/spinner.hpp"

#include "ui/glyphs.hpp"

namespace lunar3d {
namespace ui {
namespace components {

namespace {

constexpr u64 FrameDurationMilli = 100;
constexpr int FrameCount = 8;

const char* glyphForFrame(int frame) {
    switch (frame % FrameCount) {
    case 0:
        return glyphs::Loader0;
    case 1:
        return glyphs::Loader1;
    case 2:
        return glyphs::Loader2;
    case 3:
        return glyphs::Loader3;
    case 4:
        return glyphs::Loader4;
    case 5:
        return glyphs::Loader5;
    case 6:
        return glyphs::Loader6;
    default:
        return glyphs::Loader7;
    }
}

} // namespace

Spinner::Spinner() {}

Spinner::Spinner(Point position, float scale, u32 color)
    : position_(position), scale_(scale), color_(color) {
    updateText();
}

Spinner::~Spinner() {
    C2D_TextBufDelete(textBuffer_);
}

void Spinner::setPosition(Point position) {
    position_ = position;
}

void Spinner::setScale(float scale) {
    scale_ = scale;
}

void Spinner::setColor(u32 color) {
    color_ = color;
}

void Spinner::reset() {
    elapsedMilli_ = 0;
    frame_ = 0;
    updateText();
}

void Spinner::update(u64 deltaMilli) {
    if (!visible()) {
        reset();
        return;
    }

    if (!textBuffer_) {
        return;
    }

    elapsedMilli_ += deltaMilli;
    if (elapsedMilli_ < FrameDurationMilli) {
        return;
    }

    frame_ = (frame_ + static_cast<int>(elapsedMilli_ / FrameDurationMilli)) % FrameCount;
    elapsedMilli_ %= FrameDurationMilli;
    updateText();
}

bool Spinner::render(C3D_RenderTarget& target) const {
    (void)target;
    C2D_DrawText(&text_, C2D_WithColor, position_.x, position_.y, 0.0f, scale_, scale_,
                 color_);
    return true;
}

void Spinner::updateText() {
    if (!textBuffer_) {
        return;
    }

    C2D_TextBufClear(textBuffer_);
    C2D_TextParse(&text_, textBuffer_, glyphForFrame(frame_));
    C2D_TextOptimize(&text_);
}

} // namespace components
} // namespace ui
} // namespace lunar3d
