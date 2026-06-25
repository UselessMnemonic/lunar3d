#pragma once

#include "ui/component.hpp"
#include "ui/point.hpp"

#include <citro2d.h>

namespace lunar3d {
namespace ui {
namespace components {

class Label : public Component {
  public:
    Label();
    Label(const char* text, Point position, float scale, u32 color);
    ~Label() override;

    void setText(const char* text);
    void setPosition(Point position);
    void setScale(float scale);
    void setColor(u32 color);
    bool render(C3D_RenderTarget& target) const override;

  private:
    C2D_TextBuf textBuffer_ = C2D_TextBufNew(128);
    C2D_Text text_;
    Point position_;
    float scale_ = 0.5f;
    u32 color_ = 0;
};

} // namespace components
} // namespace ui
} // namespace lunar3d
