#include "server_info_storage.h"
#include "errors.h"

#include <kernel/common_server/server/config.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <util/string/split.h>


namespace NServiceMonitor {
    void TServerInfoStorageConfig::Init(const TYandexConfig::Section* section) {
        ConnectionLiveTime = section->GetDirectives().Value<TDuration>("ConnectionLiveTime", ConnectionLiveTime);

        auto children = section->GetAllChildren();
        {
            auto it = children.find("DynamicHostAPI");
            VERIFY_WITH_LOG(it != children.end(), "Not found required config section \"DynamicHostAPI\".");
            DynamicHostAPIConfig.Init(it->second, /* requestPolicy */ nullptr);
        }
    }

    void TServerInfoStorageConfig::ToString(IOutputStream& os) const {
        os << "ConnectionLiveTime: " << ConnectionLiveTime << Endl;
        os << "<DynamicHostAPI>" << Endl;
        DynamicHostAPIConfig.ToString(os);
        os << "</DynamicHostAPI>" << Endl;
    }
}

namespace {
    class TPodServerInfoRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    public:
        bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetUri("/")
                .AddHeader("accept", "application/json")
                .AddCgiData("command=get_info_server");
            return true;
        }

        class TResponse: public TJsonResponse {
            CSA_DEFAULT(TResponse, NServiceMonitor::NProto::TServerInfo, ServerInfo);
            CS_ACCESS(TResponse, NJson::TJsonValue, Error, NJson::JSON_NULL);
        protected:
            bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                NServiceMonitor::NProto::TServerInfoContainer protoServerInfoContainer;
                NProtobufJson::Json2Proto(json, protoServerInfoContainer, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));

                ServerInfo = std::move(*protoServerInfoContainer.MutableResult());

                TStringBuf svnRoot = ServerInfo.GetSvnRoot();
                if (svnRoot.SkipPrefix("svn://arcadia.yandex.ru/arc/tags/")) {
                    TStringBuf svnTag;
                    if (svnRoot.NextTok('/') && svnRoot.NextTok('/', svnTag)) {
                        ServerInfo.SetSvnTag(TString(svnTag));
                    }
                }
                return true;
            }

            bool DoParseJsonError(const NJson::TJsonValue& json) override {
                Error["response_code"] = GetCode();
                Error["response_body"] = json;
                return true;
            }
        };
    };

    TString GetSchemeHostPort(const TAtomicSharedPtr<NExternalAPI::TSender>& sender) {
        if (!sender) {
            return "";
        }
        return TStringBuilder() << (sender->GetConfig().GetIsHttps() ? "https" : "http") << "://" << sender->GetConfig().GetHost() << ":" << sender->GetConfig().GetPort();
    }
}

namespace NServiceMonitor {
    TServerInfoStorage::TServerInfoStorage(const TServerInfoStorageConfig& config)
        : Config(config)
        , ConnectionsCache(Config.GetConnectionLiveTime())
    {
    }

    TAtomicSharedPtr<NExternalAPI::TSender> TServerInfoStorage::GetSender(const TString& podHostname, const ui16 controllerPort) const {
        auto senderConfig = Config.GetDynamicHostAPIConfig();
        senderConfig.SetHost(podHostname);
        if (controllerPort != 0) {
            senderConfig.SetPort(controllerPort);
        }

        const TString key = senderConfig.GetHost() + ":" + ToString(senderConfig.GetPort());

        TAtomicSharedPtr<NExternalAPI::TSender> result;
        if (TMaybe<TAtomicSharedPtr<NExternalAPI::TSender>> maybeSender = ConnectionsCache.GetValue(key)) {
            result = std::move(maybeSender).GetRef();
            ConnectionsCache.EraseExpired();
        } else {
            result = MakeAtomicShared<NExternalAPI::TSender>(senderConfig, key);
            auto copy = result;
            ConnectionsCache.PutValue(key, std::move(copy));
        }

        return result;
    }

    TVector<NProto::TServerInfo> TServerInfoStorage::GetServerInfo(const TVector<TPodLocator>& podLocators) const  {
        TVector<NProto::TServerInfo> serverInfos;

        TVector<TAtomicSharedPtr<NExternalAPI::TSender>> keepAliveSenders;
        TVector<NThreading::TFuture<TPodServerInfoRequest::TResponse>> futureResponses;
        for (auto&& podLocator : podLocators) {
            auto podClient = GetSender(podLocator.GetContainerHostname(), podLocator.GetControllerPort());
            keepAliveSenders.emplace_back(podClient);
            futureResponses.emplace_back(podClient->SendRequestAsync<TPodServerInfoRequest>());
        }

        const TInstant now = Now();
        for (size_t i = 0; i < futureResponses.size(); ++i) {
            auto& sender = keepAliveSenders[i];
            auto& podLocator = podLocators[i];
            auto& futureResponse = futureResponses[i];

            // In case of error keep empty value.
            serverInfos.emplace_back();

            const auto gApiContext = TFLRecords::StartContext().Method("TServerInfoStorage::GetServerInfo")
                ("api_address", GetSchemeHostPort(sender))
                ("pod_locator", podLocator.SerializeToJson())
                ("api_call", "TPodServerInfoRequest");

            const TDuration timeout = sender->GetConfig().GetRequestConfig().GetGlobalTimeout();
            if (!futureResponse.Wait(now + timeout)) {
                TFLEventLog::Error("Timeout while waiting for response from Pod.")
                    ("api_timeout", timeout);
                continue;
            }
            if (futureResponse.HasException() || !futureResponse.HasValue()) {
                TFLEventLog::Error("Exception in Future from Pod")
                    ("api_exception", SerializeFutureExceptionToJson(futureResponse));
                continue;
            }

            TPodServerInfoRequest::TResponse response = futureResponse.ExtractValue();
            if (!response.GetError().IsNull() || !response.IsSuccess()) {
                TFLEventLog::Error("Error response from Pod")
                    ("api_error", response.GetError());
                continue;
            }

            serverInfos.back() = response.GetServerInfo();
        }

        return serverInfos;
    }
}
