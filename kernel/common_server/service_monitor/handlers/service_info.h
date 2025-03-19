#pragma once

#include <kernel/common_server/processors/common/handler.h>
#include <util/datetime/base.h>


namespace NServiceMonitor {

    class TServiceInfoHandlerConfig {
        CSA_READONLY_DEF(TVector<TString>, DefaultServiceList);
    public:
        bool InitFeatures(const TYandexConfig::Section* section);
        void ToStringFeatures(IOutputStream& os) const;
    };

    class TServiceInfoHandler: public TRequestHandlerBase<TServiceInfoHandler, TServiceInfoHandlerConfig> {
        using TBase = TRequestHandlerBase<TServiceInfoHandler, TServiceInfoHandlerConfig>;

    public:
        using TConfig = TBase::THandlerConfig;

    public:
        TServiceInfoHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server);

        void ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) override final;

        static TString GetTypeName() {
            return "service_info";
        }

    private:
        using TBase::Config;
    };
}
