#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"
#include "moonlight/nvclient.hpp"
#include "moonlight/xml.hpp"

#include <variant>
#include <vector>

namespace lunar3d {
namespace task {

struct AppListRequest {
    const moonlight::Host& host;
    const moonlight::ClientIdentity& identity;
};

struct AppListSuccess {
    std::vector<moonlight::GameStreamApp> apps;
};

struct AppListInvalidRequest {};

using AppListResult = std::variant<AppListSuccess, AppListInvalidRequest, moonlight::NvResult,
                                   moonlight::GameStreamResult>;

class AppListTask {
  public:
    explicit AppListTask(AppListRequest request) : request_(request) {}

    AppListResult operator()();

  private:
    AppListRequest request_;
};

} // namespace task
} // namespace lunar3d
