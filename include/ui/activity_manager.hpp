#pragma once

#include "ui/activity.hpp"

namespace lunar3d {
namespace ui {

class ActivityManager {
  public:
    static constexpr int MaxActivityDepth = 8;

    ActivityManager();
    ~ActivityManager();

    bool launch(Activity& activity);
    bool pause();
    bool stop();
    bool restart();
    bool destroy();
    void destroyAll();
    bool update(const HidInput& input, u64 deltaMilli);

    Activity* active();
    const Activity* active() const;

  private:
    enum class State { Empty, Running, Paused, Stopped };

    struct Entry {
        Activity* activity = nullptr;
        State state = State::Empty;
    };

    Entry* top();
    const Entry* top() const;

    Entry stack_[MaxActivityDepth];
    int depth_ = 0;
};

} // namespace ui
} // namespace lunar3d
