#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/nvclient.hpp"
#include "moonlight/xml.hpp"
#include "task/channel.hpp"

#include <Limelight.h>
#include <3ds.h>

#include <cstddef>
#include <string>
#include <variant>

namespace lunar3d {
namespace task {

class StreamTask {
  public:
    StreamTask(const moonlight::Host& host, const moonlight::GameStreamApp& app);

    void operator()();

  private:
    std::string hostAddress_;
    std::string appVersion_;
    std::string gfeVersion_;
    int appId_;
    int serverCodecModeSupport_;
};

} // namespace task
} // namespace lunar3d
