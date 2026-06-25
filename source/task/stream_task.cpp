#include "task/stream_task.hpp"

namespace lunar3d {
namespace task {

static StreamTask* singleton = nullptr;
extern "C" {

static void DispatchConnStageStarting(int stage) {
    singleton->OnConnStageStarting(stage);
}

static void DispatchConnStageComplete(int stage) {
    singleton->OnConnStageComplete(stage);
}

static void DispatchConnStageFailed(int stage, int errorCode) {
    singleton->OnConnStageFailed(stage, errorCode);
}

static void DispatchConnConnectionStarted() {
    singleton->OnConnConnectionStarted();
}

static void DispatchConnConnectionTerminated(int errorCode) {
    singleton->OnConnConnectionTerminated(errorCode);
}

static void DispatchConnLogMessage(const char* format, ...) {
    va_list args;
    va_start(args, format);
    singleton->OnConnLogMessageV(format, args);
    va_end(args);
}

static void DispatchConnRumble(unsigned short controllerNumber, unsigned short lowFreqMotor,
                               unsigned short highFreqMotor) {
    singleton->OnConnRumble(controllerNumber, lowFreqMotor, highFreqMotor);
}

static void DispatchConnConnectionStatusUpdate(int connectionStatus) {
    singleton->OnConnConnectionStatusUpdate(connectionStatus);
}

static void DispatchConnSetHdrMode(bool hdrEnabled) {
    singleton->OnConnSetHdrMode(hdrEnabled);
}

static void DispatchConnRumbleTriggers(uint16_t controllerNumber, uint16_t leftTriggerMotor,
                                       uint16_t rightTriggerMotor) {
    singleton->OnConnRumbleTriggers(controllerNumber, leftTriggerMotor, rightTriggerMotor);
}

static void DispatchConnSetMotionEventState(uint16_t controllerNumber, uint8_t motionType,
                                            uint16_t reportRateHz) {
    singleton->OnConnSetMotionEventState(controllerNumber, motionType, reportRateHz);
}

static void DispatchConnSetControllerLED(uint16_t controllerNumber, uint8_t r, uint8_t g,
                                         uint8_t b) {
    singleton->OnConnSetControllerLED(controllerNumber, r, g, b);
}

static void DispatchConnSetAdaptiveTriggers(uint16_t controllerNumber, uint8_t eventFlags,
                                            uint8_t typeLeft, uint8_t typeRight, uint8_t* left,
                                            uint8_t* right) {
    singleton->OnConnSetAdaptiveTriggers(controllerNumber, eventFlags, typeLeft, typeRight, left,
                                         right);
}

static int DispatchDecoderRendererSetup(int videoFormat, int width, int height, int redrawRate,
                                        void* context, int drFlags) {
    (void)context;
    return singleton->OnDecoderRendererSetup(videoFormat, width, height, redrawRate, drFlags);
}

static void DispatchDecoderRendererStart() {
    singleton->OnDecoderRendererStart();
}

static void DispatchDecoderRendererStop() {
    singleton->OnDecoderRendererStop();
}

static void DispatchDecoderRendererCleanup() {
    singleton->OnDecoderRendererCleanup();
}

static int DispatchDecoderRendererSubmitDecodeUnit(PDECODE_UNIT decodeUnit) {
    return singleton->OnDecoderRendererSubmitDecodeUnit(decodeUnit);
}

static int DispatchAudioRendererInit(int audioConfiguration,
                                     const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
                                     void* context, int arFlags) {
    (void)context;
    return singleton->OnAudioRendererInit(audioConfiguration, opusConfig, arFlags);
}

static void DispatchAudioRendererStart() {
    singleton->OnAudioRendererStart();
}

static void DispatchAudioRendererStop() {
    singleton->OnAudioRendererStop();
}

static void DispatchAudioRendererCleanup() {
    singleton->OnAudioRendererCleanup();
}

static void DispatchAudioRendererDecodeAndPlaySample(char* sampleData, int sampleLength) {
    singleton->OnAudioRendererDecodeAndPlaySample(sampleData, sampleLength);
}
}

StreamTask::StreamTask(const moonlight::Host& host, const moonlight::GameStreamApp& app)
    : hostAddress_(host.address), appVersion_(host.appversion), gfeVersion_(host.gfeVersion),
      appId_(app.id), serverCodecModeSupport_(host.serverCodecModeSupport) {}

void StreamTask::operator()() {
    SERVER_INFORMATION serverInfo;
    LiInitializeServerInformation(&serverInfo);
    serverInfo.address = hostAddress_.c_str();
    serverInfo.serverInfoAppVersion = appVersion_.empty() ? nullptr : appVersion_.c_str();
    serverInfo.serverInfoGfeVersion = gfeVersion_.empty() ? nullptr : gfeVersion_.c_str();
    serverInfo.serverCodecModeSupport = serverCodecModeSupport_;

    STREAM_CONFIGURATION streamConfig;
    LiInitializeStreamConfiguration(&streamConfig);
    streamConfig.width = 400;
    streamConfig.height = 240;
    streamConfig.fps = 60;
    streamConfig.bitrate = 1000;
    streamConfig.packetSize = 1024;

    CONNECTION_LISTENER_CALLBACKS clCallbacks;
    LiInitializeConnectionCallbacks(&clCallbacks);
    clCallbacks.stageStarting = DispatchConnStageStarting;
    clCallbacks.stageComplete = DispatchConnStageComplete;
    clCallbacks.stageFailed = DispatchConnStageFailed;
    clCallbacks.connectionStarted = DispatchConnConnectionStarted;
    clCallbacks.connectionTerminated = DispatchConnConnectionTerminated;
    clCallbacks.logMessage = DispatchConnLogMessage;
    clCallbacks.rumble = DispatchConnRumble;
    clCallbacks.connectionStatusUpdate = DispatchConnConnectionStatusUpdate;
    clCallbacks.setHdrMode = DispatchConnSetHdrMode;
    clCallbacks.rumbleTriggers = DispatchConnRumbleTriggers;
    clCallbacks.setMotionEventState = DispatchConnSetMotionEventState;
    clCallbacks.setControllerLED = DispatchConnSetControllerLED;
    clCallbacks.setAdaptiveTriggers = DispatchConnSetAdaptiveTriggers;

    DECODER_RENDERER_CALLBACKS drCallbacks;
    LiInitializeVideoCallbacks(&drCallbacks);
    drCallbacks.setup = DispatchDecoderRendererSetup;
    drCallbacks.start = DispatchDecoderRendererStart;
    drCallbacks.stop = DispatchDecoderRendererStop;
    drCallbacks.cleanup = DispatchDecoderRendererCleanup;
    drCallbacks.submitDecodeUnit = DispatchDecoderRendererSubmitDecodeUnit;

    AUDIO_RENDERER_CALLBACKS arCallbacks;
    LiInitializeAudioCallbacks(&arCallbacks);
    arCallbacks.init = DispatchAudioRendererInit;
    arCallbacks.start = DispatchAudioRendererStart;
    arCallbacks.stop = DispatchAudioRendererStop;
    arCallbacks.cleanup = DispatchAudioRendererCleanup;
    arCallbacks.decodeAndPlaySample = DispatchAudioRendererDecodeAndPlaySample;

    int drFlags = 0;
    int arFlags = 0;
    singleton = this;
    (void)LiStartConnection(&serverInfo, &streamConfig, &clCallbacks, &drCallbacks, &arCallbacks,
                            nullptr, drFlags, nullptr, arFlags);

    singleton = nullptr;
}

void StreamTask::OnConnStageStarting(int stage) {}

void StreamTask::OnConnStageComplete(int stage) {}

void StreamTask::OnConnStageFailed(int stage, int errorCode) {}

void StreamTask::OnConnConnectionStarted() {}

void StreamTask::OnConnConnectionTerminated(int errorCode) {}

void StreamTask::OnConnLogMessageV(const char* format, va_list args) {}

void StreamTask::OnConnRumble(unsigned short controllerNumber, unsigned short lowFreqMotor,
                              unsigned short highFreqMotor) {}

void StreamTask::OnConnConnectionStatusUpdate(int connectionStatus) {}

void StreamTask::OnConnSetHdrMode(bool hdrEnabled) {}

void StreamTask::OnConnRumbleTriggers(uint16_t controllerNumber, uint16_t leftTriggerMotor,
                                      uint16_t rightTriggerMotor) {}

void StreamTask::OnConnSetMotionEventState(uint16_t controllerNumber, uint8_t motionType,
                                           uint16_t reportRateHz) {}

void StreamTask::OnConnSetControllerLED(uint16_t controllerNumber, uint8_t r, uint8_t g,
                                        uint8_t b) {}

void StreamTask::OnConnSetAdaptiveTriggers(uint16_t controllerNumber, uint8_t eventFlags,
                                           uint8_t typeLeft, uint8_t typeRight, uint8_t* left,
                                           uint8_t* right) {}

int StreamTask::OnDecoderRendererSetup(int videoFormat, int width, int height, int redrawRate,
                                       int drFlags) {
    return 0;
}

void StreamTask::OnDecoderRendererStart() {}

void StreamTask::OnDecoderRendererStop() {}

void StreamTask::OnDecoderRendererCleanup() {}

int StreamTask::OnDecoderRendererSubmitDecodeUnit(PDECODE_UNIT decodeUnit) {
    return 0;
}

int StreamTask::OnAudioRendererInit(int audioConfiguration,
                                    const POPUS_MULTISTREAM_CONFIGURATION opusConfig, int arFlags) {
    return 0;
}

void StreamTask::OnAudioRendererStart() {}

void StreamTask::OnAudioRendererStop() {}

void StreamTask::OnAudioRendererCleanup() {}

void StreamTask::OnAudioRendererDecodeAndPlaySample(char* sampleData, int sampleLength) {}

} // namespace task
} // namespace lunar3d
