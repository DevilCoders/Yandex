#include "hash_sign.h"

#include <kernel/ugc/security/lib/internal/crypto.h>

#include <util/datetime/base.h>

namespace NUgc::NSecurity {
    TString Hmac(const TSecretManager& secretManager, TStringBuf message) {
        const TString& key = secretManager.GetKeyByDate(TInstant::Now());
        return NInternal::Hmac(key, message);
    }
} // namespace NUgc::NSecurity
