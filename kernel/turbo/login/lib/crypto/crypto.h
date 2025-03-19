#pragma once
#include <string>

namespace NTurboLogin {
    struct TKeyProvider;

    bool EncryptCryptoBox(const TKeyProvider* keyProvider, uint16_t keyId, const std::string& plainText, std::string& result);
    bool DecryptCryptoBox(const TKeyProvider* keyProvider, const std::string& encryptedB64, std::string& result);

    bool EncryptYuid(const TKeyProvider* keyProvider, uint16_t keyId, const std::string& yuid, const std::string& domain, std::string& encrypted);
    bool DecryptYuid(const TKeyProvider* keyProvider, const std::string& encrypted, std::string& yuid, std::string& domain);

    TKeyProvider* CreateKeyProvider(const std::string& secret);
    void DestroyKeyProvider(TKeyProvider* keyProvider);
}
