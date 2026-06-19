#pragma once

#include "activities/delete_activity.hpp"
#include "activities/pair_activity.hpp"
#include "moonlight/client_identity.hpp"
#include "moonlight/host_list.hpp"
#include "ui/activity.hpp"
#include "ui/activity_manager.hpp"
#include "ui/components/hero_banner.hpp"
#include "ui/components/label.hpp"
#include "ui/components/list.hpp"

namespace lunar3d {
namespace activities {

class MainActivity : public ui::Activity {
  public:
    explicit MainActivity(ui::ActivityManager& activityManager);

    void onRestart() override;
    bool onKeyEvent(const ui::KeyEvent& event) override;
    void update(u64 deltaMilli) override;

  private:
    void refreshHostList();
    int selectedHostIndex() const;

    moonlight::ClientIdentity identity_;
    ui::ActivityManager& activityManager_;
    moonlight::HostList hostList_;

    PairActivity pairActivity_;
    DeleteActivity deleteActivity_;

    ui::components::Label headerLabel_;
    ui::components::Label updateLabel_;
    ui::components::Label footerLabel_;
    ui::components::List<moonlight::Host> hostListView_;
    ui::components::HeroBanner heroBanner_;
    bool ready_ = false;
};

} // namespace activities
} // namespace lunar3d
