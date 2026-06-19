#pragma once

#include "moonlight/host_list.hpp"
#include "ui/activity.hpp"
#include "ui/components/button.hpp"
#include "ui/components/label.hpp"

#include <citro2d.h>
#include <cstddef>

namespace lunar3d {
namespace activities {

class DeleteActivity : public ui::Activity {
  public:
    explicit DeleteActivity(moonlight::HostList& hostList);

    void onStart() override;
    void setIndex(size_t index);

  private:
    void confirmDelete();
    void updatePrompt();

    moonlight::HostList& hostList_;
    ui::components::Label titleLabel_;
    ui::components::Label promptLabel_;
    ui::components::Label statusLabel_;
    ui::components::Button deleteButton_;
    ui::components::Button cancelButton_;
    size_t selectedHostIndex_ = 0;
};

} // namespace activities
} // namespace lunar3d
