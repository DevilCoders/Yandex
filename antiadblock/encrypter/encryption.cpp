//
// Created by Maxim Khardin on 19/04/2018.
//

#include "encryption.h"

#include <yweb/webdaemons/icookiedaemon/icookie_lib/icookie.h>
#include <yweb/webdaemons/icookiedaemon/icookie_lib/process.h>

#include <cstdint>
#include <ostream>

namespace NAntiAdBlock {
    const TMaybe<TString> encryptCookie(const TStringBuf& value, const NIcookie::TKeysSet& keys) {
        auto encrypter = NIcookie::TIcookieEncrypter(keys);
        NIcookie::TUid uid = NIcookie::TUid();

        if (NIcookie::ValidateYandexuid(value)) {
            GenerateUidFromYandexuid(value, NIcookie::TUid::ESource::Unknown, uid);
            auto encrypted = encrypter.Encrypt(uid);
            const auto result = encrypted.GetUidEncrypted().GetRef();
            return TMaybe<TString>(result);
        } else {
            return TMaybe<TString>();
        }
    }

    const TMaybe<TString> decryptCookie(const TStringBuf& value, const NIcookie::TKeysSet& keys) {
        auto encrypter = NIcookie::TIcookieEncrypter(keys);
        const auto decrypted = encrypter.Decrypt(value);
        if (decrypted.GetUid().Defined()) {
            const auto result = decrypted.GetUid().GetRef();
            return TMaybe<TString>(result.BuildShortRepresentation());
        } else {
            return TMaybe<TString>();
        }
    }

    const TMaybe<TString> decryptCrookieWithDefaultKey(const TStringBuf& value) {
        return decryptCookie(value, NIcookie::GetDefaultYandexKeys());
    }

}
