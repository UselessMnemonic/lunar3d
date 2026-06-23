#pragma once

#include "activities/stream_activity.hpp"
#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "task/app_list_task.hpp"
#include "task/worker.hpp"
#include "ui/activity.hpp"
#include "ui/activity_manager.hpp"
#include "ui/components/hero_banner.hpp"
#include "ui/components/label.hpp"
#include "ui/components/list.hpp"

#include <citro2d.h>

#include <string>
#include <vector>

namespace lunar3d {
namespace activities {

class AppPickerActivity : public ui::Activity {

  using AppListWorker = task::Worker<task::AppListResult, task::AppListTask>;

  public:
    AppPickerActivity(const moonlight::ClientIdentity& identity, ui::ActivityManager& activityManager);

    void setHost(const moonlight::Host& host);
    void onStart() override;
    void onStop() override;
    bool onKeyEvent(const ui::KeyEvent& event) override;
    void update(u64 deltaMilli) override;

  private:
    void startAppListTask();
    void shutdownAppListTask();
    void inspectAppListResult();
    void handleAppListResult(task::AppListResult& result);
    void showMessage(const char* message);
    void showApps(std::vector<moonlight::GameStreamApp>&& apps);
    void setFooter();

    const moonlight::ClientIdentity& identity_;
    ui::ActivityManager& activityManager_;
    StreamActivity streamActivity_;
    const moonlight::Host* host_ = nullptr;
    ui::components::Label titleLabel_;
    ui::components::Label footerLabel_;
    ui::components::HeroBanner heroBanner_;
    ui::components::List<moonlight::GameStreamApp> appListView_;
    std::vector<moonlight::GameStreamApp> apps_;
    std::unique_ptr<AppListWorker> appListWorker_ = nullptr;
};

} // namespace activities
} // namespace lunar3d
