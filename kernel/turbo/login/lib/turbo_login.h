#pragma once

#include <apphost/api/service/cpp/service_context.h>

namespace NTurboLogin {
    void ProcessTurboLogin(NAppHost::IServiceContext& ctx, const TVector<ui8>& signatureSecretKey);
    void ProcessTurboLoginWithBlackbox(NAppHost::IServiceContext& ctx, const TVector<ui8>& signatureSecretKey);
} // namespace NTurboLogin
