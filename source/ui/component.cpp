#include "ui/component.hpp"

namespace lunar3d {
namespace ui {

void Component::setVisible(bool visible) {
    visible_ = visible;
}

bool Component::visible() const {
    return visible_;
}

bool Component::dispatchKeyEvent(const KeyEvent& event) {
    return onKeyEvent(event);
}

bool Component::dispatchTouchEvent(const TouchEvent& event) {
    return onTouchEvent(event);
}

bool Component::dispatchCirclePadEvent(const CirclePadEvent& event) {
    return onCirclePadEvent(event);
}

bool Component::onKeyEvent(const KeyEvent& event) {
    (void)event;
    return false;
}

bool Component::onTouchEvent(const TouchEvent& event) {
    (void)event;
    return false;
}

bool Component::onCirclePadEvent(const CirclePadEvent& event) {
    (void)event;
    return false;
}

void Component::update(u64 deltaMilli) {
    (void)deltaMilli;
}

} // namespace ui
} // namespace lunar3d
