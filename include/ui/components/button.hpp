#pragma once

#include "ui/component.hpp"
#include "ui/rect.hpp"

#include <citro2d.h>
#include <functional>

namespace lunar3d {
namespace ui {
namespace components {

enum class ButtonRenderState {
    Normal,
    Pressed,
};

class Button : public Component {
    using OnClickListener = std::function<void()>;

  public:
    Button();
    Button(const char* label, Rect bounds);
    ~Button() override;

    void setLabel(const char* label);
    void setBounds(Rect bounds);
    void setAccent(u32 color);
    void setEnabled(bool enabled);
    bool enabled() const;
    void setRenderState(ButtonRenderState state);
    ButtonRenderState renderState() const;
    void setOnClickListener(OnClickListener listener);
    bool wasPressed() const;
    void clearPressed();

    bool onTouchEvent(const TouchEvent& event) override;
    bool render(C3D_RenderTarget& target) const override;

  private:
    C2D_TextBuf textBuffer_ = C2D_TextBufNew(64);
    C2D_Text label_;
    Rect bounds_;
    u32 accentColor_ = 0;
    ButtonRenderState renderState_ = ButtonRenderState::Normal;
    OnClickListener onClickListener_;
    bool pressed_ = false;
    bool trackingTouch_ = false;
    bool enabled_ = true;
};

} // namespace components
} // namespace ui
} // namespace lunar3d
