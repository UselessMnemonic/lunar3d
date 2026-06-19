#include "activities/main_activity.hpp"

#include "ui/glyphs.hpp"
#include "ui/point.hpp"
#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/metrics.hpp"
#include "ui/style/typography.hpp"

#include <cstddef>
#include <cstdio>

namespace lunar3d {
namespace activities {

namespace {

constexpr const char* IdentityDirectory = "sdmc:/3ds/lunar3d/identity";
constexpr const char* HostListDirectory = "sdmc:/3ds/lunar3d/hosts";
const ui::Rect TopContentBounds(16.0f, 48.0f, 368.0f, 152.0f);

} // namespace

MainActivity::MainActivity(ui::ActivityManager& activityManager)
    : activityManager_(activityManager), hostList_(HostListDirectory),
      pairActivity_(identity_, hostList_), deleteActivity_(hostList_),
      headerLabel_("Lunar3D", ui::Point(8.0f, 8.0f), ui::style::typography::TitleScale,
                   ui::style::colors::Text),
      updateLabel_("Update: 0 ms", ui::Point(8.0f, 8.0f), ui::style::typography::BodyScale,
                   ui::style::colors::Text),
      footerLabel_("", ui::Point(16.0f, 212.0f), ui::style::typography::BodyScale,
                   ui::style::colors::HintText),
      heroBanner_(TopContentBounds, "No hosts paired") {
    setBackgroundColors(ui::style::colors::TopBackground, ui::style::colors::BottomBackground);

    const moonlight::ClientIdentityResult identityResult =
        moonlight::ClientIdentity::load(IdentityDirectory, identity_);
    if (identityResult != moonlight::ClientIdentityResult::Ok) {
        char message[96];
        std::snprintf(message, sizeof(message), "Could not load identity: %s",
                      moonlight::toString(identityResult));
        heroBanner_.setMessage(message);
        addComponent(GFX_TOP, heroBanner_);
        return;
    }

    const moonlight::HostListResult hostListResult = hostList_.load();
    if (hostListResult != moonlight::HostListResult::Ok) {
        char message[96];
        std::snprintf(message, sizeof(message), "Could not load hosts: %s",
                      moonlight::toString(hostListResult));
        heroBanner_.setMessage(message);
        addComponent(GFX_TOP, heroBanner_);
        return;
    }

    hostListView_.setBounds(TopContentBounds);
    hostListView_.itemRenderer().setTextProvider(
        [](size_t, const moonlight::Host& host, std::string& text) {
            text = host.hostname.empty() ? host.address : host.hostname;
        });
    refreshHostList();
    hostListView_.setSelectedIndex(0);

    std::string footerMessage;
    footerMessage += ui::glyphs::ButtonA;
    footerMessage += " Connect   ";
    footerMessage += ui::glyphs::ButtonX;
    footerMessage += " Delete   ";
    footerMessage += ui::glyphs::ButtonY;
    footerMessage += " Pair";
    footerLabel_.setText(footerMessage.c_str());

    addComponent(GFX_TOP, headerLabel_);
    addComponent(GFX_TOP, hostListView_);
    addComponent(GFX_TOP, heroBanner_);
    addComponent(GFX_TOP, footerLabel_);
    addComponent(GFX_BOTTOM, updateLabel_);
    ready_ = true;
}

void MainActivity::refreshHostList() {
    hostListView_.itemSource().setItems(hostList_.begin(), hostList_.end());
    hostListView_.setVisible(!hostList_.empty());
    heroBanner_.setVisible(hostList_.empty());

    if (hostList_.empty()) {
        return;
    }

    int selectedIndex = hostListView_.getSelectedItemPosition();
    if (selectedIndex < 0) {
        selectedIndex = 0;
    } else if (static_cast<size_t>(selectedIndex) >= hostList_.size()) {
        selectedIndex = static_cast<int>(hostList_.size() - 1);
    }
    hostListView_.setSelectedIndex(static_cast<size_t>(selectedIndex));
}

void MainActivity::onRestart() {
    if (!ready_) {
        return;
    }

    refreshHostList();
}

int MainActivity::selectedHostIndex() const {
    const int index = hostListView_.getSelectedItemPosition();
    if (index < 0 || static_cast<size_t>(index) >= hostList_.size()) {
        return -1;
    }

    return index;
}

bool MainActivity::onKeyEvent(const ui::KeyEvent& event) {
    if (!ready_) {
        return Activity::onKeyEvent(event);
    }

    if (event.action == ui::KeyAction::Down && event.key == KEY_Y) {
        activityManager_.launch(pairActivity_);
        return true;
    }

    if (event.action == ui::KeyAction::Down && event.key == KEY_X) {
        const int index = selectedHostIndex();
        if (index >= 0) {
            deleteActivity_.setIndex(static_cast<size_t>(index));
            activityManager_.launch(deleteActivity_);
        }
        return true;
    }

    return Activity::onKeyEvent(event);
}

void MainActivity::update(u64 deltaMilli) {
    Activity::update(deltaMilli);

    char updateText[32];
    snprintf(updateText, sizeof(updateText), "Update: %llu ms",
             static_cast<unsigned long long>(deltaMilli));
    updateLabel_.setText(updateText);
}

} // namespace activities
} // namespace lunar3d
