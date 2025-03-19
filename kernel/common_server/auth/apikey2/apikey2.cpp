#include "apikey2.h"
#include <kernel/common_server/auth/apikey/apikey.h>
#include <search/fetcher/fetcher.h>
#include <util/stream/file.h>


IAuthInfo::TPtr TApikey2AuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    if (!requestContext.Get()->GetCgiParameters().Has(Config->GetKeyParameter())) {
        return MakeAtomicShared<TApikeyAuthInfo>(false, "", HTTP_UNAUTHORIZED, "apikey expected");
    }

    TString apikey = requestContext.Get()->GetCgiParameters().Get(Config->GetKeyParameter());

    TCacheInfo info;
    if (Cache.Find(apikey, info)) {
        ApikeyStatusSignal.Signal(info.Authorized ? EApikeyPassStatus::OKCache : EApikeyPassStatus::RejectCache, 1);
        return MakeAtomicShared<TApikeyAuthInfo>(info.Authorized, apikey, info.Authorized ? HTTP_OK : HTTP_UNAUTHORIZED,
                                   info.Authorized ? "" : "not authorized apikey " + apikey);
    }

    TString url = Config->GetUrl() + "?" + Config->GetAdditionalCgi() + "&" + Config->GetServiceKeyParameter() + "=" + apikey;
    DEBUG_LOG << "RestoreAuthInfo: apikey url=" << url << Endl;

    THttpFetcher fetcher(TSelfFlushLogFramePtr(nullptr), "111");
    const int requestId = fetcher.AddRequestUrl(url.data(), TDuration::Seconds(2));
    if (requestId < 0) {
        DEBUG_LOG << "RestoreAuthInfo: requestId=" << requestId;
        return MakeAtomicShared<TApikeyAuthInfo>(false, apikey, HTTP_UNAUTHORIZED, "error apikey check");
    }
    fetcher.Run(THttpFetcher::TAbortOnTimeout());

    const IRemoteRequestResult* result = fetcher.GetRequestResult(0);
    if (!result) {
        DEBUG_LOG << "RestoreAuthInfo: empty apikey reply" << Endl;
        if (Config->GetPassOn5xx()) {
            return MakeAtomicShared<TApikeyAuthInfo>(true, apikey, HTTP_OK, "");
        }
        return MakeAtomicShared<TApikeyAuthInfo>(false, apikey, HTTP_UNAUTHORIZED, "error apikey check " + apikey);
    }
    DEBUG_LOG << "RestoreAuthInfo: status=" << result->StatusCode() << Endl;
    if (result->StatusCode() != 200) {
        DEBUG_LOG << "RestoreAuthInfo: apikey status " << result->StatusCode() << Endl;
        if (result->StatusCode() >= 500 && Config->GetPassOn5xx()) {
            ApikeyStatusSignal.Signal(EApikeyPassStatus::OK5xx, 1);
            return MakeAtomicShared<TApikeyAuthInfo>(true, apikey, HTTP_OK, "");
        }
        Cache.Update(apikey, false);
        ApikeyStatusSignal.Signal(EApikeyPassStatus::Reject, 1);
        return MakeAtomicShared<TApikeyAuthInfo>(false, apikey, HTTP_UNAUTHORIZED, "not authorized apikey " + apikey);
    }

    Cache.Update(apikey, true);
    DEBUG_LOG << "RestoreAuthInfo: cache updated apikey=" << apikey << Endl;
    ApikeyStatusSignal.Signal(EApikeyPassStatus::OK, 1);
    DEBUG_LOG << "RestoreAuthInfo: signal ok apikey=" << apikey << Endl;
    return MakeAtomicShared<TApikeyAuthInfo>(true, apikey, HTTP_OK, "");
}

THolder<IAuthModule> TApikey2AuthConfig::DoConstructAuthModule(const IBaseServer* /*server*/) const {
    return MakeHolder<TApikey2AuthModule>(this);
}

TApikey2AuthConfig::TFactory::TRegistrator<TApikey2AuthConfig> TApikey2AuthConfig::Registrator("apikey2");

