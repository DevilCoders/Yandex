#pragma once

#include <kernel/ugc/security/lib/secret_manager.h>

namespace NUgc::NSecurity {
    TString Hmac(const TSecretManager& secretManager, TStringBuf message);
} // namespace NUgc::NSecurity
