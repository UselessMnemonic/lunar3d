#include "ui/components/button.hpp"

#include "ui/style/colors.hpp"
#include "ui/style/metrics.hpp"
#include "ui/style/typography.hpp"

namespace lunar3d {
namespace ui {
namespace components {

Button::Button() {}

Button::Button(const char* label, Rect bounds) : bounds_(bounds) {
    accentColor_ = style::colors::Accent;
    setLabel(label);
}

Button::~Button() {
    C2D_TextBufDelete(textBuffer_);
}

void Button::setLabel(const char* label) {
    C2D_TextBufClear(textBuffer_);
    C2D_TextParse(&label_, textBuffer_, label);
    C2D_TextOptimize(&label_);
}

void Button::setBounds(Rect bounds) {
    bounds_ = bounds;
}

void Button::setAccent(u32 color) {
    accentColor_ = color;
}

void Button::setEnabled(bool enabled) {
    enabled_ = enabled;
    if (!enabled_) {
        renderState_ = ButtonRenderState::Normal;
        pressed_ = false;
        trackingTouch_ = false;
    }
}

bool Button::enabled() const {
    return enabled_;
}

void Button::setRenderState(ButtonRenderState state) {
    if (!enabled_) {
        renderState_ = ButtonRenderState::Normal;
        return;
    }

    renderState_ = state;
}

ButtonRenderState Button::renderState() const {
    return renderState_;
}

void Button::setOnClickListener(OnClickListener listener) {
    onClickListener_ = listener;
}

bool Button::wasPressed() const {
    return pressed_;
}

void Button::clearPressed() {
    pressed_ = false;
}

bool Button::onTouchEvent(const TouchEvent& event) {
    if (!enabled_) {
        return false;
    }

    const Point touchPoint(static_cast<float>(event.x), static_cast<float>(event.y));

    if (event.action == TouchAction::Down) {
        if (!bounds_.contains(touchPoint)) {
            return false;
        }

        trackingTouch_ = true;
        renderState_ = ButtonRenderState::Pressed;
        return true;
    }

    if (!trackingTouch_) {
        return false;
    }

    if (event.action == TouchAction::Move) {
        renderState_ =
            bounds_.contains(touchPoint) ? ButtonRenderState::Pressed : ButtonRenderState::Normal;
        return true;
    }

    if (event.action != TouchAction::Up) {
        return false;
    }

    const bool clicked = bounds_.contains(touchPoint);
    trackingTouch_ = false;
    renderState_ = ButtonRenderState::Normal;
    pressed_ = clicked;

    if (clicked) {
        setEnabled(false);
        if (onClickListener_) {
            onClickListener_();
        }
    }

    return true;
}

bool Button::render(C3D_RenderTarget& target) const {
    (void)target;
    u32 fill = style::colors::Panel;
    u32 accent = accentColor_;
    u32 textColor = style::colors::Text;
    if (!enabled_) {
        fill = style::colors::PanelDisabled;
        accent = style::colors::AccentDisabled;
        textColor = style::colors::HintText;
    } else if (renderState_ == ButtonRenderState::Pressed) {
        fill = style::colors::PanelPressed;
    }

    C2D_DrawRectSolid(bounds_.origin.x, bounds_.origin.y, 0.0f, bounds_.size.width,
                      bounds_.size.height, fill);
    C2D_DrawRectSolid(bounds_.origin.x, bounds_.origin.y, 0.0f, style::metrics::PanelAccentRule,
                      bounds_.size.height, accent);
    C2D_DrawText(&label_, C2D_WithColor, bounds_.origin.x + style::metrics::ButtonTextInsetX,
                 bounds_.origin.y + style::metrics::ButtonTextInsetY, 0.0f,
                 style::typography::ControlScale, style::typography::ControlScale, textColor);
    return true;
}

} // namespace components
} // namespace ui
} // namespace lunar3d
