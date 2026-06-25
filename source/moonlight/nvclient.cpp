#include "moonlight/nvclient.hpp"

#include <stdio.h>

namespace lunar3d {
namespace moonlight {
namespace {

constexpr u32 MaxResponseBytes = 1024 * 1024;
constexpr u32 DownloadChunkBytes = 4096;
constexpr u64 RequestTimeoutNanoseconds = 6ULL * 1000ULL * 1000ULL * 1000ULL;
constexpr u64 PairRequestTimeoutNanoseconds = 30ULL * 1000ULL * 1000ULL * 1000ULL;

void appendHex(const u8* input, size_t size, std::string& output) {
    static const char digits[] = "0123456789abcdef";
    size_t offset = output.size();
    output.resize(offset + size * 2);

    for (size_t i = 0; i < size; i++) {
        output[offset + i * 2] = digits[input[i] >> 4];
        output[offset + i * 2 + 1] = digits[input[i] & 0x0F];
    }
}

std::string certificateHex(const ClientIdentity& identity) {
    std::string output;
    appendHex(reinterpret_cast<const u8*>(identity.certificatePem.data()),
              identity.certificatePem.size(), output);
    return output;
}

bool responseCodeOk(u32 statusCode) {
    return statusCode >= 200 && statusCode < 300;
}

bool timedOut(Result result) {
    return static_cast<u32>(result) == HTTPC_RESULTCODE_TIMEDOUT;
}

} // namespace

NvClient::NvClient(NvClientConfig config, const ClientIdentity& identity)
    : config(config), identity(identity) {}

NvResult NvClient::serverInfo(bool secure, std::string& response) const {
    return get(secure, "/serverinfo?" + baseQuery(), response, RequestTimeoutNanoseconds);
}

NvResult NvClient::appList(std::string& response) const {
    return get(true, "/applist?" + baseQuery(), response, RequestTimeoutNanoseconds);
}

NvResult NvClient::unpair(std::string& response) const {
    return get(false, "/unpair?" + baseQuery(), response, RequestTimeoutNanoseconds);
}

NvResult NvClient::pairGetServerCertificate(const std::string& saltHex,
                                            std::string& response) const {
    std::string query = "/pair?" + baseQuery();
    query += "&devicename=" + config.deviceName;
    query += "&updateState=1&phrase=getservercert&salt=" + saltHex;
    query += "&clientcert=" + certificateHex(identity);
    return get(false, query, response, PairRequestTimeoutNanoseconds);
}

NvResult NvClient::pairSendClientChallenge(const std::string& challengeHex,
                                           std::string& response) const {
    std::string query = "/pair?" + baseQuery();
    query += "&devicename=" + config.deviceName;
    query += "&updateState=1&clientchallenge=" + challengeHex;
    return get(false, query, response, RequestTimeoutNanoseconds);
}

NvResult NvClient::pairSendServerChallengeResponse(const std::string& challengeResponseHex,
                                                   std::string& response) const {
    std::string query = "/pair?" + baseQuery();
    query += "&devicename=" + config.deviceName;
    query += "&updateState=1&serverchallengeresp=" + challengeResponseHex;
    return get(false, query, response, RequestTimeoutNanoseconds);
}

NvResult NvClient::pairSendClientPairingSecret(const std::string& pairingSecretHex,
                                               std::string& response) const {
    std::string query = "/pair?" + baseQuery();
    query += "&devicename=" + config.deviceName;
    query += "&updateState=1&clientpairingsecret=" + pairingSecretHex;
    return get(false, query, response, RequestTimeoutNanoseconds);
}

NvResult NvClient::get(bool secure, const std::string& pathAndQuery, std::string& response,
                       u64 timeoutNanoseconds) const {
    response.clear();

    std::string url = secure ? httpsUrl(pathAndQuery) : httpUrl(pathAndQuery);
    httpcContext context = {};

    Result result = httpcOpenContext(&context, HTTPC_METHOD_GET, url.c_str(), 0);
    if (R_FAILED(result)) {
        return NvHttpServiceError{};
    }

    u32 downloadedSize = 0;
    u32 contentSize = 0;
    u32 statusCode = 0;

    result = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_DISABLED);
    if (R_FAILED(result))
        goto request_error;

    if (secure && !config.verifyServerCertificate) {
        result = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
        if (R_FAILED(result))
            goto request_error;
    }

    if (secure && !identity.certificateDer.empty() && !identity.privateKeyDer.empty()) {
        result = httpcSetClientCert(&context, identity.certificateDer.data(),
                                    identity.certificateDer.size(), identity.privateKeyDer.data(),
                                    identity.privateKeyDer.size());
        if (R_FAILED(result))
            goto request_error;
    }

    result = httpcBeginRequest(&context);
    if (R_FAILED(result))
        goto request_error;

    result = httpcGetResponseStatusCodeTimeout(&context, &statusCode, timeoutNanoseconds);
    if (timedOut(result))
        goto request_timeout;
    if (R_FAILED(result))
        goto request_error;

    if (!responseCodeOk(statusCode)) {
        httpcCloseContext(&context);
        return NvHttpStatusError{statusCode};
    }

    result = httpcGetDownloadSizeState(&context, &downloadedSize, &contentSize);
    if (R_FAILED(result))
        goto request_error;
    if (contentSize > MaxResponseBytes) {
        httpcCloseContext(&context);
        return NvResponseTooLarge{};
    }

    response.reserve(contentSize ? contentSize : DownloadChunkBytes);
    do {
        const size_t offset = response.size();
        if (offset + DownloadChunkBytes > MaxResponseBytes) {
            httpcCloseContext(&context);
            response.clear();
            return NvResponseTooLarge{};
        }

        response.resize(offset + DownloadChunkBytes);
        Result receiveResult =
            httpcReceiveDataTimeout(&context, reinterpret_cast<u8*>(&response[0]) + offset,
                                    DownloadChunkBytes, timeoutNanoseconds);
        if (timedOut(receiveResult))
            goto request_timeout;

        result = httpcGetDownloadSizeState(&context, &downloadedSize, &contentSize);
        if (R_FAILED(result))
            goto request_error;
        response.resize(downloadedSize);

        result = receiveResult;
    } while (static_cast<u32>(result) == HTTPC_RESULTCODE_DOWNLOADPENDING);

    if (R_FAILED(result))
        goto request_error;

    httpcCloseContext(&context);
    return NvOk{};

request_error:
    httpcCloseContext(&context);
    response.clear();
    return NvRequestError{};

request_timeout:
    httpcCancelConnection(&context);
    httpcCloseContext(&context);
    response.clear();
    return NvTimedOut{};
}

std::string NvClient::baseQuery() const {
    return "uniqueid=" + identity.uniqueId + "&uuid=" + config.clientUuid;
}

std::string NvClient::httpUrl(const std::string& pathAndQuery) const {
    char port[8] = {};
    snprintf(port, sizeof(port), "%u", config.host.externalPort);
    return "http://" + config.host.address + ":" + port + pathAndQuery;
}

std::string NvClient::httpsUrl(const std::string& pathAndQuery) const {
    char port[8] = {};
    snprintf(port, sizeof(port), "%u", config.host.httpsPort);
    return "https://" + config.host.address + ":" + port + pathAndQuery;
}

bool succeeded(const NvResult& result) {
    return std::holds_alternative<NvOk>(result);
}

const char* toString(const NvResult& result) {
    if (std::holds_alternative<NvOk>(result)) {
        return "ok";
    }
    if (std::holds_alternative<NvHttpServiceError>(result)) {
        return "http-service-error";
    }
    if (std::holds_alternative<NvRequestError>(result)) {
        return "request-error";
    }
    if (std::holds_alternative<NvTimedOut>(result)) {
        return "timed-out";
    }
    if (std::holds_alternative<NvResponseTooLarge>(result)) {
        return "response-too-large";
    }
    if (std::holds_alternative<NvHttpStatusError>(result)) {
        return "http-status-error";
    }

    return "unknown";
}

} // namespace moonlight
} // namespace lunar3d
