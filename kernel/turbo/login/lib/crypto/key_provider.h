#pragma once

#include <contrib/libs/libsodium/include/sodium/crypto_secretbox.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/ysaveload.h>

#include <array>
#include <string>
#include <vector>

namespace NTurboLogin {


    using TKey = std::array<unsigned char, crypto_secretbox_KEYBYTES>;

    struct TKeyProvider {
    public:
        explicit TKeyProvider();
        explicit TKeyProvider(const std::string& secret);
        explicit TKeyProvider(const TString& secret);
        explicit TKeyProvider(const char* secret, size_t secretSize);

        void LoadSecret(const TString& secret);
        void LoadSecret(const char* secret, size_t secretSize);
        bool GetKey(ui16 keyId, TKey& key) const;
        const TVector<ui8>& GetSignatureKey() const;

        static ui16 GetCurrentKeyId(TInstant now);

    private:
        std::vector<TKey> Keys;
        size_t StartOffset = 0;
        TVector<ui8> SignatureKey;
    public:
        Y_SAVELOAD_DEFINE(Keys, StartOffset);
    };
}
