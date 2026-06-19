#include "ui/activity_manager.hpp"

namespace lunar3d {
namespace ui {

ActivityManager::ActivityManager() {}

ActivityManager::~ActivityManager() {
    destroyAll();
}

ActivityManager::Entry* ActivityManager::top() {
    if (depth_ == 0) {
        return nullptr;
    }

    return &stack_[depth_ - 1];
}

const ActivityManager::Entry* ActivityManager::top() const {
    if (depth_ == 0) {
        return nullptr;
    }

    return &stack_[depth_ - 1];
}

bool ActivityManager::launch(Activity& activity) {
    if (depth_ >= MaxActivityDepth) {
        return false;
    }

    if (active() == &activity) {
        return false;
    }

    stop();

    Entry& entry = stack_[depth_++];
    entry.activity = &activity;

    activity.clearFinished();
    activity.onCreate();
    activity.onStart();
    activity.onResume();
    entry.state = State::Running;

    return true;
}

bool ActivityManager::pause() {
    Entry* entry = top();
    if (entry == nullptr) {
        return false;
    }
    if (entry->state == State::Paused) {
        return true;
    }
    if (entry->state != State::Running) {
        return false;
    }

    entry->activity->onPause();
    entry->state = State::Paused;
    return true;
}

bool ActivityManager::stop() {
    Entry* entry = top();
    if (entry == nullptr) {
        return false;
    }
    if (entry->state == State::Stopped) {
        return true;
    }

    if (!pause()) { // to be stopped, we must first be paused
        return false;
    }

    entry->activity->onStop();
    entry->state = State::Stopped;
    return true;
}

bool ActivityManager::restart() {
    Entry* entry = top();
    if (entry == nullptr || entry->state != State::Stopped) {
        return false;
    }

    entry->activity->onRestart();
    entry->activity->onStart();
    entry->activity->onResume();
    entry->state = State::Running;
    return true;
}

bool ActivityManager::destroy() {
    Entry* entry = top();
    if (entry == nullptr) {
        return false;
    }
    if (!stop()) { // to be destroyed, we must first be stopped
        return false;
    }

    entry->activity->onDestroy();
    entry->activity = nullptr;
    entry->state = State::Empty;
    --depth_;

    return true;
}

void ActivityManager::destroyAll() {
    while (depth_ > 0) {
        destroy();
    }
}

Activity* ActivityManager::active() {
    Entry* entry = top();
    return entry == nullptr ? nullptr : entry->activity;
}

const Activity* ActivityManager::active() const {
    const Entry* entry = top();
    return entry == nullptr ? nullptr : entry->activity;
}

bool ActivityManager::update(const HidInput& input, u64 deltaMilli) {
    Activity* activity = active();
    if (activity == nullptr) {
        return false;
    }

    activity->handleHidInput(input);
    if (activity->isFinished()) {
        destroy();
        return restart(); // restart the next activity, if there is one
    }

    activity->update(deltaMilli);
    return true;
}

} // namespace ui
} // namespace lunar3d
