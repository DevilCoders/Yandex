#include "processor.h"

#include <kernel/common_server/library/cache/cache_with_live_time.h>
#include <kernel/common_server/library/cache/caches_pool.h>

void TAuthRequestProcessor::CheckAuthInfo(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) {
    Y_UNUSED(g);
    const auto& configHttpStatus = BaseServer->GetHttpStatusManagerConfig();
    ReqCheckCondition(!!authInfo, configHttpStatus.UnauthorizedStatus, "null AuthInfo");
    ReqCheckCondition(authInfo->IsAvailable(), configHttpStatus.UnauthorizedStatus, authInfo->GetMessage());
}

namespace {
    TMutex MutexUserHandlerLastRequests;
    TMap<TString, TMap<TString, TInstant>> UserHandlerLastRequests;
}


namespace {
    TCachesPool<TString, TInstant> CachesPool(TDuration::Seconds(10));
}

TString TAuthRequestProcessor::GenerateRequestLink() const {
    const auto guid = CreateGuidAsString();
    TString result;
    for (auto&& ch : guid) {
        if (ch != '-') {
            result += ch;
        }
    }
    return result;
}

void TAuthRequestProcessor::DoProcess(TJsonReport::TGuard& g) {
    IAuthInfo::TPtr authInfo;
    {
        auto gProfile = TFLEventLog::ReqEventLogGuard("restore_auth_info");
        const TInstant startTime = Now();
        authInfo = Auth->RestoreAuthInfo(Context);
        if (IsDebugMode()) {
            g.MutableReport().AddReportElement("d_auth", (Now() - startTime).MicroSeconds());
        }
        CheckAuthInfo(g, authInfo);
    }

    const auto logContext = TFLRecords::StartContext()("auth_service_id", authInfo->GetOriginatorId())("auth_user_id", authInfo->GetUserId());

    {
        double rpsLimit = GetHandlerSettingDef<double>("rps_limit", 0);
        if (rpsLimit > 0) {
            const double secondsOnRequest = 1 / rpsLimit;
            TCacheWithLiveTime<TString, TInstant>& cache = CachesPool.GetCache(GetHandlerName());
            TMaybe<TInstant> pred = cache.GetValue(authInfo->GetUserId());
            if (pred) {
                ReqCheckCondition((Context->GetRequestStartTime() - *pred).MicroSeconds() > secondsOnRequest * 1000000, ConfigHttpStatus.UserErrorState, "too_often_requests");
            }
            cache.PutValue(authInfo->GetUserId(), Context->GetRequestStartTime());
        }
    }

    auto gProcessProfile = TFLEventLog::ReqEventLogGuard("DoAuthProcess");

    DoAuthProcess(g, authInfo);
}

TAuthRequestProcessor::TAuthRequestProcessor(const IAuthRequestProcessorConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr auth, const IBaseServer* server)
    : IRequestProcessor(config, context, server)
    , Auth(auth)
    , AuthConfig(config)
{
    Y_UNUSED(AuthConfig);
}

TString IAuthRequestProcessorConfig::GetAuthModuleName() const {
    return AuthModuleName;
}

IRequestProcessor::TPtr IAuthRequestProcessorConfig::DoConstructProcessor(IReplyContext::TPtr context, const IBaseServer* server) const {
    auto authModuleConfig = server->GetAuthModuleInfo(GetAuthModuleName());
    Y_ENSURE_EX(authModuleConfig, yexception() << "Can't find module for " << GetHandlerName() << "/" << GetAuthModuleName());
    auto authModule = authModuleConfig->ConstructAuthModule(server);
    Y_ENSURE_EX(authModule, yexception() << "Can't construct module for " << GetHandlerName() << "/" << GetAuthModuleName());
    return DoConstructAuthProcessor(context, authModule, server);
}

bool IAuthRequestProcessorConfig::DoInit(const TYandexConfig::Section* section) {
    AuthModuleName = section->GetDirectives().Value<TString>("AuthModuleName", "");
    if (!AuthModuleName) {
        TFLEventLog::Error("incorrect auth module name")("section", section->Name);
        return false;
    }
    return true;
}

void IAuthRequestProcessorConfig::ToString(IOutputStream& os) const {
    IRequestProcessorConfig::ToString(os);
    os << "AuthModuleName: " << AuthModuleName << Endl;
}
