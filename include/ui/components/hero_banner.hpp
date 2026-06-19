#pragma once

#include "ui/component.hpp"
#include "ui/rect.hpp"

#include <citro2d.h>

namespace lunar3d {
namespace ui {
namespace components {

class HeroBanner : public Component {
public:
    HeroBanner(Rect bounds, const char* message);
    ~HeroBanner() override;

    void setMessage(const char* message);

    bool render(C3D_RenderTarget& target) const override;

private:
    C2D_TextBuf textBuffer_ = C2D_TextBufNew(256);
    C2D_Text message_;
    Rect bounds_;
};

} // namespace components
} // namespace ui
} // namespace lunar3d
