#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/nvclient.hpp"
#include "moonlight/xml.hpp"
#include "task/channel.hpp"

#include <3ds.h>

#include <cstddef>
#include <string>
#include <variant>

namespace lunar3d {
namespace task {

struct PairRequest {
    moonlight::Host host;
    moonlight::ClientIdentity identity;
    std::string clientUuid;
    std::string pin;
};

struct PairStarted {};

struct PairSuccess {
    moonlight::Host host;
};

struct PairInvalidRequest {};

struct PairPinRequired {
    std::string pin;
};

struct PairPinWrong {};

struct PairFailed {};

using PairResult =
    std::variant<PairStarted, PairSuccess, PairInvalidRequest, moonlight::NvResult,
                 moonlight::GameStreamResult, PairPinRequired, PairPinWrong, PairFailed>;

const char* toString(const PairResult& result);

constexpr size_t PairRequestCapacity = 1;
constexpr size_t PairResultCapacity = 1;

class PairTask {
  public:
    PairTask(Channel<PairRequest, PairRequestCapacity>& requests,
             Channel<PairResult, PairResultCapacity>& results)
        : requests_(requests), results_(results) {}

    void run();

  private:
    PairResult pair(PairRequest& request);
    PairResult pairWithServerInfo(PairRequest& request, moonlight::Host& host,
                                  moonlight::NvClient& client,
                                  const moonlight::GameStreamServerInfo& info);
    Channel<PairRequest, PairRequestCapacity>& requests_;
    Channel<PairResult, PairResultCapacity>& results_;
};

} // namespace task
} // namespace lunar3d
