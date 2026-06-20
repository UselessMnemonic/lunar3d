#pragma once

#include "moonlight/client_identity.hpp"
#include "moonlight/host.hpp"

#include <3ds.h>

#include <string>
#include <variant>

namespace lunar3d {
namespace moonlight {

struct NvOk {};
struct NvHttpServiceError {};
struct NvRequestError {};
struct NvTimedOut {};
struct NvResponseTooLarge {};

struct NvHttpStatusError {
    u32 statusCode = 0;
};

using NvResult = std::variant<NvOk, NvHttpServiceError,
                                   NvRequestError, NvTimedOut,
                                   NvResponseTooLarge,
                                   NvHttpStatusError>;

struct NvClientConfig {
    const Host& host;
    const std::string& clientUuid;
    std::string deviceName = "roth";
    bool verifyServerCertificate = false;
};

class NvClient {
public:
    NvClient(NvClientConfig config, const ClientIdentity& identity);

    NvResult serverInfo(bool secure, std::string& response) const;
    NvResult appList(std::string& response) const;
    NvResult appList(std::string& response, u64 timeoutNanoseconds) const;
    NvResult unpair(std::string& response) const;

    NvResult pairGetServerCertificate(const std::string& saltHex,
                                           std::string& response) const;
    NvResult pairSendClientChallenge(const std::string& challengeHex,
                                          std::string& response) const;
    NvResult pairSendServerChallengeResponse(const std::string& challengeResponseHex,
                                                   std::string& response) const;
    NvResult pairSendClientPairingSecret(const std::string& pairingSecretHex,
                                              std::string& response) const;

private:
    NvResult get(bool secure, const std::string& pathAndQuery,
                      std::string& response, u64 timeoutNanoseconds) const;
    std::string baseQuery() const;
    std::string httpUrl(const std::string& pathAndQuery) const;
    std::string httpsUrl(const std::string& pathAndQuery) const;

    NvClientConfig config;
    const ClientIdentity& identity;
};

bool succeeded(const NvResult& result);
const char* toString(const NvResult& result);

} // namespace moonlight
} // namespace lunar3d
