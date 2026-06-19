#include "ui/activity.hpp"

namespace lunar3d {
namespace ui {

void Activity::onCreate() {}

void Activity::onStart() {}

void Activity::onRestart() {}

void Activity::onResume() {}

void Activity::onPause() {}

void Activity::onStop() {}

void Activity::onDestroy() {}

void Activity::finish() {
    finished_ = true;
}

bool Activity::isFinished() const {
    return finished_;
}

void Activity::clearFinished() {
    finished_ = false;
}

bool Activity::handleHidInput(const HidInput& input) {
    bool handled = false;

    handled = dispatchKeyEvents(input.keysDown, KeyAction::Down) || handled;
    handled = dispatchKeyEvents(input.keysUp, KeyAction::Up) || handled;

    if (input.keysDown & KEY_TOUCH) {
        TouchEvent event;
        event.action = TouchAction::Down;
        event.x = input.touch.px;
        event.y = input.touch.py;
        handled = dispatchTouchEvent(event) || handled;

        lastTouch_ = input.touch;
        touching_ = true;
    } else if (input.keysHeld & KEY_TOUCH) {
        if (!touching_ || input.touch.px != lastTouch_.px || input.touch.py != lastTouch_.py) {
            TouchEvent event;
            event.action = touching_ ? TouchAction::Move : TouchAction::Down;
            event.x = input.touch.px;
            event.y = input.touch.py;
            handled = dispatchTouchEvent(event) || handled;

            lastTouch_ = input.touch;
            touching_ = true;
        }
    } else if (touching_ || (input.keysUp & KEY_TOUCH)) {
        TouchEvent event;
        event.action = TouchAction::Up;
        event.x = lastTouch_.px;
        event.y = lastTouch_.py;
        handled = dispatchTouchEvent(event) || handled;

        touching_ = false;
    }

    if (input.circlePad.dx != lastCirclePad_.dx || input.circlePad.dy != lastCirclePad_.dy) {
        CirclePadEvent event;
        event.x = input.circlePad.dx;
        event.y = input.circlePad.dy;
        handled = dispatchCirclePadEvent(event) || handled;

        lastCirclePad_ = input.circlePad;
    }

    return handled;
}

bool Activity::dispatchKeyEvent(const KeyEvent& event) {
    if (dispatchComponents(event)) {
        return true;
    }

    return onKeyEvent(event);
}

bool Activity::dispatchTouchEvent(const TouchEvent& event) {
    if (dispatchComponents(event)) {
        return true;
    }

    return onTouchEvent(event);
}

bool Activity::dispatchCirclePadEvent(const CirclePadEvent& event) {
    if (dispatchComponents(event)) {
        return true;
    }

    return onCirclePadEvent(event);
}

bool Activity::dispatchComponents(const KeyEvent& event) {
    for (int index = componentCount_ - 1; index >= 0; --index) {
        if (!components_[index].component->visible()) {
            continue;
        }

        if (components_[index].component->dispatchKeyEvent(event)) {
            return true;
        }
    }

    return false;
}

bool Activity::dispatchComponents(const TouchEvent& event) {
    for (int index = componentCount_ - 1; index >= 0; --index) {
        if (!components_[index].component->visible()) {
            continue;
        }

        if (components_[index].component->dispatchTouchEvent(event)) {
            return true;
        }
    }

    return false;
}

bool Activity::dispatchComponents(const CirclePadEvent& event) {
    for (int index = componentCount_ - 1; index >= 0; --index) {
        if (!components_[index].component->visible()) {
            continue;
        }

        if (components_[index].component->dispatchCirclePadEvent(event)) {
            return true;
        }
    }

    return false;
}

bool Activity::addComponent(gfxScreen_t screen, Component& component) {
    if (componentCount_ >= MaxComponents) {
        return false;
    }

    for (int index = 0; index < componentCount_; ++index) {
        if (components_[index].component == &component) {
            return false;
        }
    }

    components_[componentCount_].screen = screen;
    components_[componentCount_].component = &component;
    ++componentCount_;
    return true;
}

u32 Activity::backgroundColor(gfxScreen_t screen) const {
    return screen == GFX_BOTTOM ? bottomBackgroundColor_ : topBackgroundColor_;
}

void Activity::setBackgroundColor(gfxScreen_t screen, u32 color) {
    if (screen == GFX_BOTTOM) {
        bottomBackgroundColor_ = color;
        return;
    }

    topBackgroundColor_ = color;
}

void Activity::setBackgroundColors(u32 topColor, u32 bottomColor) {
    topBackgroundColor_ = topColor;
    bottomBackgroundColor_ = bottomColor;
}

bool Activity::onKeyEvent(const KeyEvent& event) {
    (void)event;
    return false;
}

bool Activity::onTouchEvent(const TouchEvent& event) {
    (void)event;
    return false;
}

bool Activity::onCirclePadEvent(const CirclePadEvent& event) {
    (void)event;
    return false;
}

bool Activity::dispatchKeyEvents(u32 keys, KeyAction action) {
    bool handled = false;
    for (u32 bit = 0; bit < 32; ++bit) {
        const u32 key = 1u << bit;
        if ((keys & key) == 0) {
            continue;
        }

        KeyEvent event;
        event.action = action;
        event.key = key;
        handled = dispatchKeyEvent(event) || handled;
    }

    return handled;
}

void Activity::update(u64 deltaMilli) {
    for (int index = 0; index < componentCount_; ++index) {
        components_[index].component->update(deltaMilli);
    }
}

void Activity::render(C3D_RenderTarget& target) const {
    for (int index = 0; index < componentCount_; ++index) {
        if (!components_[index].component->visible()) {
            continue;
        }

        if (components_[index].screen != target.screen) {
            continue;
        }
        components_[index].component->render(target);
    }
}

} // namespace ui
} // namespace lunar3d
