#include "task/stream_task.hpp"

namespace lunar3d {
namespace task {

static inline Result OpusResult(int opusError) {
    switch (opusError) {
    case OPUS_OK:
        return 0;

    case OPUS_BAD_ARG:
        return MAKERESULT(RL_USAGE, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_COMBINATION);

    case OPUS_BUFFER_TOO_SMALL:
        return MAKERESULT(RL_USAGE, RS_WRONGARG, RM_APPLICATION, RD_INVALID_SIZE);

    case OPUS_INTERNAL_ERROR:
        return MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE);

    case OPUS_INVALID_PACKET:
        return MAKERESULT(RL_PERMANENT, RS_INVALIDARG, RM_APPLICATION, RD_INVALID_COMBINATION);

    case OPUS_UNIMPLEMENTED:
        return MAKERESULT(RL_PERMANENT, RS_NOTSUPPORTED, RM_APPLICATION, RD_NOT_IMPLEMENTED);

    case OPUS_INVALID_STATE:
        return MAKERESULT(RL_STATUS, RS_INVALIDSTATE, RM_APPLICATION, RD_NOT_INITIALIZED);

    case OPUS_ALLOC_FAIL:
        return MAKERESULT(RL_TEMPORARY, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY);

    default:
        return MAKERESULT(RL_PERMANENT, RS_INTERNAL, RM_APPLICATION, RD_INVALID_RESULT_VALUE);
    }
}

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
    singleton->OnAudioRendererDecodeAndPlaySample(
        reinterpret_cast<const unsigned char*>(sampleData), sampleLength);
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
    arCallbacks.capabilities = CAPABILITY_DIRECT_SUBMIT | CAPABILITY_SLOW_OPUS_DECODER;

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

Result StreamTask::OnDecoderRendererSetup(int videoFormat, int width, int height, int redrawRate,
                                          int drFlags) {
    return 0;
}

void StreamTask::OnDecoderRendererStart() {}

void StreamTask::OnDecoderRendererStop() {}

void StreamTask::OnDecoderRendererCleanup() {}

Result StreamTask::OnDecoderRendererSubmitDecodeUnit(PDECODE_UNIT decodeUnit) {
    return 0;
}

Result StreamTask::OnAudioRendererInit(int audioConfiguration,
                                       const POPUS_MULTISTREAM_CONFIGURATION opusConfig,
                                       int arFlags) {
    (void)audioConfiguration;
    (void)arFlags;

    // prepare opus decoder
    int opusError = OPUS_OK;
    opusDecoder_ = opus_multistream_decoder_create(opusConfig->sampleRate, opusConfig->channelCount,
                                                   opusConfig->streams, opusConfig->coupledStreams,
                                                   opusConfig->mapping, &opusError);

    if (opusError != OPUS_OK) {
        return OpusResult(opusError);
    }

    // prepare ndsp service
    Result ndspResult = ndspInit();
    if (R_FAILED(ndspResult)) {
        opus_multistream_decoder_destroy(opusDecoder_);
        opusDecoder_ = nullptr;
        return ndspResult;
    }

    int channelCount = opusConfig->channelCount;
    int frameCapacity = opusConfig->samplesPerFrame;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnReset(StreamAudioChannel);
    ndspChnSetFormat(StreamAudioChannel,
                     channelCount == 1 ? NDSP_FORMAT_MONO_PCM16 : NDSP_FORMAT_STEREO_PCM16);
    ndspChnSetRate(StreamAudioChannel, static_cast<float>(opusConfig->sampleRate));
    ndspChnSetInterp(StreamAudioChannel, NDSP_INTERP_POLYPHASE);

    float mix[12] = {};
    if (channelCount == 1) {
        mix[0] = 0.75f;
        mix[1] = 0.75f;
    } else {
        mix[0] = 1.0f;
        mix[1] = 1.0f;
    }
    ndspChnSetMix(StreamAudioChannel, mix);

    // create audio buffers
    for (auto& next : buffers_) {
        next = NDSPBuffer<s16>(frameCapacity, channelCount);
    }

    return MAKERESULT(RL_SUCCESS, RS_SUCCESS, RM_APPLICATION, RD_SUCCESS);
}

void StreamTask::OnAudioRendererStart() {}

void StreamTask::OnAudioRendererStop() {}

void StreamTask::OnAudioRendererCleanup() {
    ndspChnWaveBufClear(StreamAudioChannel);
    ndspChnReset(StreamAudioChannel);

    for (auto& next : buffers_) {
        next = NDSPBuffer<s16>();
    }

    if (opusDecoder_ != nullptr) {
        opus_multistream_decoder_destroy(opusDecoder_);
        opusDecoder_ = nullptr;
    }

    ndspExit();
}

void StreamTask::OnAudioRendererDecodeAndPlaySample(const unsigned char* opusPacketData,
                                                    int opusPacketLength) {
    // find a free buffer
    NDSPBuffer<s16>* buffer = nullptr;

    for (auto& next : buffers_) {
        const auto status = next.status();

        if (status == NDSP_WBUF_FREE || status == NDSP_WBUF_DONE) {
            buffer = &next;
            break;
        }
    }

    if (buffer == nullptr) {
        return;
    }

    // decode one compressed Opus packet
    const int frames = opus_multistream_decode(opusDecoder_, opusPacketData, opusPacketLength,
                                               buffer->pcm(), buffer->frameCapacity(), 0);
    if (frames <= 0) {
        return;
    }
    auto wavBuf = buffer->waveBufForFrames(frames);

    // send to ndsp
    const auto size = frames * buffer->channelCount() * sizeof(s16);
    DSP_FlushDataCache(buffer->pcm(), size);
    ndspChnWaveBufAdd(StreamAudioChannel, wavBuf);
}

} // namespace task
} // namespace lunar3d
