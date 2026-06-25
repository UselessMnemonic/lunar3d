#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/nvclient.hpp"
#include "moonlight/xml.hpp"
#include "task/channel.hpp"

#include <3ds.h>
#include <Limelight.h>

#include <cstdarg>
#include <cstddef>
#include <string>
#include <variant>

namespace lunar3d {
namespace task {

class StreamTask {
  public:
    StreamTask(const moonlight::Host& host, const moonlight::GameStreamApp& app);

    void operator()();

    void OnConnStageStarting(int stage);
    void OnConnStageComplete(int stage);
    void OnConnStageFailed(int stage, int errorCode);
    void OnConnConnectionStarted();
    void OnConnConnectionTerminated(int errorCode);
    void OnConnLogMessageV(const char* format, va_list args);
    void OnConnRumble(unsigned short controllerNumber, unsigned short lowFreqMotor,
                      unsigned short highFreqMotor);
    void OnConnConnectionStatusUpdate(int connectionStatus);
    void OnConnSetHdrMode(bool hdrEnabled);
    void OnConnRumbleTriggers(uint16_t controllerNumber, uint16_t leftTriggerMotor,
                              uint16_t rightTriggerMotor);
    void OnConnSetMotionEventState(uint16_t controllerNumber, uint8_t motionType,
                                   uint16_t reportRateHz);
    void OnConnSetControllerLED(uint16_t controllerNumber, uint8_t r, uint8_t g, uint8_t b);
    void OnConnSetAdaptiveTriggers(uint16_t controllerNumber, uint8_t eventFlags, uint8_t typeLeft,
                                   uint8_t typeRight, uint8_t* left, uint8_t* right);
    int OnDecoderRendererSetup(int videoFormat, int width, int height, int redrawRate, int drFlags);
    void OnDecoderRendererStart();
    void OnDecoderRendererStop();
    void OnDecoderRendererCleanup();
    int OnDecoderRendererSubmitDecodeUnit(PDECODE_UNIT decodeUnit);
    int OnAudioRendererInit(int audioConfiguration,
                            const POPUS_MULTISTREAM_CONFIGURATION opusConfig, int arFlags);
    void OnAudioRendererStart();
    void OnAudioRendererStop();
    void OnAudioRendererCleanup();
    void OnAudioRendererDecodeAndPlaySample(char* sampleData, int sampleLength);

  private:
    std::string hostAddress_;
    std::string appVersion_;
    std::string gfeVersion_;
    int appId_;
    int serverCodecModeSupport_;
};

} // namespace task
} // namespace lunar3d
