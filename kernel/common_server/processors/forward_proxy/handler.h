#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/processors/common/forward/agent.h>
#include <kernel/common_server/processors/common/forward/direct.h>

namespace NCS {
    namespace NForwardProxy {
        template <class TProductClass, class TProductConfig>
        class TCommonHandler: public TCommonSystemHandler<TProductClass, TProductConfig> {
        private:
            using TBase = TCommonSystemHandler<TProductClass, TProductConfig>;

            TString UriPrefix;

            TString GetForwardUri() const {
                auto it = TBase::GetUrlParams().find("forward_url");
                if (it != TBase::GetUrlParams().end()) {
                    if (it->second.StartsWith('/')) {
                        return it->second;
                    } else {
                        return "/" + it->second;
                    }
                } else {
                    const TString uriStr = TBase::Context->GetUri();
                    TStringBuf uri(uriStr.data(), uriStr.size());
                    TBase::ReqCheckCondition(uri.SkipPrefix(UriPrefix), HTTP_INTERNAL_SERVER_ERROR, "Invalid handler configuration: " + TString(uri) + " doesn't start with " + UriPrefix);
                    TStringBuilder uriBuilder;
                    if (!uri.StartsWith('/')) {
                        uriBuilder << "/";
                    }
                    uriBuilder << uri;
                    return uriBuilder;
                }
            }

            NNeh::THttpRequest ConstructForwardRequest(TSystemUserPermissions::TPtr permissions) {
                NNeh::THttpRequest forwardRequest;
                forwardRequest.SetUri(GetForwardUri());
                forwardRequest.SetCgiData(TBase::Context->GetCgiParameters().Print());
                TBase::ReqCheckCondition(TuneRequestData(forwardRequest, permissions), HTTP_INTERNAL_SERVER_ERROR, "Tune request failed.");
                for (const auto& [key, value] : TBase::Context->GetBaseRequestData().HeadersIn()) {
                    const TString lowerKey(ToLowerUTF8(key));
                    if (lowerKey == "host" || lowerKey == "content-length") {
                        continue;
                    }
                    forwardRequest.AddHeader(key, value);
                }
                return forwardRequest;
            }

        protected:
            virtual bool TuneRequestData(NNeh::THttpRequest& request, TSystemUserPermissions::TPtr /*permissions*/) {
                if (!TBase::Context->GetBuf().Empty()) {
                    request.SetPostData(TBase::Context->GetBuf());
                }
                return true;
            }

            virtual NCS::NForwardProxy::TReportConstructorContainer BuildReportConstructor() const {
                return new NCS::NForwardProxy::TDirectConstructor();
            }

        public:
            using TConfig = typename TBase::TRegistrationHandlerDefaultConfig;

            TCommonHandler(const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
                : TBase(config, context, authModule, server) {
                const TString handlerName = config.GetHandlerName();
                TStringBuf prefix(handlerName.data(), handlerName.size());
                prefix.ChopSuffix("*");
                UriPrefix = TString(prefix);
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) final {
                const auto forwardRequest = ConstructForwardRequest(permissions);
                auto sender = TBase::GetServer().GetSenderPtr(TBase::Config.GetTargetApiName());
                NCS::NForwardProxy::TAgent agent(sender, BuildReportConstructor());
                agent.ForwardToExternalAPI(g, forwardRequest);
            }
        };

        class THandlerConfig {
            CSA_READONLY_DEF(TString, TargetApiName);
        public:
            virtual bool InitFeatures(const TYandexConfig::Section* section);
            virtual void ToStringFeatures(IOutputStream& os) const;
        };

        template <class TProductClass>
        using THandler = TCommonHandler<TProductClass, THandlerConfig>;
    }
}
