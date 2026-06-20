#include "task/pair_task.hpp"

#include <3ds.h>

#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/pem.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#include <tinyxml2.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

namespace lunar3d {
namespace task {
namespace {

constexpr u32 PinModulo = 10000;
constexpr size_t SaltBytes = 16;
constexpr size_t ChallengeBytes = 16;
constexpr size_t SecretBytes = 16;
constexpr size_t AesKeyBytes = 16;
constexpr size_t AesBlockBytes = 16;
constexpr size_t SignatureBufferBytes = 512;
constexpr size_t PemBufferBytes = 4096;

enum class PairHash { Sha1, Sha256 };

enum class AesOperation : bool { DECRYPT = false, ENCRYPT = true };

struct ServerCertificateResponse {
    bool paired = false;
    std::string plaincertHex;
};

struct ChallengeResponse {
    bool paired = false;
    std::string challengeResponseHex;
};

struct PairingSecretResponse {
    bool paired = false;
    std::string pairingSecretHex;
};

struct PairedResponse {
    bool paired = false;
};

size_t hashLength(PairHash hash) {
    return hash == PairHash::Sha256 ? 32 : 20;
}

mbedtls_md_type_t mdType(PairHash hash) {
    return hash == PairHash::Sha256 ? MBEDTLS_MD_SHA256 : MBEDTLS_MD_SHA1;
}

std::string generatePin() {
    u32 value = 0;
    if (R_FAILED(PS_GenerateRandomBytes(reinterpret_cast<u8*>(&value), sizeof(value)))) {
        value = static_cast<u32>(osGetTime());
    }

    char pin[5];
    std::snprintf(pin, sizeof(pin), "%04lu", static_cast<unsigned long>(value % PinModulo));
    return pin;
}

bool randomBytes(std::vector<u8>& output, size_t size) {
    output.resize(size);
    return R_SUCCEEDED(PS_GenerateRandomBytes(output.data(), output.size()));
}

void appendHex(const u8* input, size_t size, std::string& output) {
    static const char digits[] = "0123456789ABCDEF";
    const size_t offset = output.size();
    output.resize(offset + size * 2);

    for (size_t i = 0; i < size; i++) {
        output[offset + i * 2] = digits[input[i] >> 4];
        output[offset + i * 2 + 1] = digits[input[i] & 0x0F];
    }
}

std::string toHex(const std::vector<u8>& input) {
    std::string output;
    appendHex(input.data(), input.size(), output);
    return output;
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return value - 'A' + 10;
    }

    return -1;
}

bool fromHex(const std::string& input, std::vector<u8>& output) {
    if ((input.size() % 2) != 0) {
        return false;
    }

    output.resize(input.size() / 2);
    for (size_t i = 0; i < input.size(); i += 2) {
        const int high = hexValue(input[i]);
        const int low = hexValue(input[i + 1]);
        if (high < 0 || low < 0) {
            output.clear();
            return false;
        }

        output[i / 2] = static_cast<u8>((high << 4) | low);
    }

    return true;
}

void appendBytes(std::vector<u8>& target, const std::vector<u8>& source) {
    target.insert(target.end(), source.begin(), source.end());
}

void appendBytes(std::vector<u8>& target, const u8* input, size_t size) {
    target.insert(target.end(), input, input + size);
}

bool hashBytes(PairHash hash, const std::vector<u8>& input, std::vector<u8>& output) {
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(mdType(hash));
    if (!info) {
        return false;
    }

    output.resize(hashLength(hash));
    return mbedtls_md(info, input.data(), input.size(), output.data()) == 0;
}

bool sha256(const std::vector<u8>& input, std::vector<u8>& output) {
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info) {
        return false;
    }

    output.resize(32);
    return mbedtls_md(info, input.data(), input.size(), output.data()) == 0;
}

bool generateAesKey(PairHash hash, const std::vector<u8>& salt, const std::string& pin,
                    std::vector<u8>& aesKey) {
    std::vector<u8> saltedPin = salt;
    appendBytes(saltedPin, reinterpret_cast<const u8*>(pin.data()), pin.size());

    std::vector<u8> hashed;
    if (!hashBytes(hash, saltedPin, hashed) || hashed.size() < AesKeyBytes) {
        return false;
    }

    aesKey.assign(hashed.begin(), hashed.begin() + AesKeyBytes);
    return true;
}

bool aesEcbCrypt(AesOperation operation, const std::vector<u8>& input, const std::vector<u8>& key,
                 std::vector<u8>& output) {
    const bool encrypt = static_cast<bool>(operation);

    if (key.size() != AesKeyBytes) {
        return false;
    }

    if (!encrypt && (input.size() % AesBlockBytes) != 0) {
        return false;
    }

    const size_t paddedSize =
        encrypt ? (input.size() + AesBlockBytes - 1) & ~(AesBlockBytes - 1) : input.size();
    output.assign(paddedSize, 0);
    std::copy(input.begin(), input.end(), output.begin());

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int result = encrypt ? mbedtls_aes_setkey_enc(&aes, key.data(), AesKeyBytes * 8)
                         : mbedtls_aes_setkey_dec(&aes, key.data(), AesKeyBytes * 8);
    for (size_t offset = 0; result == 0 && offset < output.size(); offset += AesBlockBytes) {
        result = mbedtls_aes_crypt_ecb(&aes, encrypt ? MBEDTLS_AES_ENCRYPT : MBEDTLS_AES_DECRYPT,
                                       output.data() + offset, output.data() + offset);
    }

    mbedtls_aes_free(&aes);
    return result == 0;
}

tinyxml2::XMLElement* findElement(tinyxml2::XMLElement* element, const char* name) {
    if (!element) {
        return nullptr;
    }
    if (std::strcmp(element->Name(), name) == 0) {
        return element;
    }

    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child;
         child = child->NextSiblingElement()) {
        tinyxml2::XMLElement* found = findElement(child, name);
        if (found) {
            return found;
        }
    }

    return nullptr;
}

moonlight::GameStreamResult loadPairDocument(const std::string& response,
                                             tinyxml2::XMLDocument& document) {
    tinyxml2::XMLError error = document.Parse(response.c_str(), response.size());
    if (error != tinyxml2::XML_SUCCESS) {
        return moonlight::GameStreamResult::ParseError;
    }

    tinyxml2::XMLElement* root = document.RootElement();
    if (!root) {
        return moonlight::GameStreamResult::ParseError;
    }

    int statusCode = 0;
    error = root->QueryIntAttribute("status_code", &statusCode);
    if (error != tinyxml2::XML_SUCCESS) {
        return moonlight::GameStreamResult::MissingField;
    }

    return statusCode == 200 ? moonlight::GameStreamResult::Ok
                             : moonlight::GameStreamResult::StatusError;
}

bool readDocumentText(tinyxml2::XMLDocument& document, const char* name, std::string& value) {
    value.clear();
    tinyxml2::XMLElement* element = findElement(document.RootElement(), name);
    if (!element) {
        return false;
    }

    const char* text = element->GetText();
    value = text ? text : "";
    return true;
}

bool readPaired(tinyxml2::XMLDocument& document) {
    std::string value;
    return readDocumentText(document, "paired", value) && value == "1";
}

moonlight::GameStreamResult parsePairedResponse(const std::string& response,
                                                PairedResponse& parsed) {
    tinyxml2::XMLDocument document;
    moonlight::GameStreamResult error = loadPairDocument(response, document);
    if (error != moonlight::GameStreamResult::Ok) {
        return error;
    }

    parsed.paired = readPaired(document);
    return moonlight::GameStreamResult::Ok;
}

moonlight::GameStreamResult parseServerCertificateResponse(const std::string& response,
                                                           ServerCertificateResponse& parsed) {
    tinyxml2::XMLDocument document;
    moonlight::GameStreamResult error = loadPairDocument(response, document);
    if (error != moonlight::GameStreamResult::Ok) {
        return error;
    }

    parsed.paired = readPaired(document);
    if (!parsed.paired) {
        return moonlight::GameStreamResult::Ok;
    }

    return readDocumentText(document, "plaincert", parsed.plaincertHex)
               ? moonlight::GameStreamResult::Ok
               : moonlight::GameStreamResult::MissingField;
}

moonlight::GameStreamResult parseChallengeResponse(const std::string& response,
                                                   ChallengeResponse& parsed) {
    tinyxml2::XMLDocument document;
    moonlight::GameStreamResult error = loadPairDocument(response, document);
    if (error != moonlight::GameStreamResult::Ok) {
        return error;
    }

    parsed.paired = readPaired(document);
    if (!parsed.paired) {
        return moonlight::GameStreamResult::Ok;
    }

    return readDocumentText(document, "challengeresponse", parsed.challengeResponseHex)
               ? moonlight::GameStreamResult::Ok
               : moonlight::GameStreamResult::MissingField;
}

moonlight::GameStreamResult parsePairingSecretResponse(const std::string& response,
                                                       PairingSecretResponse& parsed) {
    tinyxml2::XMLDocument document;
    moonlight::GameStreamResult error = loadPairDocument(response, document);
    if (error != moonlight::GameStreamResult::Ok) {
        return error;
    }

    parsed.paired = readPaired(document);
    if (!parsed.paired) {
        return moonlight::GameStreamResult::Ok;
    }

    return readDocumentText(document, "pairingsecret", parsed.pairingSecretHex)
               ? moonlight::GameStreamResult::Ok
               : moonlight::GameStreamResult::MissingField;
}

bool certificateDerToPem(const std::vector<u8>& der, std::string& pem) {
    std::vector<u8> buffer(PemBufferBytes);
    size_t written = 0;
    int result =
        mbedtls_pem_write_buffer("-----BEGIN CERTIFICATE-----\n", "-----END CERTIFICATE-----\n",
                                 der.data(), der.size(), buffer.data(), buffer.size(), &written);
    if (result != 0 || written == 0) {
        return false;
    }

    pem.assign(reinterpret_cast<const char*>(buffer.data()), written - 1);
    return true;
}

bool parseHostCertificate(const std::vector<u8>& bytes, mbedtls_x509_crt& certificate,
                          std::string& pem, std::vector<u8>& der) {
    static const char pemMarker[] = "-----BEGIN CERTIFICATE-----";

    // Hosts in the Moonlight ecosystem expose plaincert as hex-encoded X.509
    // material. Apollo sends PEM bytes, while other hosts may send DER bytes.
    std::string pemText(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    if (pemText.find(pemMarker) != std::string::npos) {
        int result = mbedtls_x509_crt_parse(&certificate,
                                            reinterpret_cast<const unsigned char*>(pemText.c_str()),
                                            pemText.size() + 1);
        if (result != 0 || !certificate.raw.p || certificate.raw.len == 0) {
            return false;
        }

        pem = pemText;
        der.assign(certificate.raw.p, certificate.raw.p + certificate.raw.len);
        return true;
    }

    int result = mbedtls_x509_crt_parse_der(&certificate, bytes.data(), bytes.size());
    if (result != 0) {
        return false;
    }

    der = bytes;
    return certificateDerToPem(der, pem);
}

bool loadClientCertificate(const moonlight::ClientIdentity& identity,
                           mbedtls_x509_crt& certificate) {
    return mbedtls_x509_crt_parse(
               &certificate,
               reinterpret_cast<const unsigned char*>(identity.certificatePem.c_str()),
               identity.certificatePem.size() + 1) == 0;
}

bool signData(const moonlight::ClientIdentity& identity, const std::vector<u8>& data,
              std::vector<u8>& signature) {
    mbedtls_pk_context privateKey;
    mbedtls_pk_init(&privateKey);

    int result = mbedtls_pk_parse_key(
        &privateKey, reinterpret_cast<const unsigned char*>(identity.privateKeyPem.c_str()),
        identity.privateKeyPem.size() + 1, nullptr, 0);

    std::vector<u8> digest;
    size_t signatureLength = 0;
    if (result == 0 && sha256(data, digest)) {
        signature.resize(SignatureBufferBytes);
        result = mbedtls_pk_sign(
            &privateKey, MBEDTLS_MD_SHA256, digest.data(), 0, signature.data(), &signatureLength,
            [](void*, unsigned char* output, size_t size) {
                return R_SUCCEEDED(PS_GenerateRandomBytes(output, size)) ? 0 : -1;
            },
            nullptr);
    }

    mbedtls_pk_free(&privateKey);
    if (result != 0) {
        signature.clear();
        return false;
    }

    signature.resize(signatureLength);
    return true;
}

bool verifySignature(mbedtls_x509_crt& certificate, const std::vector<u8>& data,
                     const std::vector<u8>& signature) {
    std::vector<u8> digest;
    if (!sha256(data, digest)) {
        return false;
    }

    return mbedtls_pk_verify(&certificate.pk, MBEDTLS_MD_SHA256, digest.data(), 0, signature.data(),
                             signature.size()) == 0;
}

PairHash pairHashForVersion(const std::string& appVersion) {
    return std::atoi(appVersion.c_str()) >= 7 ? PairHash::Sha256 : PairHash::Sha1;
}

void unpairQuietly(moonlight::NvClient& client) {
    std::string ignored;
    client.unpair(ignored);
}

PairResult unpairAndFail(moonlight::NvClient& client) {
    unpairQuietly(client);
    return PairFailed{};
}

void updateHostFromServerInfo(moonlight::Host& host, const moonlight::GameStreamServerInfo& info) {
    host.httpsPort = info.httpsPort;
    host.serverCodecModeSupport = info.serverCodecModeSupport;
    host.appversion = info.appversion;
    host.gfeVersion = info.gfeVersion;
    host.gputype = info.gputype;
}

bool isValidPairRequest(const PairRequest& request) {
    return !request.hostAddress.empty() && !request.identity.uniqueId.empty();
}

PairResult completePairingWithPin(const PairRequest& request, moonlight::Host& host,
                                  moonlight::NvClient& client,
                                  const moonlight::GameStreamServerInfo& info,
                                  const std::string& pin) {
    const PairHash hash = pairHashForVersion(info.appversion);

    std::vector<u8> salt;
    std::vector<u8> aesKey;
    if (!randomBytes(salt, SaltBytes) || !generateAesKey(hash, salt, pin, aesKey)) {
        return PairFailed{};
    }

    std::string response;
    moonlight::NvResult nvResult = client.pairGetServerCertificate(toHex(salt), response);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }
    ServerCertificateResponse serverCertificateResponse;
    moonlight::GameStreamResult responseStatus =
        parseServerCertificateResponse(response, serverCertificateResponse);
    if (responseStatus == moonlight::GameStreamResult::MissingField) {
        return responseStatus;
    }
    if (responseStatus != moonlight::GameStreamResult::Ok) {
        return PairFailed{};
    }
    if (!serverCertificateResponse.paired) {
        return PairFailed{};
    }
    if (serverCertificateResponse.plaincertHex.empty()) {
        return PairFailed{};
    }

    std::vector<u8> serverCertificateBytes;
    if (!fromHex(serverCertificateResponse.plaincertHex, serverCertificateBytes)) {
        unpairQuietly(client);
        return moonlight::GameStreamResult::InvalidField;
    }

    std::vector<u8> serverCertificateSignature;
    {
        mbedtls_x509_crt serverCertificate;
        mbedtls_x509_crt_init(&serverCertificate);
        const bool parsed = parseHostCertificate(serverCertificateBytes, serverCertificate,
                                                 host.plaincert, host.plaincertDer);
        if (parsed) {
            serverCertificateSignature.assign(serverCertificate.sig.p,
                                              serverCertificate.sig.p + serverCertificate.sig.len);
        }
        mbedtls_x509_crt_free(&serverCertificate);
        if (!parsed) {
            unpairQuietly(client);
            return PairFailed{};
        }
    }

    std::vector<u8> clientCertificateSignature;
    {
        mbedtls_x509_crt clientCertificate;
        mbedtls_x509_crt_init(&clientCertificate);
        const bool loaded = loadClientCertificate(request.identity, clientCertificate);
        if (loaded) {
            clientCertificateSignature.assign(clientCertificate.sig.p,
                                              clientCertificate.sig.p + clientCertificate.sig.len);
        }
        mbedtls_x509_crt_free(&clientCertificate);
        if (!loaded) {
            unpairQuietly(client);
            return PairFailed{};
        }
    }

    std::vector<u8> clientChallenge;
    std::vector<u8> encryptedClientChallenge;
    if (!randomBytes(clientChallenge, ChallengeBytes) ||
        !aesEcbCrypt(AesOperation::ENCRYPT, clientChallenge, aesKey, encryptedClientChallenge)) {
        return unpairAndFail(client);
    }

    nvResult = client.pairSendClientChallenge(toHex(encryptedClientChallenge), response);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }
    ChallengeResponse challengeResponse;
    responseStatus = parseChallengeResponse(response, challengeResponse);
    if (responseStatus == moonlight::GameStreamResult::MissingField) {
        return responseStatus;
    }
    if (responseStatus != moonlight::GameStreamResult::Ok) {
        return unpairAndFail(client);
    }
    if (!challengeResponse.paired) {
        return unpairAndFail(client);
    }

    std::vector<u8> encryptedChallengeResponse;
    std::vector<u8> decryptedChallengeResponse;
    if (!fromHex(challengeResponse.challengeResponseHex, encryptedChallengeResponse) ||
        !aesEcbCrypt(AesOperation::DECRYPT, encryptedChallengeResponse, aesKey,
                     decryptedChallengeResponse)) {
        return unpairAndFail(client);
    }

    const size_t responseHashBytes = hashLength(hash);
    if (decryptedChallengeResponse.size() < responseHashBytes + ChallengeBytes) {
        return unpairAndFail(client);
    }

    std::vector<u8> serverResponse(decryptedChallengeResponse.begin(),
                                   decryptedChallengeResponse.begin() + responseHashBytes);
    std::vector<u8> serverChallenge(decryptedChallengeResponse.begin() + responseHashBytes,
                                    decryptedChallengeResponse.begin() + responseHashBytes +
                                        ChallengeBytes);

    std::vector<u8> clientSecret;
    if (!randomBytes(clientSecret, SecretBytes)) {
        return unpairAndFail(client);
    }

    std::vector<u8> serverChallengeMaterial = serverChallenge;
    appendBytes(serverChallengeMaterial, clientCertificateSignature);
    appendBytes(serverChallengeMaterial, clientSecret);

    std::vector<u8> serverChallengeResponse;
    std::vector<u8> encryptedServerChallengeResponse;
    if (!hashBytes(hash, serverChallengeMaterial, serverChallengeResponse) ||
        !aesEcbCrypt(AesOperation::ENCRYPT, serverChallengeResponse, aesKey,
                     encryptedServerChallengeResponse)) {
        return unpairAndFail(client);
    }

    nvResult =
        client.pairSendServerChallengeResponse(toHex(encryptedServerChallengeResponse), response);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }
    PairingSecretResponse pairingSecretResponse;
    responseStatus = parsePairingSecretResponse(response, pairingSecretResponse);
    if (responseStatus == moonlight::GameStreamResult::MissingField) {
        return responseStatus;
    }
    if (responseStatus != moonlight::GameStreamResult::Ok) {
        return unpairAndFail(client);
    }
    if (!pairingSecretResponse.paired) {
        return unpairAndFail(client);
    }

    std::vector<u8> pairingSecret;
    if (!fromHex(pairingSecretResponse.pairingSecretHex, pairingSecret) ||
        pairingSecret.size() <= SecretBytes) {
        return unpairAndFail(client);
    }

    std::vector<u8> serverSecret(pairingSecret.begin(), pairingSecret.begin() + SecretBytes);
    std::vector<u8> serverSecretSignature(pairingSecret.begin() + SecretBytes, pairingSecret.end());
    bool verifiedServerSecret = false;
    {
        mbedtls_x509_crt serverCertificate;
        mbedtls_x509_crt_init(&serverCertificate);
        const int result = mbedtls_x509_crt_parse_der(&serverCertificate, host.plaincertDer.data(),
                                                      host.plaincertDer.size());
        verifiedServerSecret =
            result == 0 && verifySignature(serverCertificate, serverSecret, serverSecretSignature);
        mbedtls_x509_crt_free(&serverCertificate);
    }
    if (!verifiedServerSecret) {
        return unpairAndFail(client);
    }

    std::vector<u8> expectedServerResponseMaterial = clientChallenge;
    appendBytes(expectedServerResponseMaterial, serverCertificateSignature);
    appendBytes(expectedServerResponseMaterial, serverSecret);

    std::vector<u8> expectedServerResponse;
    if (!hashBytes(hash, expectedServerResponseMaterial, expectedServerResponse) ||
        expectedServerResponse.size() != serverResponse.size() ||
        !std::equal(serverResponse.begin(), serverResponse.end(), expectedServerResponse.begin())) {
        unpairQuietly(client);
        return PairPinWrong{};
    }

    std::vector<u8> clientSecretSignature;
    std::vector<u8> clientPairingSecret = clientSecret;
    if (!signData(request.identity, clientSecret, clientSecretSignature)) {
        return unpairAndFail(client);
    }
    appendBytes(clientPairingSecret, clientSecretSignature);

    nvResult = client.pairSendClientPairingSecret(toHex(clientPairingSecret), response);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }
    PairedResponse finalResponse;
    responseStatus = parsePairedResponse(response, finalResponse);
    if (responseStatus != moonlight::GameStreamResult::Ok || !finalResponse.paired) {
        return unpairAndFail(client);
    }

    return PairSuccess{host};
}

} // namespace

void PairTask::operator()() {
    while (std::optional<PairRequest> request = requests_.receive()) {
        results_.send(PairStarted{});

        if (!isValidPairRequest(*request)) {
            results_.send(PairInvalidRequest{});
            continue;
        }

        PairResult result = pair(*request);
        results_.send(std::move(result));
    }

    results_.close();
}

PairResult PairTask::pair(PairRequest& request) {
    moonlight::Host host;
    host.address = request.hostAddress;
    moonlight::NvClientConfig config{host, request.identity.uniqueId};
    moonlight::NvClient client(config, request.identity);

    std::string response;
    moonlight::NvResult nvResult = client.serverInfo(false, response);
    if (!moonlight::succeeded(nvResult)) {
        return nvResult;
    }

    moonlight::GameStreamServerInfo info;
    moonlight::GameStreamResult responseStatus = moonlight::parseServerInfo(response, info);
    if (responseStatus != moonlight::GameStreamResult::Ok) {
        return responseStatus;
    }

    return pairWithServerInfo(request, host, client, info);
}

PairResult PairTask::pairWithServerInfo(PairRequest& request, moonlight::Host& host,
                                        moonlight::NvClient& client,
                                        const moonlight::GameStreamServerInfo& info) {
    updateHostFromServerInfo(host, info);
    if (info.paired) {
        return PairSuccess{host};
    }

    const std::string pin = generatePin();
    results_.send(PairPinRequired{pin});

    return completePairingWithPin(request, host, client, info, pin);
}

const char* toString(const PairResult& result) {
    if (std::holds_alternative<PairStarted>(result)) {
        return "started";
    }
    if (std::holds_alternative<PairSuccess>(result)) {
        return "ok";
    }
    if (std::holds_alternative<PairInvalidRequest>(result)) {
        return "invalid-request";
    }
    if (std::holds_alternative<moonlight::NvResult>(result)) {
        return "nvclient-error";
    }
    if (std::holds_alternative<moonlight::GameStreamResult>(result)) {
        return "response-error";
    }
    if (std::holds_alternative<PairPinRequired>(result)) {
        return "pin-required";
    }
    if (std::holds_alternative<PairPinWrong>(result)) {
        return "pin-wrong";
    }
    if (std::holds_alternative<PairFailed>(result)) {
        return "failed";
    }

    return "unknown";
}

} // namespace task
} // namespace lunar3d
