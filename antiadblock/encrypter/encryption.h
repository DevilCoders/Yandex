#pragma once

//
// Created by Maxim Khardin on 19/04/2018.
//

#include <util/generic/fwd.h>

#include <yweb/webdaemons/icookiedaemon/icookie_lib/keys.h>

#include <cstdint>
#include <ostream>

namespace NAntiAdBlock {
    const TMaybe<TString> encryptCookie(const TStringBuf& value, const NIcookie::TKeysSet& keys);
    const TMaybe<TString> decryptCookie(const TStringBuf& value, const NIcookie::TKeysSet& keys);
    const TMaybe<TString> decryptCrookieWithDefaultKey(const TStringBuf& value);
}
