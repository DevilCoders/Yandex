#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class THttpProxyHandlerConfig {
        CSA_READONLY_DEF(TString, ProxyApiName);
        CSA_READONLY_DEF(TString, ProxyLogin);
        CSA_READONLY_DEF(TString, ProxyPassword);
        CSA_READONLY_DEF(TString, TargetUrl);
        CSA_READONLY_FLAG(NeedConnect, false);
        CSA_READONLY_FLAG(Authorization, true);

    public:
        bool InitFeatures(const TYandexConfig::Section* section);
        void ToStringFeatures(IOutputStream& os) const;
    };

    class THttpProxyHandler : public TCommonSystemHandler<THttpProxyHandler, THttpProxyHandlerConfig> {
    private:
        using TBase = TCommonSystemHandler<THttpProxyHandler, THttpProxyHandlerConfig>;

    public:
        using TConfig = TBase::TRegistrationHandlerDefaultConfig;

    public:
        THttpProxyHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server);
        static TString GetTypeName();
        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;

    private:
        class TAuthRequest;

    private:
        NExternalAPI::TSender::TPtr ProxyClient;
        TString UriPrefix;
    };

}
