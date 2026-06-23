#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/xml.hpp"
#include "task/stream_task.hpp"
#include "task/worker.hpp"
#include "ui/activity.hpp"
#include "ui/components/button.hpp"
#include "ui/components/hero_banner.hpp"

#include <citro2d.h>
#include <memory>

namespace lunar3d {
namespace activities {

class StreamActivity : public ui::Activity {
    using StreamWorker = task::Worker<void, task::StreamTask>;

  public:
    explicit StreamActivity(const moonlight::ClientIdentity& identity);

    void setHostAndApp(const moonlight::Host& host, const moonlight::GameStreamApp& app);

    void onStart() override;
    void onStop() override;

  private:
    const moonlight::ClientIdentity& identity_;
    const moonlight::Host* host_ = nullptr;
    moonlight::GameStreamApp app_;

    ui::components::HeroBanner overlayBanner_;
    ui::components::Button exitButton_;

    std::unique_ptr<StreamWorker> streamWorker_ = nullptr;
};

} // namespace activities
} // namespace lunar3d
