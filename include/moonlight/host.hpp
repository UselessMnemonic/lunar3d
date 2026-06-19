#pragma once

#include <3ds/types.h>

#include <string>
#include <vector>

namespace lunar3d {
namespace moonlight {

struct Host {
    std::string address;
    std::string hostname;
    std::string uniqueid;
    std::string plaincert;
    std::vector<u8> plaincertDer;
    u16 externalPort = 47989;
    u16 httpsPort = 47984;
    u16 rtspPort = 48010;
    int serverCodecModeSupport = 0;
    std::string appversion;
    std::string gfeVersion;
    std::string gputype;

    bool paired() const {
        return !plaincert.empty() || !plaincertDer.empty();
    }
};

} // namespace moonlight
} // namespace lunar3d
