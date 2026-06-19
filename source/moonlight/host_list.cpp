#include "moonlight/host_list.hpp"

#include <mbedtls/x509_crt.h>

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#include <cstring>
#include <utility>

namespace lunar3d {
namespace moonlight {
namespace {

const char* HostListFileName = "host_list.dat";
const u8 HostListMagic[] = {'L', '3', 'D', 'H', 'O', 'S', 'T', 'S'};
constexpr u16 HostListVersion = 1;
constexpr u32 MaxHosts = 256;
constexpr u32 MaxStringBytes = 1024 * 1024;

bool ensureDirectory(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    struct stat info;
    if (stat(path.c_str(), &info) == 0) {
        return S_ISDIR(info.st_mode);
    }

    return mkdir(path.c_str(), 0777) == 0 || errno == EEXIST;
}

bool ensureDirectoryTree(const std::string& path) {
    for (size_t i = 0; i < path.size(); i++) {
        if (path[i] != '/') {
            continue;
        }

        std::string parent = path.substr(0, i);
        bool isRoot = parent.empty() || parent[parent.size() - 1] == ':';
        if (!isRoot && !ensureDirectory(parent)) {
            return false;
        }
    }

    return ensureDirectory(path);
}

std::string joinPath(const std::string& directory, const char* fileName) {
    if (!directory.empty() && directory[directory.size() - 1] == '/') {
        return directory + fileName;
    }

    return directory + "/" + fileName;
}

bool readExact(FILE* file, void* output, size_t size) {
    return fread(output, 1, size, file) == size;
}

bool writeExact(FILE* file, const void* input, size_t size) {
    return fwrite(input, 1, size, file) == size;
}

bool readU16(FILE* file, u16& value) {
    return readExact(file, &value, sizeof(value));
}

bool writeU16(FILE* file, u16 value) {
    return writeExact(file, &value, sizeof(value));
}

bool readU32(FILE* file, u32& value) {
    return readExact(file, &value, sizeof(value));
}

bool writeU32(FILE* file, u32 value) {
    return writeExact(file, &value, sizeof(value));
}

bool readString(FILE* file, std::string& value) {
    u32 size = 0;
    if (!readU32(file, size) || size > MaxStringBytes) {
        return false;
    }

    value.resize(size);
    return size == 0 || readExact(file, &value[0], size);
}

bool writeString(FILE* file, const std::string& value) {
    if (!writeU32(file, static_cast<u32>(value.size()))) {
        return false;
    }

    return value.empty() || writeExact(file, value.data(), value.size());
}

bool loadPlaincertDer(Host& host) {
    host.plaincertDer.clear();
    if (host.plaincert.empty()) {
        return true;
    }

    mbedtls_x509_crt certificate;
    mbedtls_x509_crt_init(&certificate);

    int result = mbedtls_x509_crt_parse(
        &certificate, reinterpret_cast<const unsigned char*>(host.plaincert.c_str()),
        host.plaincert.size() + 1);
    if (result == 0 && certificate.raw.p && certificate.raw.len > 0) {
        host.plaincertDer.assign(certificate.raw.p, certificate.raw.p + certificate.raw.len);
    }

    mbedtls_x509_crt_free(&certificate);
    return result == 0;
}

bool readHost(FILE* file, Host& host) {
    host = Host();
    return readString(file, host.address) && readString(file, host.hostname) &&
           readString(file, host.uniqueid) && readString(file, host.plaincert) &&
           readU16(file, host.externalPort) && readU16(file, host.httpsPort) &&
           readU16(file, host.rtspPort) && loadPlaincertDer(host);
}

bool writeHost(FILE* file, const Host& host) {
    return writeString(file, host.address) && writeString(file, host.hostname) &&
           writeString(file, host.uniqueid) && writeString(file, host.plaincert) &&
           writeU16(file, host.externalPort) && writeU16(file, host.httpsPort) &&
           writeU16(file, host.rtspPort);
}

} // namespace

HostList::HostList(std::string directory) : directory_(std::move(directory)) {}

HostListResult HostList::load() {
    return load(directory_, *this);
}

HostListResult HostList::add(const Host& host) {
    if (hosts_.size() >= MaxHosts) {
        return HostListResult::InvalidData;
    }

    hosts_.push_back(host);
    HostListResult result = store();
    if (result != HostListResult::Ok) {
        hosts_.pop_back();
    }
    return result;
}

HostListResult HostList::remove(size_t index) {
    if (index >= hosts_.size()) {
        return HostListResult::InvalidData;
    }

    Host removed = hosts_[index];
    hosts_.erase(hosts_.begin() + static_cast<int>(index));
    HostListResult result = store();
    if (result != HostListResult::Ok) {
        hosts_.insert(hosts_.begin() + static_cast<int>(index), removed);
    }
    return result;
}

const Host* HostList::get(size_t index) const {
    return index < hosts_.size() ? &hosts_[index] : nullptr;
}

size_t HostList::size() const {
    return hosts_.size();
}

bool HostList::empty() const {
    return hosts_.empty();
}

HostList::Iterator HostList::begin() {
    return hosts_.begin();
}

HostList::Iterator HostList::end() {
    return hosts_.end();
}

HostList::ConstIterator HostList::begin() const {
    return hosts_.begin();
}

HostList::ConstIterator HostList::end() const {
    return hosts_.end();
}

HostListResult HostList::store() const {
    return store(directory_, *this);
}

HostListResult HostList::load(const std::string& directory, HostList& hostList) {
    hostList.directory_ = directory;
    hostList.hosts_.clear();

    FILE* file = fopen(joinPath(directory, HostListFileName).c_str(), "rb");
    if (!file) {
        return HostListResult::IoError;
    }

    u8 magic[sizeof(HostListMagic)] = {};
    u16 version = 0;
    u32 hostCount = 0;
    bool ok = readExact(file, magic, sizeof(magic)) &&
              memcmp(magic, HostListMagic, sizeof(HostListMagic)) == 0 && readU16(file, version) &&
              version == HostListVersion && readU32(file, hostCount) && hostCount <= MaxHosts;

    for (u32 i = 0; ok && i < hostCount; i++) {
        Host host;
        ok = readHost(file, host);
        if (ok) {
            hostList.hosts_.push_back(host);
        }
    }

    bool closed = fclose(file) == 0;
    if (!ok || !closed) {
        hostList.hosts_.clear();
        return ok ? HostListResult::IoError : HostListResult::InvalidData;
    }

    return HostListResult::Ok;
}

HostListResult HostList::store(const std::string& directory, const HostList& hostList) {
    if (!ensureDirectoryTree(directory)) {
        return HostListResult::IoError;
    }

    FILE* file = fopen(joinPath(directory, HostListFileName).c_str(), "wb");
    if (!file) {
        return HostListResult::IoError;
    }

    bool ok = writeExact(file, HostListMagic, sizeof(HostListMagic)) &&
              writeU16(file, HostListVersion) &&
              writeU32(file, static_cast<u32>(hostList.hosts_.size()));

    for (size_t i = 0; ok && i < hostList.hosts_.size(); i++) {
        ok = writeHost(file, hostList.hosts_[i]);
    }

    bool closed = fclose(file) == 0;
    return ok && closed ? HostListResult::Ok : HostListResult::IoError;
}

const char* toString(HostListResult result) {
    switch (result) {
    case HostListResult::Ok:
        return "ok";
    case HostListResult::IoError:
        return "io-error";
    case HostListResult::InvalidData:
        return "invalid-data";
    }

    return "unknown";
}

} // namespace moonlight
} // namespace lunar3d
