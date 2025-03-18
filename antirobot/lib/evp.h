#pragma once

#include <openssl/evp.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NAntiRobot {

constexpr size_t EVP_RECOMMENDED_IV_LEN = 12;


struct TEvpContextDeleter {
    static void Destroy(EVP_CIPHER_CTX* context) {
        EVP_CIPHER_CTX_free(context);
    }
};

using TEvpContext = THolder<EVP_CIPHER_CTX, TEvpContextDeleter>;


// Add new algorithms as necessary.
enum class EEvpAlgorithm {
    Aes256Gcm
};


constexpr size_t EVP_RECOMMENDED_GCM_IV_LEN = 12;


class TEvpEncryptor {
public:
    explicit TEvpEncryptor(EEvpAlgorithm algorithm, TStringBuf key, TStringBuf iv);

    void Update(TStringBuf plaintext, TString* ciphertext);

    void Finalize(TString* ciphertext);

    void Encrypt(TStringBuf plaintext, TString* ciphertext);
    TString Encrypt(TStringBuf plaintext);

private:
    TEvpContext Context;
};


class TEvpDecryptor {
public:
    explicit TEvpDecryptor(EEvpAlgorithm algorithm, TStringBuf key, TStringBuf iv);

    void Update(TStringBuf ciphertext, TString* plaintext);

    void Finalize(TStringBuf tag, TString* plaintext);

    void Decrypt(TStringBuf ciphertextAndTag, TString* plaintext);
    TString Decrypt(TStringBuf ciphertextAndTag);

private:
    TEvpContext Context;
};


} // namespace NAntiRobot
