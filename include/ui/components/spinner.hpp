#pragma once

#include "ui/component.hpp"
#include "ui/point.hpp"

#include <citro2d.h>

namespace lunar3d {
namespace ui {
namespace components {

class Spinner : public Component {
  public:
    Spinner();
    Spinner(Point position, float scale, u32 color);
    ~Spinner() override;

    void setPosition(Point position);
    void setScale(float scale);
    void setColor(u32 color);
    void reset();
    void update(u64 deltaMilli) override;
    bool render(C3D_RenderTarget& target) const override;

  private:
    void updateText();

    C2D_TextBuf textBuffer_ = C2D_TextBufNew(16);
    C2D_Text text_ = {};
    Point position_;
    float scale_ = 0.5f;
    u32 color_ = 0;
    u64 elapsedMilli_ = 0;
    int frame_ = 0;
};

} // namespace components
} // namespace ui
} // namespace lunar3d
