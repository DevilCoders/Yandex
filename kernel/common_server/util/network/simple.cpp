#include "simple.h"

TSimpleAsyncRequestSender::TSimpleAsyncRequestSender(const TConfig& config, const TString& apiName)
    : CommonConfig(config)
{
    if (!!config.GetEventLog()) {
        EventLog = MakeAtomicShared<TEventLog>(config.GetEventLog(), NEvClass::Factory()->CurrentFormat());
    } else {
        EventLog = config.GetRequestConfig().GetEventLog();
    }
    AD = MakeAtomicShared<TAsyncDelivery>();
    Agent = MakeHolder<NNeh::THttpClient>(AD, EventLog.Get());
    Agent->RegisterSource(apiName, config.GetHost(), config.GetPort(), config.GetRequestConfig(), config.GetIsHttps(), config.GetCert(), config.GetCertKey());
    AD->Start(config.GetRequestConfig().GetThreadsStatusChecker(), config.GetRequestConfig().GetThreadsSenders());
}
