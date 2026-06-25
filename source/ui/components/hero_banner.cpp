#include "ui/components/hero_banner.hpp"

#include "ui/style/colors.hpp"
#include "ui/style/typography.hpp"

namespace lunar3d {
namespace ui {
namespace components {

HeroBanner::HeroBanner(Rect bounds, const char* message) : bounds_(bounds) {
    setMessage(message);
}

HeroBanner::~HeroBanner() {
    C2D_TextBufDelete(textBuffer_);
}

void HeroBanner::setMessage(const char* message) {
    C2D_TextBufClear(textBuffer_);
    C2D_TextParse(&message_, textBuffer_, message);
    C2D_TextOptimize(&message_);
}

void HeroBanner::setBackgroundColor(u32 color) {
    backgroundColor_ = color;
}

bool HeroBanner::render(C3D_RenderTarget& target) const {
    (void)target;

    if (backgroundColor_ != style::colors::Transparent) {
        C2D_DrawRectSolid(bounds_.origin.x, bounds_.origin.y, 0.0f, bounds_.size.width,
                          bounds_.size.height, backgroundColor_);
    }

    float messageWidth = 0.0f;
    float messageHeight = 0.0f;
    C2D_TextGetDimensions(&message_, style::typography::TitleScale, style::typography::TitleScale,
                          &messageWidth, &messageHeight);

    const float centerX = bounds_.origin.x + bounds_.size.width * 0.5f;
    const float centerY = bounds_.origin.y + bounds_.size.height * 0.5f;
    const float messageX = centerX - messageWidth * 0.5f;
    const float messageY = centerY - messageHeight * 0.5f;

    C2D_DrawText(&message_, C2D_WithColor, messageX, messageY, 0.0f, style::typography::TitleScale,
                 style::typography::TitleScale, style::colors::Text);
    return true;
}

} // namespace components
} // namespace ui
} // namespace lunar3d
