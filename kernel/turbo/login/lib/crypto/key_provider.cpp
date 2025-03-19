#include "key_provider.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <contrib/libs/libsodium/include/sodium/core.h>

namespace NTurboLogin {

    TKeyProvider::TKeyProvider()
    {}

    TKeyProvider::TKeyProvider(const std::string& secret)
        : TKeyProvider(secret.data(), secret.size())
    {}

    TKeyProvider::TKeyProvider(const TString& secret)
        : TKeyProvider(secret.data(), secret.size())
    {}

    TKeyProvider::TKeyProvider(const char* secret, size_t secretSize) {
        LoadSecret(secret, secretSize);
    }

    void TKeyProvider::LoadSecret(const TString& secret) {
        LoadSecret(secret.data(), secret.size());
    }

    void TKeyProvider::LoadSecret(const char* secret, size_t secretSize) {
        if (sodium_init() < 0) {
            ythrow yexception() << "unable to init sodium";
        }
        NJson::TJsonValue secretData;
        if (!ReadJsonTree(TStringBuf(secret, secretSize), &secretData)) {
            ythrow yexception() << "secret is not json";
        }
        NJson::TJsonValue keys = secretData["keys"];
        if (!keys.IsArray()) {
            ythrow yexception() << "missing 'keys' in secret";
        }

        StartOffset = 0;
        if (secretData.Has("start_offset")) {
            StartOffset = secretData["start_offset"].GetInteger();
        }

        Keys.clear();
        for (const auto& item : keys.GetArray()) {
            TString key = Base64Decode(item.GetString());
            if (key.Size() != crypto_secretbox_KEYBYTES) {
                ythrow yexception() << "key N=" << Keys.size() << " has wrong size " << key.Size() << " != " << crypto_secretbox_KEYBYTES;
            }
            Keys.emplace_back();
            std::copy(key.begin(), key.begin() + crypto_secretbox_KEYBYTES, Keys.back().begin());
        }

        if (secretData.Has("signature_key")) {
            TString bytes = Base64Decode(secretData["signature_key"].GetString());
            const ui8* start = reinterpret_cast<const ui8*>(bytes.data());
            const ui8* end = start + bytes.size();
            SignatureKey = TVector<ui8>(start, end);
        }
    }


    bool TKeyProvider::GetKey(ui16 keyId, TKey& key) const {
        if (keyId < StartOffset) {
            return false;
        }
        size_t position = keyId - StartOffset;
        if (position < Keys.size()) {
            key = Keys[position];
            return true;
        }
        return false;
    }
    const TVector<ui8>& TKeyProvider::GetSignatureKey() const {
        return SignatureKey;
    }

    ui16 TKeyProvider::GetCurrentKeyId(TInstant/* now */) {
        return 0;
    }
}
