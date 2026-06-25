#pragma once

#include "ui/component.hpp"
#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/metrics.hpp"
#include "ui/style/typography.hpp"

#include <citro2d.h>
#include <functional>
#include <string>

namespace lunar3d {
namespace ui {
namespace components {

template <size_t MaxUtf16 = 32> class InputField : public Component {

    using OnValueChangeListener = std::function<void(std::string&)>;

  public:
    InputField();
    InputField(Rect bounds, const char* hint);
    ~InputField() override;

    void setBounds(Rect bounds);
    void setHint(const char* hint);
    void setText(const char* text);
    void setEnabled(bool enabled);
    void setOnValueChangeListener(OnValueChangeListener listener);
    bool enabled() const;
    const std::string& data() const;
    bool onTouchEvent(const TouchEvent& event) override;
    bool render(C3D_RenderTarget& target) const override;

  private:
    bool openKeyboard();
    void updateDisplayText();

    C2D_TextBuf textBuffer_ = C2D_TextBufNew(MaxUtf16);
    C2D_Text displayText_ = {};
    Rect bounds_;
    std::string text_;
    std::string hint_;
    bool enabled_ = true;
    OnValueChangeListener onValueChangeListener_;
};

template <size_t MaxUtf16> InputField<MaxUtf16>::InputField() {}

template <size_t MaxUtf16>
InputField<MaxUtf16>::InputField(Rect bounds, const char* hint) : bounds_(bounds), hint_(hint) {
    updateDisplayText();
}

template <size_t MaxUtf16> InputField<MaxUtf16>::~InputField() {
    C2D_TextBufDelete(textBuffer_);
}

template <size_t MaxUtf16> void InputField<MaxUtf16>::setBounds(Rect bounds) {
    bounds_ = bounds;
}

template <size_t MaxUtf16> void InputField<MaxUtf16>::setHint(const char* hint) {
    hint_ = hint;
    updateDisplayText();
}

template <size_t MaxUtf16> void InputField<MaxUtf16>::setText(const char* text) {
    text_ = text;
    updateDisplayText();
}

template <size_t MaxUtf16> void InputField<MaxUtf16>::setEnabled(bool enabled) {
    enabled_ = enabled;
}

template <size_t MaxUtf16>
void InputField<MaxUtf16>::setOnValueChangeListener(OnValueChangeListener listener) {
    onValueChangeListener_ = listener;
}

template <size_t MaxUtf16> bool InputField<MaxUtf16>::enabled() const {
    return enabled_;
}

template <size_t MaxUtf16> const std::string& InputField<MaxUtf16>::data() const {
    return text_;
}

template <size_t MaxUtf16> bool InputField<MaxUtf16>::onTouchEvent(const TouchEvent& event) {
    if (!enabled_) {
        return false;
    }

    if (event.action != TouchAction::Up) {
        return bounds_.contains(Point(static_cast<float>(event.x), static_cast<float>(event.y)));
    }

    if (!bounds_.contains(Point(static_cast<float>(event.x), static_cast<float>(event.y)))) {
        return false;
    }

    return openKeyboard();
}

template <size_t MaxUtf16> bool InputField<MaxUtf16>::render(C3D_RenderTarget& target) const {
    (void)target;

    const u32 panelColor = enabled_ ? style::colors::Panel : style::colors::PanelDisabled;
    const u32 accentColor = enabled_ ? style::colors::Accent : style::colors::AccentDisabled;
    C2D_DrawRectSolid(bounds_.origin.x, bounds_.origin.y, 0.0f, bounds_.size.width,
                      bounds_.size.height, panelColor);
    C2D_DrawRectSolid(bounds_.origin.x, bounds_.origin.y + bounds_.size.height - 3.0f, 0.0f,
                      bounds_.size.width, style::metrics::AccentRule, accentColor);

    const u32 textColor =
        !enabled_ || text_.empty() ? style::colors::HintText : style::colors::Text;
    C2D_DrawText(&displayText_, C2D_WithColor, bounds_.origin.x + style::metrics::ControlTextInsetX,
                 bounds_.origin.y + style::metrics::ControlTextInsetY, 0.0f,
                 style::typography::ControlScale, style::typography::ControlScale, textColor);
    return true;
}

template <size_t MaxUtf16> bool InputField<MaxUtf16>::openKeyboard() {
    static_assert(MaxUtf16 > 0);
    constexpr size_t BufferSize = MaxUtf16 * 3 + 1;
    std::array<char, BufferSize> buffer{};

    SwkbdState keyboard;
    swkbdInit(&keyboard, SWKBD_TYPE_WESTERN, 2, MaxUtf16);
    swkbdSetValidation(&keyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetHintText(&keyboard, hint_.c_str());
    swkbdSetInitialText(&keyboard, text_.c_str());
    swkbdSetButton(&keyboard, SWKBD_BUTTON_LEFT, "Cancel", false);
    swkbdSetButton(&keyboard, SWKBD_BUTTON_RIGHT, "OK", true);

    const SwkbdButton button = swkbdInputText(&keyboard, buffer.data(), buffer.size());
    if (button != SWKBD_BUTTON_RIGHT) {
        return false;
    }

    text_.assign(buffer.data(), buffer.size());
    if (onValueChangeListener_) {
        onValueChangeListener_(text_);
    }
    updateDisplayText();
    return true;
}

template <size_t MaxUtf16> void InputField<MaxUtf16>::updateDisplayText() {
    if (!textBuffer_) {
        return;
    }

    C2D_TextBufClear(textBuffer_);
    C2D_TextParse(&displayText_, textBuffer_, text_.empty() ? hint_.c_str() : text_.c_str());
    C2D_TextOptimize(&displayText_);
}

} // namespace components
} // namespace ui
} // namespace lunar3d
