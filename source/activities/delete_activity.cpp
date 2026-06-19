#include "activities/delete_activity.hpp"

#include "ui/point.hpp"
#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/typography.hpp"

#include <cstdio>
#include <string>

namespace lunar3d {
namespace activities {
namespace {

std::string hostDisplayName(const moonlight::Host& host) {
    return host.hostname.empty() ? host.address : host.hostname;
}

} // namespace

DeleteActivity::DeleteActivity(moonlight::HostList& hostList)
    : hostList_(hostList), titleLabel_("Delete Host", ui::Point(8.0f, 8.0f),
                                       ui::style::typography::TitleScale, ui::style::colors::Text),
      promptLabel_("", ui::Point(24.0f, 56.0f), ui::style::typography::BodyScale,
                   ui::style::colors::Text),
      statusLabel_("", ui::Point(24.0f, 92.0f), ui::style::typography::BodyScale,
                   ui::style::colors::HintText),
      deleteButton_("Delete", ui::Rect(24.0f, 72.0f, 112.0f, 40.0f)),
      cancelButton_("Cancel", ui::Rect(184.0f, 72.0f, 112.0f, 40.0f)) {
    setBackgroundColors(ui::style::colors::TopBackground, ui::style::colors::BottomBackground);

    deleteButton_.setOnClickListener([this]() { confirmDelete(); });
    cancelButton_.setOnClickListener([this]() { finish(); });

    addComponent(GFX_TOP, titleLabel_);
    addComponent(GFX_TOP, promptLabel_);
    addComponent(GFX_TOP, statusLabel_);
    addComponent(GFX_BOTTOM, deleteButton_);
    addComponent(GFX_BOTTOM, cancelButton_);
}

void DeleteActivity::onStart() {
    deleteButton_.setEnabled(true);
    cancelButton_.setEnabled(true);
    statusLabel_.setText("");
    updatePrompt();
}

void DeleteActivity::setIndex(size_t index) {
    selectedHostIndex_ = index;
}

void DeleteActivity::confirmDelete() {
    const moonlight::HostListResult result = hostList_.remove(selectedHostIndex_);
    if (result == moonlight::HostListResult::InvalidData) {
        statusLabel_.setText("No host selected");
        deleteButton_.setEnabled(false);
        return;
    }
    if (result != moonlight::HostListResult::Ok) {
        char message[64];
        std::snprintf(message, sizeof(message), "Could not save hosts: %s",
                      moonlight::toString(result));
        statusLabel_.setText(message);
        deleteButton_.setEnabled(true);
        return;
    }

    finish();
}

void DeleteActivity::updatePrompt() {
    const moonlight::Host* host = hostList_.get(selectedHostIndex_);
    if (!host) {
        promptLabel_.setText("No host selected");
        deleteButton_.setEnabled(false);
        return;
    }

    std::string prompt("Delete ");
    prompt += hostDisplayName(*host);
    prompt += "?";
    promptLabel_.setText(prompt.c_str());
}

} // namespace activities
} // namespace lunar3d
