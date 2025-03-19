#include "aes.h"

#include <kernel/common_server/library/logging/events.h>

#include <contrib/libs/openssl/include/openssl/aes.h>
#include <contrib/libs/openssl/include/openssl/bn.h>
#include <contrib/libs/openssl/include/openssl/evp.h>
#include <contrib/libs/openssl/include/openssl/hmac.h>
#include <contrib/libs/openssl/include/openssl/rand.h>

#include <library/cpp/openssl/holders/evp.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/string/join.h>

namespace {
    template <class T>
    bool AESImpl(const TString& key, const T& input, TString& output, bool decrypt) {
        TString iv(16, '\0');
        NOpenSSL::TEvpCipherCtx context;
        if (EVP_CipherInit_ex(context, EVP_aes_256_cbc(), nullptr, (unsigned char*)key.data(), (unsigned char*)iv.data(), decrypt ? 0 : 1) != 1) {
            TFLEventLog::Error("AES execution failed")("method", "EVP_CipherInit_ex")("decrypt", ::ToString(decrypt));
            return false;
        }

        ui32 itersCount = (input.Size() / AES_BLOCK_SIZE);
        if (input.size() % AES_BLOCK_SIZE > 0) {
            ++itersCount;
        }
        TBufferOutput result;
        for (ui32 i = 0; i < itersCount; ++i) {
            ui32 shift = i * AES_BLOCK_SIZE;
            int bytes = 0;

            TString buffer(AES_BLOCK_SIZE, '\0');
            if (EVP_CipherUpdate(context, (unsigned char *)buffer.data(), &bytes, (const unsigned char*)input.data() + shift, Min<ui32>(AES_BLOCK_SIZE, input.Size() - shift)) != 1) {
                TFLEventLog::Error("AES execution failed")("method", "EVP_CipherUpdate")("decrypt", ::ToString(decrypt));
                return false;
            }
            result << TStringBuf(buffer.data(), bytes);
        }
        output = TString(result.Buffer().data(), result.Buffer().size());
        output.append(AES_BLOCK_SIZE, '\0');
        int finalBytes = 0;
        if (EVP_CipherFinal_ex(context, (unsigned char*)output.data() + result.Buffer().size() , &finalBytes) != 1) {
            TFLEventLog::Error("AES execution failed")("method", "EVP_CipherFinal_ex")("decrypt", ::ToString(decrypt));
            return false;
        }
        output.resize(result.Buffer().size() + finalBytes);
        return true;
    }

    const unsigned char* GetBasePtr(const TString& data) {
        return (const unsigned char*)data.data();
    }

    unsigned char* GetBasePtr(TString& data) {
        return (unsigned char*)data.data();
    }

    TString RandBytes(size_t size) {
        TString buf(size, 0);

        if (!RAND_bytes((unsigned char*)buf.data(), size)) {
            buf.clear();
        }
        return buf;
    }

    constexpr int TagSize = 16;
}

TString NOpenssl::GenerateAESKey(ui32 len) {
    return RandBytes(len);
}

bool NOpenssl::AESDecrypt(const TString& key, const TString& input, TString& output) {
    return AESImpl(key, input, output, true);
}

bool NOpenssl::AESEncrypt(const TString& key, const TString& input, TString& output) {
    return AESImpl(key, input, output, false);
}

bool NOpenssl::AESEncrypt(const TString& key, const TBuffer& input, TString& output) {
    return AESImpl(key, input, output, false);
}

bool NOpenssl::AESGCMEncrypt(const TString& key, const TString& plain, TString& encrypted, const TString& keyId) {
    NOpenSSL::TEvpCipherCtx ctx;
    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_EncryptInit_ex");
        return false;
    }

    TString iv = RandBytes(16);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr)) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_CTRL_GCM_SET_IVLEN");
        return false;
    }

    if (!EVP_EncryptInit_ex(ctx, nullptr, nullptr, GetBasePtr(key), GetBasePtr(iv))) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_EncryptInit_ex");
        return false;
    }

    auto result = TString(plain.size() + iv.size(), 0);
    int plainLen = 0;
    if (!EVP_EncryptUpdate(ctx, GetBasePtr(result), &plainLen, GetBasePtr(plain), plain.size())) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_EncryptUpdate");
        return false;
    }

    int tailLen = 0;
    if (!EVP_EncryptFinal_ex(ctx, GetBasePtr(result) + plainLen, &tailLen)) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_EncryptFinal_ex");
        return false;
    }

    result.resize(plainLen + tailLen);

    TString tag(TagSize, 0);
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TagSize, GetBasePtr(tag))) {
        TFLEventLog::Error("AES GCM encrypt failed")("method", "EVP_CTRL_GCM_GET_TAG");
        return false;
    }

    TGcmToken gcmToken;
    gcmToken.SetData(result).SetTag(tag).SetIV(iv).SetKeyId(keyId);
    encrypted = gcmToken.ToString();
    return true;
}
bool NOpenssl::AESGCMDecrypt(const TString& key, const TString& encrypted, TString& result) {
    TGcmToken gcmToken;
    gcmToken.FromString(encrypted);
    result = TString(gcmToken.GetData().size(), 0);

    NOpenSSL::TEvpCipherCtx ctx;

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr)) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_DecryptInit_ex");
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, gcmToken.GetIV().size(), nullptr)) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_CTRL_GCM_SET_IVLEN");
        return false;
    }

    if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, GetBasePtr(key), GetBasePtr(gcmToken.GetIV()))) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_DecryptInit_ex");
        return false;
    }

    int plainLen = 0;
    if (!EVP_DecryptUpdate(ctx, GetBasePtr(result), &plainLen, GetBasePtr(gcmToken.GetData()), gcmToken.GetData().size())) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_DecryptUpdate");
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, gcmToken.GetTag().size(), GetBasePtr(gcmToken.MutableTag()))) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_CTRL_GCM_SET_TAG");
        return false;
    }

    int tailLen = 0;
    if (!EVP_DecryptFinal_ex(ctx, GetBasePtr(result) + plainLen, &tailLen)) {
        TFLEventLog::Error("AES GCM decrypt failed")("method", "EVP_DecryptFinal_ex");
        return false;
    }

    result.resize(plainLen + tailLen);
    return true;
}

bool NOpenssl::TGcmToken::FromString(const TString& str) {
    TString keyId, iv, tag, data;
    if (!StringSplitter(str).Split('.').TryCollectInto(&keyId, &iv, &tag, &data)) {
        TFLEventLog::Error("Cannot build gcsm token from string")("input", str);
        return false;
    }
    KeyId = Base64DecodeUneven(keyId);
    IV = Base64DecodeUneven(iv);
    Tag = Base64DecodeUneven(tag);
    Data = Base64DecodeUneven(data);
    return true;
}

TString NOpenssl::TGcmToken::ToString() const {
    return Join('.', Base64Encode(KeyId), Base64Encode(IV), Base64Encode(Tag), Base64Encode(Data));
}

