#include "activities/stream_activity.hpp"

#include "ui/glyphs.hpp"
#include "ui/point.hpp"
#include "ui/rect.hpp"
#include "ui/style/colors.hpp"
#include "ui/style/typography.hpp"

#include <cstdio>

namespace lunar3d {
namespace activities {

StreamActivity::StreamActivity(const moonlight::ClientIdentity& identity)
    : identity_(identity),
      overlayBanner_(ui::Rect(0.0f, 0.0f, 400.0f, 240.0f), "Starting..."),
      exitButton_("Exit Stream", ui::Rect(100.0f, 100.0f, 120.0f, 40.0f)) {
    setBackgroundColors(ui::style::colors::TopBackground, ui::style::colors::BottomBackground);

    // Dark slate gray with ~78% opacity (200/255)
    overlayBanner_.setBackgroundColor(C2D_Color32(17, 21, 27, 200));

    exitButton_.setOnClickListener([this]() {
        finish();
    });

    addComponent(GFX_TOP, overlayBanner_);
    addComponent(GFX_BOTTOM, exitButton_);
}

void StreamActivity::setHostAndApp(const moonlight::Host& host, const moonlight::GameStreamApp& app) {
    host_ = &host;
    app_ = app;
}

void StreamActivity::onStart() {
    if (!host_) {
        overlayBanner_.setMessage("No host configured");
        overlayBanner_.setVisible(true);
        return;
    }

    overlayBanner_.setMessage("Starting...");
    overlayBanner_.setVisible(true);

    streamWorker_ = task::launch(task::StreamTask(*host_, app_));
    if (!streamWorker_) {
        overlayBanner_.setMessage("Failed to start thread");
    }
}

void StreamActivity::onStop() {
    overlayBanner_.setMessage("Stopping...");
    overlayBanner_.setVisible(true);

    LiStopConnection();
    if (streamWorker_) {
        streamWorker_->join();
        streamWorker_.reset();
    }
}

} // namespace activities
} // namespace lunar3d
