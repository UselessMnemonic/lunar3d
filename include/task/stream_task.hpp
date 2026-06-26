#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/nvclient.hpp"
#include "moonlight/xml.hpp"
#include "task/channel.hpp"
#include "utility.hpp"

#include <3ds.h>
#include <Limelight.h>
#include <opus/opus_multistream.h>

#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

typedef struct OpusMSDecoder OpusMSDecoder;

namespace lunar3d {
namespace task {

class StreamTask {

    template <typename TSample> class NDSPBuffer {
        static_assert(std::is_same_v<TSample, s8> || std::is_same_v<TSample, s16> ||
                      std::is_same_v<TSample, u8>,
                      "TSample must be one of s8, s16, or u8");

      public:
        NDSPBuffer() {
            wavBuf_.status = NDSP_WBUF_FREE;
        }

        explicit NDSPBuffer(int frameCapacity, int channelCount) {
            wavBuf_.status = NDSP_WBUF_FREE;
            frameCapacity_ = frameCapacity;
            channelCount_ = channelCount;
            pcmBuf_ = LinearBuffer<TSample>(frameCapacity_ * channelCount_);

            if constexpr (std::is_same_v<TSample, s8>) {
                wavBuf_.data_pcm8 = pcmBuf_.get();
            } else if constexpr (std::is_same_v<TSample, s16>) {
                wavBuf_.data_pcm16 = pcmBuf_.get();
            } else if constexpr (std::is_same_v<TSample, u8>) {
                wavBuf_.data_adpcm = pcmBuf_.get();
            }
        }

        NDSPBuffer(const NDSPBuffer&) = delete;
        NDSPBuffer& operator=(const NDSPBuffer&) = delete;

        NDSPBuffer(NDSPBuffer&& r) {
            wavBuf_ = r.wavBuf_;
            r.wavBuf_ = {};
            pcmBuf_ = std::move(r.pcmBuf_);
            frameCapacity_ = r.frameCapacity_;
            channelCount_ = r.channelCount_;
        }

        NDSPBuffer& operator=(NDSPBuffer&& r) {
            wavBuf_ = r.wavBuf_;
            r.wavBuf_ = {};
            pcmBuf_ = std::move(r.pcmBuf_);
            frameCapacity_ = r.frameCapacity_;
            channelCount_ = r.channelCount_;
            return *this;
        }

        TSample* pcm() {
            return pcmBuf_.get();
        }

        int frameCapacity() {
            return frameCapacity_;
        }

        int channelCount() {
            return channelCount_;
        }

        u8 status() {
            return wavBuf_.status;
        }

        ndspWaveBuf* waveBufForFrames(u32 frameCount) {
            wavBuf_.nsamples = frameCount;
            return &wavBuf_;
        }

      private:
        ndspWaveBuf wavBuf_ = {};
        LinearBuffer<TSample> pcmBuf_;
        int frameCapacity_ = 0;
        int channelCount_ = 0;
    };

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
    Result OnDecoderRendererSetup(int videoFormat, int width, int height, int redrawRate,
                                  int drFlags);
    void OnDecoderRendererStart();
    void OnDecoderRendererStop();
    void OnDecoderRendererCleanup();
    Result OnDecoderRendererSubmitDecodeUnit(PDECODE_UNIT decodeUnit);
    Result OnAudioRendererInit(int audioConfiguration,
                               const POPUS_MULTISTREAM_CONFIGURATION opusConfig, int arFlags);
    void OnAudioRendererStart();
    void OnAudioRendererStop();
    void OnAudioRendererCleanup();
    void OnAudioRendererDecodeAndPlaySample(const unsigned char* data, int length);

  private:
    std::string hostAddress_;
    std::string appVersion_;
    std::string gfeVersion_;
    int appId_;
    int serverCodecModeSupport_;

    static constexpr int StreamAudioChannel = 0;
    OpusMSDecoder* opusDecoder_ = nullptr;
    std::array<NDSPBuffer<s16>, 6> buffers_;
};

} // namespace task
} // namespace lunar3d
