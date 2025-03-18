#include "evp.h"

#include <openssl/err.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/maybe.h>
#include <util/generic/yexception.h>

#include <array>


namespace NAntiRobot {


namespace {
    constexpr int GCM_TAG_LEN = 16;


    TStringBuf GetLastOpenSslError(TMaybe<int> err = {}) {
        static thread_local char buf[120];

        if (!err) {
            err = ERR_get_error();
        }

        ERR_error_string_n(*err, buf, sizeof(buf));
        return TStringBuf(buf);
    }

    TEvpContext CreateEvpContext(
        EEvpAlgorithm algorithm,
        TStringBuf key,
        TStringBuf iv,
        int (*init)(
            EVP_CIPHER_CTX*,
            const EVP_CIPHER*,
            ENGINE*,
            const unsigned char *key,
            const unsigned char *iv
        )
    ) {
        TEvpContext context(EVP_CIPHER_CTX_new());
        Y_ENSURE(context, GetLastOpenSslError());

        const EVP_CIPHER* cipher = nullptr;

        switch (algorithm) {
        case EEvpAlgorithm::Aes256Gcm:
            cipher = EVP_aes_256_gcm();
            break;
        }

        const size_t expectedKeyLength = EVP_CIPHER_key_length(cipher);

        Y_ENSURE(
            key.size() == expectedKeyLength,
            "Invalid key length: expected " << expectedKeyLength << ", got " << key.size()
        );

        Y_ENSURE(
            init(&*context, cipher, nullptr, nullptr, nullptr) &&
            EVP_CIPHER_CTX_ctrl(&*context, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr) &&
            init(
                &*context, nullptr, nullptr,
                reinterpret_cast<const unsigned char*>(key.data()),
                reinterpret_cast<const unsigned char*>(iv.data())
            ),
            GetLastOpenSslError()
        );

        return context;
    }

    void UpdateEvp(
        EVP_CIPHER_CTX* context,
        TStringBuf input,
        TString* output,
        int (*update)(
            EVP_CIPHER_CTX*,
            unsigned char*,
            int*,
            const unsigned char*,
            int inl
        )
    ) {
        size_t maxAppendLen = input.size();

        if (const int blockSize = EVP_CIPHER_CTX_block_size(context); blockSize > 1) {
            maxAppendLen += blockSize;
        }

        const size_t oldSize = output->size();
        output->resize(oldSize + maxAppendLen);

        int appendLen = 0;

        if (!update(
            context,
            reinterpret_cast<unsigned char*>(output->begin() + oldSize),
            &appendLen,
            reinterpret_cast<const unsigned char*>(input.data()),
            static_cast<int>(input.size())
        )) {
            output->resize(oldSize);
            ythrow yexception() << GetLastOpenSslError();
        }

        output->resize(oldSize + appendLen);
    }

    void FinalizeEvp(
        EVP_CIPHER_CTX* context,
        TString* output,
        int (*final)(
            EVP_CIPHER_CTX*,
            unsigned char*,
            int*
        )
    ) {
        size_t maxAppendLen = 0;

        if (const int blockSize = EVP_CIPHER_CTX_block_size(&*context); blockSize > 1) {
            maxAppendLen = blockSize;
        }

        const size_t oldSize = output->size();
        output->resize(oldSize + maxAppendLen);

        int appendLen = 0;

        if (!final(
            &*context,
            reinterpret_cast<unsigned char*>(output->begin() + oldSize),
            &appendLen
        )) {
            output->resize(oldSize);
            ythrow yexception() << GetLastOpenSslError();
        }

        output->resize(oldSize + appendLen);
    }
}


TEvpEncryptor::TEvpEncryptor(EEvpAlgorithm algorithm, TStringBuf key, TStringBuf iv)
    : Context(CreateEvpContext(algorithm, key, iv, EVP_EncryptInit_ex))
{}

void TEvpEncryptor::Update(TStringBuf plaintext, TString* ciphertext) {
    UpdateEvp(&*Context, plaintext, ciphertext, EVP_EncryptUpdate);
}

void TEvpEncryptor::Finalize(TString* ciphertext) {
    FinalizeEvp(&*Context, ciphertext, EVP_EncryptFinal_ex);

    std::array<char, GCM_TAG_LEN> tag;

    Y_ENSURE(
        EVP_CIPHER_CTX_ctrl(&*Context, EVP_CTRL_GCM_GET_TAG, GCM_TAG_LEN, tag.data()),
        GetLastOpenSslError()
    );

    ciphertext->append(tag.data(), tag.size());
}

void TEvpEncryptor::Encrypt(TStringBuf plaintext, TString* ciphertext) {
    Update(plaintext, ciphertext);
    Finalize(ciphertext);
}

TString TEvpEncryptor::Encrypt(TStringBuf plaintext) {
    TString ciphertext;
    Encrypt(plaintext, &ciphertext);
    return ciphertext;
}


TEvpDecryptor::TEvpDecryptor(EEvpAlgorithm algorithm, TStringBuf key, TStringBuf iv)
    : Context(CreateEvpContext(algorithm, key, iv, EVP_DecryptInit_ex))
{}

void TEvpDecryptor::Update(TStringBuf ciphertext, TString* plaintext) {
    UpdateEvp(&*Context, ciphertext, plaintext, EVP_DecryptUpdate);
}

void TEvpDecryptor::Finalize(TStringBuf tag, TString* plaintext) {
    Y_ENSURE(
        EVP_CIPHER_CTX_ctrl(
            &*Context, EVP_CTRL_GCM_SET_TAG,
            tag.size(),
            const_cast<char*>(tag.data()) // EVP_CTRL_GCM_SET_TAG doesn't actually write there.
        ),
        GetLastOpenSslError()
    );

    FinalizeEvp(&*Context, plaintext, EVP_DecryptFinal_ex);
}

void TEvpDecryptor::Decrypt(TStringBuf ciphertextAndTag, TString* plaintext) {
    Y_ENSURE(ciphertextAndTag.size() > GCM_TAG_LEN, "Ciphertext is too short");

    TStringBuf ciphertext, tag;
    ciphertextAndTag.SplitAt(ciphertextAndTag.size() - GCM_TAG_LEN, ciphertext, tag);

    Update(ciphertext, plaintext);
    Finalize(tag, plaintext);
}

TString TEvpDecryptor::Decrypt(TStringBuf ciphertextAndTag) {
    TString plaintext;
    Decrypt(ciphertextAndTag, &plaintext);
    return plaintext;
}


} // namespace NAntiRobot
