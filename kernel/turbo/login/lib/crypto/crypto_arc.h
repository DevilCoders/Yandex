#pragma once

#include "key_provider.h"

#include <kernel/turbo/login/lib/crypto/proto/message.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NTurboLogin {
    bool EncryptYuid(const TKeyProvider* keyProvider, uint16_t keyId, const TStringBuf& yuid, const TStringBuf& domain, TString& encrypted);
    bool EncryptYuid(const TKeyProvider* keyProvider, uint16_t keyId, const TPlainText& message, TString& encrypted);

    bool EncryptYuidV1(const TKeyProvider* keyProvider, uint16_t keyId, const TPlainText& message, TString& encrypted);

    bool DecryptYuid(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TString& yuid, TString& domain);
    bool DecryptYuid(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TPlainText& message);

    bool DecryptTimestamp(const TKeyProvider* keyProvider, const TStringBuf& encrypted, TInstant& timestamp);

    TMaybe<TString> GetICookie(const TMaybe<TStringBuf>& cookieIc, const TStringBuf& request, const TKeyProvider& keyProvider);
}
