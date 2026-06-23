#include "task/stream_task.hpp"


namespace lunar3d {
namespace task {

StreamTask::StreamTask(const moonlight::Host& host, const moonlight::GameStreamApp& app)
  : hostAddress_(host.address),
    appVersion_(host.appversion),
    gfeVersion_(host.gfeVersion),
    appId_(app.id),
    serverCodecModeSupport_(host.serverCodecModeSupport) {}

void StreamTask::operator()() {
    SERVER_INFORMATION serverInfo;
    STREAM_CONFIGURATION streamConfig;
    CONNECTION_LISTENER_CALLBACKS clCallbacks;
    DECODER_RENDERER_CALLBACKS drCallbacks;
    AUDIO_RENDERER_CALLBACKS arCallbacks;
    void* renderContext = nullptr;
    int drFlags = 0;
    void* audioContext = nullptr;
    int arFlags = 0;

    LiInitializeServerInformation(&serverInfo);
    serverInfo.address = hostAddress_.c_str();
    serverInfo.serverInfoAppVersion = appVersion_.empty() ? nullptr : appVersion_.c_str();
    serverInfo.serverInfoGfeVersion = gfeVersion_.empty() ? nullptr : gfeVersion_.c_str();
    serverInfo.serverCodecModeSupport = serverCodecModeSupport_;

    LiInitializeStreamConfiguration(&streamConfig);
    streamConfig.width = 400;
    streamConfig.height = 240;
    streamConfig.fps = 60;
    streamConfig.bitrate = 1000;
    streamConfig.packetSize = 1024;

    LiInitializeConnectionCallbacks(&clCallbacks);
    LiInitializeVideoCallbacks(&drCallbacks);
    LiInitializeAudioCallbacks(&arCallbacks);

    (void)LiStartConnection(&serverInfo, &streamConfig, &clCallbacks, &drCallbacks,
                            &arCallbacks, renderContext, drFlags, audioContext, arFlags);
}

} // namespace task
} // namespace lunar3d
