#include "moonlight/client_identity.hpp"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/x509_crt.h>

#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

namespace lunar3d {
namespace moonlight {
namespace {

constexpr size_t UniqueIdBytes = 8;
constexpr size_t UniqueIdChars = 16;
constexpr size_t PemBufferSize = 4096;
constexpr size_t DerBufferSize = 4096;

int psRandom(void*, unsigned char* output, size_t size) {
    return R_SUCCEEDED(PS_GenerateRandomBytes(output, size)) ? 0
                                                             : MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
}

bool isHex(char value) {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') ||
           (value >= 'A' && value <= 'F');
}

void encodeHex(const u8* input, size_t size, std::string& output) {
    static const char digits[] = "0123456789ABCDEF";
    output.resize(size * 2);

    for (size_t i = 0; i < size; i++) {
        output[i * 2] = digits[input[i] >> 4];
        output[i * 2 + 1] = digits[input[i] & 0x0F];
    }
}

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

bool readFile(const std::string& path, std::string& output) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0 || info.st_size < 0) {
        return false;
    }

    FILE* file = fopen(path.c_str(), "rb");
    if (!file) {
        return false;
    }

    output.resize(static_cast<size_t>(info.st_size));
    size_t bytesRead = output.empty() ? 0 : fread(&output[0], 1, output.size(), file);
    bool ok = (bytesRead == output.size()) && !ferror(file);
    fclose(file);

    if (!ok) {
        output.clear();
    }

    return ok;
}

bool writeFile(const std::string& path, const std::string& input) {
    FILE* file = fopen(path.c_str(), "wb");
    if (!file) {
        return false;
    }

    size_t bytesWritten = fwrite(input.data(), 1, input.size(), file);
    bool closed = fclose(file) == 0;
    return bytesWritten == input.size() && closed;
}

bool validUniqueId(const std::string& uniqueId) {
    if (uniqueId.size() != UniqueIdChars) {
        return false;
    }

    for (size_t i = 0; i < uniqueId.size(); i++) {
        if (!isHex(uniqueId[i])) {
            return false;
        }
    }

    return true;
}

ClientIdentityResult loadStoredIdentity(const std::string& directory, ClientIdentity& identity) {
    identity = ClientIdentity();

    if (!readFile(joinPath(directory, "uniqueid.dat"), identity.uniqueId)) {
        return ClientIdentityResult::IoError;
    }

    if (!validUniqueId(identity.uniqueId)) {
        identity = ClientIdentity();
        return ClientIdentityResult::InvalidData;
    }

    if (!readFile(joinPath(directory, "client.pem"), identity.certificatePem) ||
        !readFile(joinPath(directory, "key.pem"), identity.privateKeyPem)) {
        identity = ClientIdentity();
        return ClientIdentityResult::IoError;
    }

    mbedtls_x509_crt certificate;
    mbedtls_pk_context privateKey;
    mbedtls_x509_crt_init(&certificate);
    mbedtls_pk_init(&privateKey);
    int result = 0;
    int keyLength = 0;
    size_t keyOffset = 0;

    result = mbedtls_x509_crt_parse(
        &certificate, reinterpret_cast<const unsigned char*>(identity.certificatePem.c_str()),
        identity.certificatePem.size() + 1);
    if (result != 0)
        goto end;

    result = mbedtls_pk_parse_key(
        &privateKey, reinterpret_cast<const unsigned char*>(identity.privateKeyPem.c_str()),
        identity.privateKeyPem.size() + 1, nullptr, 0);
    if (result != 0)
        goto end;

    identity.privateKeyDer.resize(DerBufferSize);
    keyLength = mbedtls_pk_write_key_der(&privateKey, &identity.privateKeyDer[0],
                                         identity.privateKeyDer.size());
    if (keyLength < 0) {
        result = keyLength;
        goto end;
    }

    keyOffset = identity.privateKeyDer.size() - keyLength;
    identity.certificateDer.assign(certificate.raw.p, certificate.raw.p + certificate.raw.len);
    identity.privateKeyDer.erase(identity.privateKeyDer.begin(),
                                 identity.privateKeyDer.begin() + keyOffset);

end:
    mbedtls_pk_free(&privateKey);
    mbedtls_x509_crt_free(&certificate);

    if (result != 0) {
        identity = ClientIdentity();
        return ClientIdentityResult::InvalidData;
    }

    return ClientIdentityResult::Ok;
}

} // namespace

ClientIdentityResult ClientIdentity::generate(ClientIdentity& identity) {
    identity = ClientIdentity();

    u8 uniqueId[UniqueIdBytes] = {};
    Result randomResult = PS_GenerateRandomBytes(uniqueId, sizeof(uniqueId));
    if (R_FAILED(randomResult)) {
        return ClientIdentityResult::CryptoError;
    }

    encodeHex(uniqueId, sizeof(uniqueId), identity.uniqueId);

    mbedtls_pk_context key;
    mbedtls_x509write_cert certificate;
    mbedtls_mpi serial;
    mbedtls_ctr_drbg_context rng;

    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&certificate);
    mbedtls_mpi_init(&serial);
    mbedtls_ctr_drbg_init(&rng);

    int certificateDerLength = 0;
    int privateKeyDerLength = 0;
    size_t certificateOffset = 0;
    size_t keyOffset = 0;
    size_t certificatePemLength = 0;
    size_t privateKeyPemLength = 0;
    int result = 0;
    u8 serialBytes[16] = {};
    const unsigned char seed[] = "lunar3d-client-identity";

    result = mbedtls_ctr_drbg_seed(&rng, psRandom, nullptr, seed, sizeof(seed) - 1);
    if (result != 0)
        goto end;

    result = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (result != 0)
        goto end;

    result = mbedtls_rsa_gen_key(mbedtls_pk_rsa(key), mbedtls_ctr_drbg_random, &rng, 2048, 65537);
    if (result != 0)
        goto end;

    result = mbedtls_ctr_drbg_random(&rng, serialBytes, sizeof(serialBytes));
    if (result != 0)
        goto end;

    serialBytes[0] &= 0x7F;
    if (serialBytes[0] == 0) {
        serialBytes[0] = 1;
    }

    result = mbedtls_mpi_read_binary(&serial, serialBytes, sizeof(serialBytes));
    if (result != 0)
        goto end;

    mbedtls_x509write_crt_set_version(&certificate, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&certificate, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&certificate, &key);
    mbedtls_x509write_crt_set_issuer_key(&certificate, &key);

    result = mbedtls_x509write_crt_set_serial(&certificate, &serial);
    if (result != 0)
        goto end;

    result = mbedtls_x509write_crt_set_subject_name(&certificate, "CN=NVIDIA GameStream Client");
    if (result != 0)
        goto end;

    result = mbedtls_x509write_crt_set_issuer_name(&certificate, "CN=NVIDIA GameStream Client");
    if (result != 0)
        goto end;

    result = mbedtls_x509write_crt_set_validity(&certificate, "20260101000000", "20460101000000");
    if (result != 0)
        goto end;

    result = mbedtls_x509write_crt_set_basic_constraints(&certificate, 0, -1);
    if (result != 0)
        goto end;

    result = mbedtls_x509write_crt_set_key_usage(
        &certificate, MBEDTLS_X509_KU_DIGITAL_SIGNATURE | MBEDTLS_X509_KU_KEY_ENCIPHERMENT);
    if (result != 0)
        goto end;

    identity.certificatePem.resize(PemBufferSize, '\0');
    identity.privateKeyPem.resize(PemBufferSize, '\0');
    identity.certificateDer.resize(DerBufferSize);
    identity.privateKeyDer.resize(DerBufferSize);

    result = mbedtls_x509write_crt_pem(
        &certificate, reinterpret_cast<unsigned char*>(&identity.certificatePem[0]),
        identity.certificatePem.size(), mbedtls_ctr_drbg_random, &rng);
    if (result != 0)
        goto end;

    result =
        mbedtls_pk_write_key_pem(&key, reinterpret_cast<unsigned char*>(&identity.privateKeyPem[0]),
                                 identity.privateKeyPem.size());
    if (result != 0)
        goto end;

    certificateDerLength =
        mbedtls_x509write_crt_der(&certificate, &identity.certificateDer[0],
                                  identity.certificateDer.size(), mbedtls_ctr_drbg_random, &rng);
    if (certificateDerLength < 0) {
        result = certificateDerLength;
        goto end;
    }

    privateKeyDerLength =
        mbedtls_pk_write_key_der(&key, &identity.privateKeyDer[0], identity.privateKeyDer.size());
    if (privateKeyDerLength < 0) {
        result = privateKeyDerLength;
        goto end;
    }

    certificatePemLength = identity.certificatePem.find('\0');
    privateKeyPemLength = identity.privateKeyPem.find('\0');
    if (certificatePemLength == std::string::npos || privateKeyPemLength == std::string::npos) {
        result = -1;
        goto end;
    }
    identity.certificatePem.resize(certificatePemLength);
    identity.privateKeyPem.resize(privateKeyPemLength);

    certificateOffset = identity.certificateDer.size() - certificateDerLength;
    identity.certificateDer.erase(identity.certificateDer.begin(),
                                  identity.certificateDer.begin() + certificateOffset);

    keyOffset = identity.privateKeyDer.size() - privateKeyDerLength;
    identity.privateKeyDer.erase(identity.privateKeyDer.begin(),
                                 identity.privateKeyDer.begin() + keyOffset);

end:
    mbedtls_ctr_drbg_free(&rng);
    mbedtls_mpi_free(&serial);
    mbedtls_x509write_crt_free(&certificate);
    mbedtls_pk_free(&key);

    if (result != 0) {
        identity = ClientIdentity();
        return ClientIdentityResult::CryptoError;
    }

    return ClientIdentityResult::Ok;
}

ClientIdentityResult ClientIdentity::load(const std::string& directory, ClientIdentity& identity) {
    ClientIdentityResult status = loadStoredIdentity(directory, identity);
    if (status == ClientIdentityResult::Ok) {
        return status;
    }

    if (status != ClientIdentityResult::IoError) {
        return status;
    }

    status = generate(identity);
    if (status != ClientIdentityResult::Ok) {
        return status;
    }

    return store(directory, identity);
}

ClientIdentityResult ClientIdentity::store(const std::string& directory,
                                           const ClientIdentity& identity) {
    if (!ensureDirectoryTree(directory)) {
        return ClientIdentityResult::IoError;
    }

    if (!writeFile(joinPath(directory, "uniqueid.dat"), identity.uniqueId)) {
        return ClientIdentityResult::IoError;
    }

    if (!writeFile(joinPath(directory, "client.pem"), identity.certificatePem) ||
        !writeFile(joinPath(directory, "key.pem"), identity.privateKeyPem)) {
        return ClientIdentityResult::IoError;
    }

    return ClientIdentityResult::Ok;
}

const char* toString(ClientIdentityResult result) {
    switch (result) {
    case ClientIdentityResult::Ok:
        return "ok";
    case ClientIdentityResult::IoError:
        return "io-error";
    case ClientIdentityResult::InvalidData:
        return "invalid-data";
    case ClientIdentityResult::CryptoError:
        return "crypto-error";
    }

    return "unknown";
}

} // namespace moonlight
} // namespace lunar3d
