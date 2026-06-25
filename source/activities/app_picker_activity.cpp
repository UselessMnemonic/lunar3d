#include "activities/app_picker_activity.hpp"

#include "ui/glyphs.hpp"
#include "ui/point.hpp"
#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/typography.hpp"

#include <cstdio>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace lunar3d {
namespace activities {
namespace {

const ui::Rect TopContentBounds(16.0f, 48.0f, 368.0f, 152.0f);

std::string hostDisplayName(const moonlight::Host& host) {
    return host.hostname.empty() ? host.address : host.hostname;
}

} // namespace

AppPickerActivity::AppPickerActivity(const moonlight::ClientIdentity& identity,
                                     ui::ActivityManager& activityManager)
    : identity_(identity), activityManager_(activityManager), streamActivity_(identity),
      titleLabel_("Apps", ui::Point(8.0f, 8.0f), ui::style::typography::TitleScale,
                  ui::style::colors::Text),
      footerLabel_("", ui::Point(16.0f, 212.0f), ui::style::typography::BodyScale,
                   ui::style::colors::HintText),
      heroBanner_(TopContentBounds, "Loading apps...") {
    setBackgroundColors(ui::style::colors::TopBackground, ui::style::colors::BottomBackground);

    appListView_.setBounds(TopContentBounds);
    appListView_.itemRenderer().setTextProvider(
        [](size_t, const moonlight::GameStreamApp& app, std::string& text) { text = app.title; });
    apps_.clear();
    appListView_.itemSource().setItems(apps_.begin(), apps_.end());
    appListView_.setVisible(false);
    setFooter();

    addComponent(GFX_TOP, titleLabel_);
    addComponent(GFX_TOP, appListView_);
    addComponent(GFX_TOP, heroBanner_);
    addComponent(GFX_TOP, footerLabel_);
}

void AppPickerActivity::setHost(const moonlight::Host& host) {
    host_ = &host;
}

void AppPickerActivity::onStart() {
    std::string title = "Apps";
    if (host_) {
        title += ": ";
        title += hostDisplayName(*host_);
    }
    titleLabel_.setText(title.c_str());

    apps_.clear();
    appListView_.itemSource().setItems(apps_.begin(), apps_.end());
    appListView_.setVisible(false);
    showMessage("Loading apps...");
    startAppListTask();
}

void AppPickerActivity::onStop() {
    shutdownAppListTask();
    apps_.clear();
    appListView_.itemSource().setItems(apps_.begin(), apps_.end());
    appListView_.setVisible(false);
    heroBanner_.setMessage("");
    titleLabel_.setText("");
}

bool AppPickerActivity::onKeyEvent(const ui::KeyEvent& event) {
    if (event.action == ui::KeyAction::Down && event.key == KEY_A) {
        const int index = appListView_.getSelectedItemPosition();
        if (index >= 0 && static_cast<size_t>(index) < apps_.size()) {
            const moonlight::GameStreamApp& app = apps_[index];
            if (host_) {
                streamActivity_.setHostAndApp(*host_, app);
                activityManager_.launch(streamActivity_);
            }
        }
        return true;
    }

    if (event.action == ui::KeyAction::Down && event.key == KEY_B) {
        finish();
        return true;
    }

    return Activity::onKeyEvent(event);
}

void AppPickerActivity::update(u64 deltaMilli) {
    Activity::update(deltaMilli);
    inspectAppListResult();
}

void AppPickerActivity::startAppListTask() {
    shutdownAppListTask();

    if (!host_) {
        showMessage("No host selected");
        return;
    }

    task::AppListRequest request{*host_, identity_};
    appListWorker_ = task::launch(task::AppListTask(request));
}

void AppPickerActivity::shutdownAppListTask() {
    if (!appListWorker_) {
        return;
    }

    task::AppListResult result = appListWorker_->join();
    (void)result;
    appListWorker_.reset();
}

void AppPickerActivity::inspectAppListResult() {
    if (!appListWorker_) {
        return;
    }

    std::optional<task::AppListResult> result = appListWorker_->tryJoin();
    if (!result) {
        return;
    }

    appListWorker_.reset();
    handleAppListResult(*result);
}

void AppPickerActivity::handleAppListResult(task::AppListResult& result) {
    if (task::AppListSuccess* success = std::get_if<task::AppListSuccess>(&result)) {
        showApps(std::move(success->apps));
        return;
    }

    if (std::get_if<task::AppListInvalidRequest>(&result)) {
        showMessage("No host selected");
        return;
    }

    if (const moonlight::NvResult* nvResult = std::get_if<moonlight::NvResult>(&result)) {
        char message[96];
        if (const moonlight::NvHttpStatusError* statusError =
                std::get_if<moonlight::NvHttpStatusError>(nvResult)) {
            std::snprintf(message, sizeof(message), "%s %lu", moonlight::toString(*nvResult),
                          static_cast<unsigned long>(statusError->statusCode));
        } else {
            std::snprintf(message, sizeof(message), "%s", moonlight::toString(*nvResult));
        }
        showMessage(message);
        return;
    }

    if (const moonlight::GameStreamResult* gameStreamResult =
            std::get_if<moonlight::GameStreamResult>(&result)) {
        char message[96];
        std::snprintf(message, sizeof(message), "Could not parse apps: %s",
                      moonlight::toString(*gameStreamResult));
        showMessage(message);
    }
}

void AppPickerActivity::showMessage(const char* message) {
    heroBanner_.setMessage(message);
    heroBanner_.setVisible(true);
    appListView_.setVisible(false);
}

void AppPickerActivity::showApps(std::vector<moonlight::GameStreamApp>&& apps) {
    apps_ = std::move(apps);
    appListView_.itemSource().setItems(apps_.begin(), apps_.end());
    if (apps_.empty()) {
        showMessage("No apps found");
        return;
    }

    appListView_.setSelectedIndex(0);
    appListView_.setVisible(true);
    heroBanner_.setVisible(false);
}

void AppPickerActivity::setFooter() {
    std::string footerMessage;
    footerMessage += ui::glyphs::ButtonA;
    footerMessage += " Launch   ";
    footerMessage += ui::glyphs::ButtonB;
    footerMessage += " Return";
    footerLabel_.setText(footerMessage.c_str());
}

} // namespace activities
} // namespace lunar3d
