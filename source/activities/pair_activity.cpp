#include "activities/pair_activity.hpp"

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
PairActivity::PairActivity(const moonlight::ClientIdentity& identity, moonlight::HostList& hostList)
    : identity_(identity), hostList_(hostList),
      titleLabel_("Pair Host", ui::Point(8.0f, 8.0f), ui::style::typography::TitleScale,
                  ui::style::colors::Text),
      hostInput_(ui::Rect(16.0f, 32.0f, 288.0f, 40.0f), "Input a hostname or address"),
      pinInstructionLabel_("Enter this PIN on the host:", ui::Point(24.0f, 76.0f),
                           ui::style::typography::BodyScale, ui::style::colors::HintText),
      pinLabel_("", ui::Point(24.0f, 104.0f), 1.2f, ui::style::colors::Text),
      pairButton_("Pair", ui::Rect(16.0f, 88.0f, 112.0f, 40.0f)),
      spinner_(ui::Point(144.0f, 98.0f), ui::style::typography::BodyScale, ui::style::colors::Text),
      statusLabel_("", ui::Point(24.0f, 52.0f), ui::style::typography::BodyScale,
                   ui::style::colors::HintText),
      footerLabel_("", ui::Point(16.0f, 212.0f), ui::style::typography::BodyScale,
                   ui::style::colors::HintText) {
    setBackgroundColors(ui::style::colors::TopBackground, ui::style::colors::BottomBackground);

    pairButton_.setOnClickListener([this]() { launchPair(); });
    hostInput_.setOnValueChangeListener([this](std::string& _) { applyUiState(uiState_); });
    spinner_.setVisible(false);
    pinInstructionLabel_.setVisible(false);
    pinLabel_.setVisible(false);
    statusLabel_.setVisible(false);

    std::string footerMessage;
    footerMessage += ui::glyphs::ButtonB;
    footerMessage += " Return";
    footerLabel_.setText(footerMessage.c_str());

    addComponent(GFX_TOP, titleLabel_);
    addComponent(GFX_TOP, statusLabel_);
    addComponent(GFX_TOP, pinInstructionLabel_);
    addComponent(GFX_TOP, pinLabel_);
    addComponent(GFX_TOP, footerLabel_);
    addComponent(GFX_BOTTOM, hostInput_);
    addComponent(GFX_BOTTOM, pairButton_);
    addComponent(GFX_BOTTOM, spinner_);
}

PairActivity::~PairActivity() {
    shutdownPairRuntime();
}

void PairActivity::onStart() {
    startPairRuntime();
    applyUiState(PairUiState::Idle);
    setStatus("");
    hostInput_.setHint("Hostname or address");
    statusLabel_.setVisible(false);
}

void PairActivity::onStop() {
    shutdownPairRuntime();
    hostInput_.setHint("");
    hostInput_.setText("");
    applyUiState(PairUiState::Idle);
}

bool PairActivity::onKeyEvent(const ui::KeyEvent& event) {
    if (event.action == ui::KeyAction::Down && event.key == KEY_B) {
        if (uiState_ == PairUiState::Idle) {
            finish();
        }
        return true;
    }

    return Activity::onKeyEvent(event);
}

void PairActivity::update(u64 deltaMilli) {
    Activity::update(deltaMilli);
    inspectPairResult();
}

void PairActivity::startPairRuntime() {
    if (pairWorker_) {
        return;
    }

    pairRequests_ = new task::Channel<task::PairRequest, task::PairRequestCapacity>();
    pairResults_ = new task::Channel<task::PairResult, task::PairResultCapacity>();
    pairWorker_ = task::launch(task::PairTask(*pairRequests_, *pairResults_));

    if (!pairWorker_) {
        setStatus("Could not start pairing task");
        statusLabel_.setVisible(true);
        shutdownPairRuntime();
    }
}

void PairActivity::launchPair() {
    if (pairing_ || hostInput_.data().empty()) {
        return;
    }

    task::PairRequest request(hostInput_.data(), identity_);

    pairing_ = pairRequests_ && pairRequests_->trySend(std::move(request));
    if (pairing_) {
        applyUiState(PairUiState::CheckingHost);
        return;
    }

    setStatus("Could not submit pairing request");
    statusLabel_.setVisible(true);
    applyUiState(uiState_);
}

void PairActivity::applyUiState(PairUiState state) {
    uiState_ = state;

    pinInstructionLabel_.setVisible(state == PairUiState::Pairing);
    pinLabel_.setVisible(state == PairUiState::Pairing);
    spinner_.setVisible(state == PairUiState::CheckingHost || state == PairUiState::Pairing);
    hostInput_.setEnabled(state == PairUiState::Idle);
    pairButton_.setLabel("Pair");
    footerLabel_.setVisible(state == PairUiState::Idle);
    pairButton_.setEnabled(uiState_ == PairUiState::Idle && !hostInput_.data().empty());
}

void PairActivity::shutdownPairRuntime() {
    if (pairRequests_) {
        pairRequests_->close();
    }
    if (pairWorker_) {
        pairWorker_->join();
        pairWorker_.reset();
    }

    delete pairResults_;
    pairResults_ = nullptr;
    delete pairRequests_;
    pairRequests_ = nullptr;
    pairing_ = false;
}

void PairActivity::inspectPairResult() {
    if (!pairResults_) {
        return;
    }

    std::optional<task::PairResult> result = pairResults_->tryReceive();
    if (!result) {
        return;
    }

    handlePairResult(*result);
    pairing_ = false;
}

void PairActivity::handlePairResult(const task::PairResult& result) {
    if (std::get_if<task::PairStarted>(&result)) {
        applyUiState(PairUiState::CheckingHost);
        setStatus("Checking host...");
        statusLabel_.setVisible(true);
        return;
    }

    if (const task::PairSuccess* success = std::get_if<task::PairSuccess>(&result)) {
        if (!addPairedHost(success->host)) {
            return;
        }

        setStatus("Paired");
        finish();
        return;
    }

    if (std::get_if<task::PairInvalidRequest>(&result)) {
        setStatus("Enter a hostname or address");
        applyUiState(PairUiState::Idle);
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
        setStatus(message);
        applyUiState(PairUiState::Idle);
        return;
    }

    if (const moonlight::GameStreamResult* gameStreamResult =
            std::get_if<moonlight::GameStreamResult>(&result)) {
        char message[64];
        std::snprintf(message, sizeof(message), "Server response error: %s",
                      moonlight::toString(*gameStreamResult));
        setStatus(message);
        applyUiState(PairUiState::Idle);
        return;
    }

    if (const task::PairPinRequired* pinRequired = std::get_if<task::PairPinRequired>(&result)) {
        pinLabel_.setText(pinRequired->pin.c_str());
        setStatus("Waiting for host confirmation...");
        applyUiState(PairUiState::Pairing);
        return;
    }

    if (std::get_if<task::PairPinWrong>(&result)) {
        setStatus("PIN was rejected");
        applyUiState(PairUiState::Idle);
        return;
    }

    if (std::get_if<task::PairFailed>(&result)) {
        setStatus("Pairing failed");
        applyUiState(PairUiState::Idle);
    }
}

bool PairActivity::addPairedHost(const moonlight::Host& host) {
    const moonlight::HostListResult result = hostList_.add(host);
    if (result == moonlight::HostListResult::Ok) {
        return true;
    }

    char message[64];
    std::snprintf(message, sizeof(message), "Could not save host: %s", moonlight::toString(result));
    setStatus(message);
    applyUiState(PairUiState::Idle);
    return false;
}

void PairActivity::setStatus(const char* text) {
    statusLabel_.setText(text);
}

} // namespace activities
} // namespace lunar3d
