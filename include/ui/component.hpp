#pragma once

#include "ui/circle_pad_event.hpp"
#include "ui/key_event.hpp"
#include "ui/touch_event.hpp"

#include <citro2d.h>

namespace lunar3d {
namespace ui {

class Component {
  public:
    virtual ~Component() {}

    void setVisible(bool visible);
    bool visible() const;

    virtual bool dispatchKeyEvent(const KeyEvent& event);
    virtual bool dispatchTouchEvent(const TouchEvent& event);
    virtual bool dispatchCirclePadEvent(const CirclePadEvent& event);
    virtual bool onKeyEvent(const KeyEvent& event);
    virtual bool onTouchEvent(const TouchEvent& event);
    virtual bool onCirclePadEvent(const CirclePadEvent& event);
    virtual void update(u64 deltaMilli);
    virtual bool render(C3D_RenderTarget& target) const = 0;

  private:
    bool visible_ = true;
};

} // namespace ui
} // namespace lunar3d
