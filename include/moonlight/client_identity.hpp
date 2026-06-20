#pragma once

#include <3ds.h>

#include <string>
#include <vector>

namespace lunar3d {
namespace moonlight {

enum class ClientIdentityResult {
    Ok,
    IoError,
    InvalidData,
    CryptoError,
};

struct ClientIdentity {
    std::string uniqueId;
    std::string certificatePem;
    std::string privateKeyPem;
    std::vector<u8> certificateDer;
    std::vector<u8> privateKeyDer;

    static ClientIdentityResult load(const std::string& directory, ClientIdentity& result);

  private:
    static ClientIdentityResult generate(ClientIdentity& identity);
    static ClientIdentityResult store(const std::string& directory, const ClientIdentity& identity);
};

const char* toString(ClientIdentityResult result);

} // namespace moonlight
} // namespace lunar3d
