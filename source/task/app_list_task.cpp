#include "task/app_list_task.hpp"

#include <utility>

namespace lunar3d {
namespace task {
namespace {

constexpr u64 AppListTimeoutNanoseconds = 2ULL * 1000ULL * 1000ULL * 1000ULL;

} // namespace

AppListResult AppListTask::operator()() {
    if (request_.host.address.empty() || request_.identity.uniqueId.empty()) {
        return AppListInvalidRequest{};
    }

    moonlight::NvClientConfig config{request_.host, request_.identity.uniqueId};
    moonlight::NvClient client(config, request_.identity);
    std::string response;

    moonlight::NvResult nvResult = client.appList(response, AppListTimeoutNanoseconds);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }

    std::vector<moonlight::GameStreamApp> apps;
    moonlight::GameStreamResult parseResult = moonlight::parseAppList(response, apps);
    if (parseResult != moonlight::GameStreamResult::Ok) {
        return parseResult;
    }

    return AppListSuccess{std::move(apps)};
}

} // namespace task
} // namespace lunar3d
