#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host_list.hpp"
#include "task/channel.hpp"
#include "task/pair_task.hpp"
#include "task/worker.hpp"
#include "ui/activity.hpp"
#include "ui/components/button.hpp"
#include "ui/components/input_field.hpp"
#include "ui/components/label.hpp"
#include "ui/components/spinner.hpp"

#include <citro2d.h>

namespace lunar3d {
namespace activities {

class PairActivity : public ui::Activity {

  using PairWorker = task::Worker<void, task::PairTask>;

  public:
    PairActivity(const moonlight::ClientIdentity& identity, moonlight::HostList& hostList);
    ~PairActivity() override;
    void onStart() override;
    void onStop() override;
    bool onKeyEvent(const ui::KeyEvent& event) override;
    void update(u64 deltaMilli) override;

  private:
    enum class PairUiState { Idle, CheckingHost, Pairing };

    void startPairRuntime();
    void launchPair();
    void applyUiState(PairUiState state);
    void shutdownPairRuntime();
    void inspectPairResult();
    void handlePairResult(const task::PairResult& result);
    bool addPairedHost(const moonlight::Host& host);
    void setStatus(const char* text);

    const moonlight::ClientIdentity& identity_;
    moonlight::HostList& hostList_;
    ui::components::Label titleLabel_;
    ui::components::InputField<64> hostInput_;
    ui::components::Label pinInstructionLabel_;
    ui::components::Label pinLabel_;
    ui::components::Button pairButton_;
    ui::components::Spinner spinner_;
    ui::components::Label statusLabel_;
    ui::components::Label footerLabel_;
    task::Channel<task::PairRequest, task::PairRequestCapacity>* pairRequests_ = nullptr;
    task::Channel<task::PairResult, task::PairResultCapacity>* pairResults_ = nullptr;
    std::unique_ptr<PairWorker> pairWorker_ = nullptr;
    PairUiState uiState_ = PairUiState::Idle;
    bool pairing_ = false;
};

} // namespace activities
} // namespace lunar3d
