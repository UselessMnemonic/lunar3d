#pragma once

#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/metrics.hpp"
#include "ui/style/typography.hpp"

#include <citro2d.h>

#include <cstddef>
#include <functional>
#include <string>

namespace lunar3d {
namespace ui {
namespace components {

template <typename T> class StringListItemRenderer {
  public:
    typedef std::function<void(size_t, const T&, std::string&)> TextProvider;

    StringListItemRenderer();
    explicit StringListItemRenderer(TextProvider textProvider);
    StringListItemRenderer(const StringListItemRenderer& other) = delete;
    StringListItemRenderer& operator=(const StringListItemRenderer& other) = delete;
    ~StringListItemRenderer();

    void setTextProvider(TextProvider textProvider);
    void setTextScale(float scale);
    void setTextColor(u32 color);
    void setColors(u32 backgroundColor, u32 rowColor, u32 selectedRowColor);
    void setSelectedColor(u32 color);
    void renderList(C3D_RenderTarget& target, Rect bounds) const;
    void renderItem(C3D_RenderTarget& target, Rect bounds, size_t position, const T& item,
                    bool selected) const;

  private:
    C2D_TextBuf textBuffer_ = C2D_TextBufNew(512);
    TextProvider textProvider_;
    mutable std::string textScratch_;
    float textScale_ = style::typography::ListItemScale;
    u32 textColor_ = style::colors::Text;
    u32 backgroundColor_ = style::colors::TopBackground;
    u32 rowColor_ = style::colors::Panel;
    u32 selectedRowColor_ = style::colors::PanelAccent;
};

template <typename T> StringListItemRenderer<T>::StringListItemRenderer() {}

template <typename T>
StringListItemRenderer<T>::StringListItemRenderer(TextProvider textProvider)
    : textProvider_(textProvider) {}

template <typename T> StringListItemRenderer<T>::~StringListItemRenderer() {
    C2D_TextBufDelete(textBuffer_);
}

template <typename T> void StringListItemRenderer<T>::setTextProvider(TextProvider textProvider) {
    textProvider_ = textProvider;
}

template <typename T> void StringListItemRenderer<T>::setTextScale(float scale) {
    textScale_ = scale;
}

template <typename T> void StringListItemRenderer<T>::setTextColor(u32 color) {
    textColor_ = color;
}

template <typename T>
void StringListItemRenderer<T>::setColors(u32 backgroundColor, u32 rowColor, u32 selectedRowColor) {
    backgroundColor_ = backgroundColor;
    rowColor_ = rowColor;
    selectedRowColor_ = selectedRowColor;
}

template <typename T> void StringListItemRenderer<T>::setSelectedColor(u32 color) {
    selectedRowColor_ = color;
}

template <typename T>
void StringListItemRenderer<T>::renderList(C3D_RenderTarget& target, Rect bounds) const {
    (void)target;
    if (textBuffer_) {
        C2D_TextBufClear(textBuffer_);
    }

    C2D_DrawRectSolid(bounds.origin.x, bounds.origin.y, 0.0f, bounds.size.width, bounds.size.height,
                      backgroundColor_);
}

template <typename T>
void StringListItemRenderer<T>::renderItem(C3D_RenderTarget& target, Rect bounds, size_t position,
                                           const T& item, bool selected) const {
    (void)target;

    const u32 rowColor = selected ? selectedRowColor_ : rowColor_;
    C2D_DrawRectSolid(bounds.origin.x, bounds.origin.y, 0.0f, bounds.size.width, bounds.size.height,
                      rowColor);

    if (!textBuffer_ || !textProvider_) {
        return;
    }

    textScratch_.clear();
    textProvider_(position, item, textScratch_);
    C2D_Text text;
    C2D_TextParse(&text, textBuffer_, textScratch_.c_str());
    C2D_TextOptimize(&text);
    C2D_DrawText(&text, C2D_WithColor, bounds.origin.x + style::metrics::ListTextInset,
                 bounds.origin.y + style::metrics::ListTextInset, 0.0f, textScale_, textScale_,
                 textColor_);
}

} // namespace components
} // namespace ui
} // namespace lunar3d
