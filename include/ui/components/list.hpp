#pragma once

#include "ui/component.hpp"
#include "ui/components/iterator_list_source.hpp"
#include "ui/components/string_list_item_renderer.hpp"
#include "ui/rect.hpp"

#include <3ds.h>
#include <citro2d.h>

#include <cstddef>
#include <functional>

namespace lunar3d {
namespace ui {
namespace components {

template <typename T, typename Source = IteratorListSource<T>,
          typename Renderer = StringListItemRenderer<T>>
class List : public Component {
  public:
    typedef std::function<void(size_t, const T&)> ItemSelectedListener;

    List();
    explicit List(Rect bounds);
    List(Rect bounds, const Source& source);

    void setBounds(Rect bounds);
    Source& itemSource();
    const Source& itemSource() const;
    Renderer& itemRenderer();
    const Renderer& itemRenderer() const;
    void setOnItemSelectedListener(ItemSelectedListener listener);
    void setRowHeight(float rowHeight);
    size_t getCount() const;
    const T* getItem(size_t position) const;
    int getSelectedItemPosition() const;
    bool setSelectedIndex(size_t index);

    bool onKeyEvent(const KeyEvent& event) override;
    bool render(C3D_RenderTarget& target) const override;

  private:
    size_t visibleRowCount() const;
    bool navigateBy(int delta);
    bool selectIndex(size_t index);
    void ensureSelectedVisible();
    void clampSelectedIndex();

    Rect bounds_;
    Source source_;
    Renderer renderer_;
    ItemSelectedListener onItemSelectedListener_;
    float rowHeight_ = 32.0f;
    size_t firstVisibleIndex_ = 0;
    int selectedIndex_ = -1;
};

template <typename T, typename Source, typename Renderer> List<T, Source, Renderer>::List() {}

template <typename T, typename Source, typename Renderer>
List<T, Source, Renderer>::List(Rect bounds) : bounds_(bounds) {}

template <typename T, typename Source, typename Renderer>
List<T, Source, Renderer>::List(Rect bounds, const Source& source)
    : bounds_(bounds), source_(source) {
    clampSelectedIndex();
    ensureSelectedVisible();
}

template <typename T, typename Source, typename Renderer>
void List<T, Source, Renderer>::setBounds(Rect bounds) {
    bounds_ = bounds;
    ensureSelectedVisible();
}

template <typename T, typename Source, typename Renderer>
Source& List<T, Source, Renderer>::itemSource() {
    return source_;
}

template <typename T, typename Source, typename Renderer>
const Source& List<T, Source, Renderer>::itemSource() const {
    return source_;
}

template <typename T, typename Source, typename Renderer>
Renderer& List<T, Source, Renderer>::itemRenderer() {
    return renderer_;
}

template <typename T, typename Source, typename Renderer>
const Renderer& List<T, Source, Renderer>::itemRenderer() const {
    return renderer_;
}

template <typename T, typename Source, typename Renderer>
void List<T, Source, Renderer>::setOnItemSelectedListener(ItemSelectedListener listener) {
    onItemSelectedListener_ = listener;
}

template <typename T, typename Source, typename Renderer>
void List<T, Source, Renderer>::setRowHeight(float rowHeight) {
    rowHeight_ = rowHeight;
    ensureSelectedVisible();
}

template <typename T, typename Source, typename Renderer>
size_t List<T, Source, Renderer>::getCount() const {
    return source_.getCount();
}

template <typename T, typename Source, typename Renderer>
const T* List<T, Source, Renderer>::getItem(size_t position) const {
    return source_.getItem(position);
}

template <typename T, typename Source, typename Renderer>
int List<T, Source, Renderer>::getSelectedItemPosition() const {
    return selectedIndex_;
}

template <typename T, typename Source, typename Renderer>
bool List<T, Source, Renderer>::setSelectedIndex(size_t index) {
    return selectIndex(index);
}

template <typename T, typename Source, typename Renderer>
bool List<T, Source, Renderer>::onKeyEvent(const KeyEvent& event) {
    if (event.action != KeyAction::Down) {
        return false;
    }

    if (event.key == KEY_DUP) {
        return navigateBy(-1);
    }

    if (event.key == KEY_DDOWN) {
        return navigateBy(1);
    }

    return false;
}

template <typename T, typename Source, typename Renderer>
bool List<T, Source, Renderer>::render(C3D_RenderTarget& target) const {
    renderer_.renderList(target, bounds_);

    const size_t visibleRows = visibleRowCount();
    const size_t count = getCount();
    const size_t remainingRows = firstVisibleIndex_ < count ? count - firstVisibleIndex_ : 0;
    const size_t rowCount = remainingRows < visibleRows ? remainingRows : visibleRows;
    for (size_t i = 0; i < rowCount; i++) {
        const size_t itemIndex = firstVisibleIndex_ + i;
        const T* item = getItem(itemIndex);
        if (!item) {
            continue;
        }

        Rect rowBounds(bounds_.origin.x, bounds_.origin.y + static_cast<float>(i) * rowHeight_,
                       bounds_.size.width, rowHeight_);
        const bool selected = static_cast<int>(itemIndex) == selectedIndex_;
        renderer_.renderItem(target, rowBounds, itemIndex, *item, selected);
    }

    return true;
}

template <typename T, typename Source, typename Renderer>
size_t List<T, Source, Renderer>::visibleRowCount() const {
    return rowHeight_ > 0.0f ? static_cast<size_t>(bounds_.size.height / rowHeight_) : 0;
}

template <typename T, typename Source, typename Renderer>
bool List<T, Source, Renderer>::navigateBy(int delta) {
    const size_t count = getCount();
    if (count == 0) {
        return false;
    }

    int nextIndex = selectedIndex_;
    if (nextIndex < 0) {
        nextIndex = delta < 0 ? static_cast<int>(count - 1) : 0;
    } else {
        nextIndex += delta;
        if (nextIndex < 0) {
            nextIndex = 0;
        } else if (static_cast<size_t>(nextIndex) >= count) {
            nextIndex = static_cast<int>(count - 1);
        }
    }

    return selectIndex(static_cast<size_t>(nextIndex));
}

template <typename T, typename Source, typename Renderer>
bool List<T, Source, Renderer>::selectIndex(size_t index) {
    const T* item = getItem(index);
    if (!item) {
        return false;
    }

    if (selectedIndex_ == static_cast<int>(index)) {
        return true;
    }

    selectedIndex_ = static_cast<int>(index);
    ensureSelectedVisible();

    if (onItemSelectedListener_) {
        onItemSelectedListener_(index, *item);
    }

    return true;
}

template <typename T, typename Source, typename Renderer>
void List<T, Source, Renderer>::ensureSelectedVisible() {
    if (selectedIndex_ < 0) {
        firstVisibleIndex_ = 0;
        return;
    }

    const size_t visibleRows = visibleRowCount();
    if (visibleRows == 0) {
        firstVisibleIndex_ = 0;
        return;
    }

    const size_t index = static_cast<size_t>(selectedIndex_);
    if (index < firstVisibleIndex_) {
        firstVisibleIndex_ = index;
    } else if (index >= firstVisibleIndex_ + visibleRows) {
        firstVisibleIndex_ = index - visibleRows + 1;
    }
}

template <typename T, typename Source, typename Renderer>
void List<T, Source, Renderer>::clampSelectedIndex() {
    const size_t count = getCount();
    if (count == 0) {
        selectedIndex_ = -1;
        return;
    }

    if (selectedIndex_ >= 0 && static_cast<size_t>(selectedIndex_) >= count) {
        selectedIndex_ = static_cast<int>(count - 1);
    }
}

} // namespace components
} // namespace ui
} // namespace lunar3d
