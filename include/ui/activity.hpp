#pragma once

#include "ui/circle_pad_event.hpp"
#include "ui/component.hpp"
#include "ui/hid_input.hpp"
#include "ui/key_event.hpp"
#include "ui/touch_event.hpp"

#include <3ds/types.h>

namespace lunar3d {
namespace ui {

class Activity {
  public:
    static constexpr int MaxComponents = 16;

    virtual ~Activity() {}

    virtual void onCreate();
    virtual void onStart();
    virtual void onRestart();
    virtual void onResume();
    virtual void onPause();
    virtual void onStop();
    virtual void onDestroy();
    void finish();
    bool isFinished() const;
    void clearFinished();
    virtual bool handleHidInput(const HidInput& input);
    virtual bool dispatchKeyEvent(const KeyEvent& event);
    virtual bool dispatchTouchEvent(const TouchEvent& event);
    virtual bool dispatchCirclePadEvent(const CirclePadEvent& event);
    virtual bool onKeyEvent(const KeyEvent& event);
    virtual bool onTouchEvent(const TouchEvent& event);
    virtual bool onCirclePadEvent(const CirclePadEvent& event);
    virtual void update(u64 deltaMilli);
    virtual void render(C3D_RenderTarget& target) const;
    u32 backgroundColor(gfxScreen_t screen) const;

  protected:
    bool addComponent(gfxScreen_t screen, Component& component);
    void setBackgroundColor(gfxScreen_t screen, u32 color);
    void setBackgroundColors(u32 topColor, u32 bottomColor);

  private:
    struct ComponentEntry {
        gfxScreen_t screen = GFX_TOP;
        Component* component = nullptr;
    };

    bool dispatchKeyEvents(u32 keys, KeyAction action);
    bool dispatchComponents(const KeyEvent& event);
    bool dispatchComponents(const TouchEvent& event);
    bool dispatchComponents(const CirclePadEvent& event);
    ComponentEntry components_[MaxComponents];
    int componentCount_ = 0;
    u32 topBackgroundColor_ = C2D_Color32(0, 0, 0, 255);
    u32 bottomBackgroundColor_ = C2D_Color32(0, 0, 0, 255);
    bool finished_ = false;
    touchPosition lastTouch_ = {};
    circlePosition lastCirclePad_ = {};
    bool touching_ = false;
};

} // namespace ui
} // namespace lunar3d
