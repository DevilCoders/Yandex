#include "fake.h"

IAuthInfo::TPtr TFakeAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    if (Config.GetFixedUserId()) {
        return MakeAtomicShared<TFakeAuthInfo>(Config.GetFixedUserId(), Config.GetServiceId());
    }

    const TString* userId = requestContext->GetBaseRequestData().HeaderIn("Authorization");
    const char* userIdPtr = userId ? userId->c_str() : nullptr;
    if (Config.GetCheckXYandexUid()) {
        const TString* yandexUid = requestContext->GetBaseRequestData().HeaderIn("X-Yandex-Uid");
        if (yandexUid) {
            return MakeAtomicShared<TFakeAuthInfo>(*yandexUid, (userId ? *userId : Config.GetServiceId()));
        }
    }

    if (!userIdPtr) {
        if (requestContext->MutableCgiParameters().Has("user_id")) {
            userIdPtr = requestContext->MutableCgiParameters().Get("user_id").c_str();
        }
    }

    if (!userIdPtr && !Config.GetDefaultUserId().empty()) {
        userIdPtr = Config.GetDefaultUserId().c_str();
    }

    if (userIdPtr) {
        return MakeAtomicShared<TFakeAuthInfo>(userIdPtr, Config.GetServiceId());
    } else {
        return MakeAtomicShared<TFakeAuthInfo>("fake", Config.GetServiceId());
    }
}

TFakeAuthConfig::TFactory::TRegistrator<TFakeAuthConfig> TFakeAuthConfig::Registrator("fake");
