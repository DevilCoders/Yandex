#pragma once
#include <kernel/common_server/library/metasearch/simple/config.h>
#include <library/cpp/yconf/conf.h>
#include <kernel/common_server/library/async_proxy/async_delivery.h>
#include <kernel/common_server/util/accessor.h>
#include "neh.h"
#include <library/cpp/mediator/global_notifications/system_status.h>

class TSimpleAsyncRequestSender {
public:

    struct TConfigDefaults: public NSimpleMeta::TConfigDefaults {
        static TString GetHost() {
            return "";
        }
        static ui32 GetPort() {
            return 80;
        }
        static bool GetIsHttps() {
            return false;
        }
    };

    class TConfig {
        RTLINE_ACCEPTOR_DEF(TConfig, RequestConfig, NSimpleMeta::TConfig);

        RTLINE_ACCEPTOR(TConfig, Host, TString, "");
        RTLINE_ACCEPTOR(TConfig, Port, ui16, 80);
        RTLINE_READONLY_ACCEPTOR(EventLog, TString, "");
        RTLINE_ACCEPTOR(TConfig, IsHttps, bool, false);
        RTLINE_READONLY_ACCEPTOR(Cert, TString, "");
        RTLINE_READONLY_ACCEPTOR(CertKey, TString, "");
        RTLINE_READONLY_ACCEPTOR(RequestPolicyName, TString, "");

    public:
        virtual ~TConfig() = default;

        template <class TDefaults = TConfigDefaults>
        void Init(const TYandexConfig::Section* section, const TMap<TString, NSimpleMeta::TConfig>* requestPolicy = nullptr) {
            if (!section->GetDirectives().GetValue("ApiHost", Host) && !section->GetDirectives().GetValue("Host", Host)) {
                Host = TDefaults::GetHost();
            }
            AssertCorrectConfig(!!Host, "incorrect Host info");
            if (!section->GetDirectives().GetValue("ApiPort", Port) && !section->GetDirectives().GetValue("Port", Port)) {
                Port = TDefaults::GetPort();
            }
            if (!section->GetDirectives().GetValue("Https", IsHttps) && !section->GetDirectives().GetValue("IsHttps", IsHttps)) {
                IsHttps = TDefaults::GetIsHttps();
            }
            Cert = section->GetDirectives().Value<TString>("Cert", Cert);
            CertKey = section->GetDirectives().Value<TString>("CertKey", CertKey);
            EventLog = section->GetDirectives().Value("EventLog", EventLog);

            if (section->GetDirectives().GetValue("RequestPolicyName", RequestPolicyName)) {
                AssertCorrectConfig(!!requestPolicy, "incorrect RequestPolicyName usage withno external policies");
                auto it = requestPolicy->find(RequestPolicyName);
                AssertCorrectConfig(it != requestPolicy->end(), "Incorrect policy %s", RequestPolicyName.data());
                RequestConfig = it->second;
            } else {
                const TYandexConfig::TSectionsMap children = section->GetAllChildren();
                auto it = children.find("RequestConfig");
                if (it != children.end()) {
                    RequestConfig.InitFromSection<TDefaults>(it->second);
                } else {
                    RequestConfig.InitFromSection<TDefaults>(section);
                }
            }
        }

        void ToString(IOutputStream& os) const {
            os << "Host: " << Host << Endl;
            os << "Port: " << Port << Endl;
            os << "Https: " << IsHttps << Endl;
            os << "Cert: " << Cert << Endl;
            os << "CertKey: " << CertKey << Endl;
            os << "EventLog: " << EventLog << Endl;
            if (!!RequestPolicyName) {
                os << "RequestPolicyName: " << RequestPolicyName << Endl;
            } else {
                os << "<RequestConfig>" << Endl;
                RequestConfig.ToString(os);
                os << "</RequestConfig>" << Endl;
            }
        }

        template <class TConfigImpl = TConfig>
        static TConfigImpl ParseFromString(const TString& configStr) {
            TConfigImpl result;
            TAnyYandexConfig config;
            CHECK_WITH_LOG(config.ParseMemory(configStr.data()));
            result.Init(config.GetRootSection(), nullptr);
            return result;
        }
    };

    TSimpleAsyncRequestSender(const TConfig& config, const TString& apiName = "first");

    ~TSimpleAsyncRequestSender() {
        AD->Stop();
    }

    const NNeh::THttpClient* operator->() const {
        return Agent.Get();
    }

    NNeh::THttpClient* operator->() {
        return Agent.Get();
    }

    const TConfig& GetCommonConfig() const {
        return CommonConfig;
    }

private:
    TAsyncDelivery::TPtr AD;
    THolder<NNeh::THttpClient> Agent;
    TAtomicSharedPtr<TEventLog> EventLog;
    TConfig CommonConfig;
};
