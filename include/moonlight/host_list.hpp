#pragma once

#include "moonlight/host.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace lunar3d {
namespace moonlight {

enum class HostListResult {
    Ok,
    IoError,
    InvalidData,
};

class HostList {
  public:
    using Iterator = std::vector<Host>::iterator;
    using ConstIterator = std::vector<Host>::const_iterator;

    explicit HostList(std::string directory);

    HostListResult load();
    HostListResult add(const Host& host);
    HostListResult remove(size_t index);
    const Host* get(size_t index) const;
    size_t size() const;
    bool empty() const;
    Iterator begin();
    Iterator end();
    ConstIterator begin() const;
    ConstIterator end() const;

  private:
    HostListResult store() const;
    static HostListResult load(const std::string& directory, HostList& hostList);
    static HostListResult store(const std::string& directory, const HostList& hostList);

    std::string directory_;
    std::vector<Host> hosts_;
};

const char* toString(HostListResult result);

} // namespace moonlight
} // namespace lunar3d
