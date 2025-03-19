#include "key_gen.h"
#include <contrib/libs/libsodium/include/sodium/core.h>
#include <contrib/libs/libsodium/include/sodium/crypto_secretbox.h>
#include <contrib/libs/libsodium/include/sodium/randombytes.h>
#include <library/cpp/json/json_writer.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace {
    size_t SIGNATURE_KEY_SIZE = 32;
}

namespace NTurboLogin {

    TString GenerateRandomB64String(size_t size) {
        std::vector<unsigned char> data(size);
        randombytes_buf(data.data(), size);
        TStringBuf keyData((char*)data.data(), data.size());
        return Base64Encode(keyData);
    }

    TString GenerateSecret(size_t keyNum, size_t start_offset, bool pretty) {
        if (sodium_init() < 0) {
            ythrow yexception() << "unable to init sodium";
        }
        NJson::TJsonValue secret;
        for (size_t i = 0; i < keyNum; ++i) {
            std::vector<unsigned char> newKey(crypto_secretbox_keybytes());
            crypto_secretbox_keygen(newKey.data());
            TStringBuf keyData((char*)newKey.data(), crypto_secretbox_keybytes());
            secret["keys"].AppendValue(Base64Encode(keyData));
        }
        secret["start_offset"] = start_offset;
        secret["signature_key"] = GenerateRandomB64String(SIGNATURE_KEY_SIZE);
        return WriteJson(secret, pretty);
    }
}
