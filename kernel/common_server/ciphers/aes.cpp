#include "aes.h"

#include <kernel/common_server/library/openssl/aes.h>

namespace NCS {
    TAesKeyCipherConfig::TFactory::TRegistrator<TAesKeyCipherConfig> TAesKeyCipherConfig::Registrator("aes");

    TAesGcmKeyCipherConfig::TFactory::TRegistrator<TAesGcmKeyCipherConfig> TAesGcmKeyCipherConfig::Registrator("aes-256-gcm");

    bool TAESCipher::Encrypt(const TString& plain, TString& encrypted) const {
        return NOpenssl::AESEncrypt(GetKey().GetValue(), plain, encrypted);
    }

    bool TAESCipher::Decrypt(const TString& encrypted, TString& result) const {
        return NOpenssl::AESDecrypt(GetKey().GetValue(), encrypted, result);
    }

    bool TAESGcmCipher::Encrypt(const TString& plain, TString& encrypted) const {
        return NOpenssl::AESGCMEncrypt(GetKey().GetValue(), plain, encrypted, GetKey().GetId());
    }

    bool TAESGcmCipher::Decrypt(const TString& encrypted, TString& result) const {
        return NOpenssl::AESGCMDecrypt(GetKey().GetValue(), encrypted, result);
    }

    TString TAesGcmKeyEncryptedCipher::GetKeyId(const TString& encrypted) {
        NOpenssl::TGcmToken gcmToken;
        if (!gcmToken.FromString(encrypted)) {
            return "";
        }
        return gcmToken.GetKeyId();
    }
}
