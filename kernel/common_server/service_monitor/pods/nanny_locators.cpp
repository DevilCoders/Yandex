#include "nanny_locators.h"
#include "errors.h"

#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/abstract/external_api.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <library/cpp/protobuf/json/json2proto.h>


namespace NServiceMonitor {
    TNannyPodLocatorsStorage::TNannyPodLocatorsStorage(const NCS::IExternalServicesOperator& externalServicesOperator)
        : NannyClient(externalServicesOperator.GetSenderPtr(TypeStr()))
    {
        VERIFY_WITH_LOG(NannyClient, "Server config is incomplete, absent External API section \"%s\".", TypeStr().c_str());
    }

    class TNannyServiceCurrentStateRequest: public NExternalAPI::IHttpRequestWithJsonReport {
    private:
        CSA_READONLY_DEF(TString, Service);
    public:
        TNannyServiceCurrentStateRequest(const TString& service)
            : Service(service)
        {
        }

        bool BuildHttpRequest(NNeh::THttpRequest& request) const override {
            request.SetUri("/v2/services/" + Service + "/current_state/instances/")
                .AddHeader("accept", "application/json");
            return true;
        }

        class TResponse: public TJsonResponse {
            CSA_DEFAULT(TResponse, NServiceMonitor::NProto::TServiceInstanceInfoContainer, InstanceInfoContainer);
            CS_ACCESS(TResponse, NJson::TJsonValue, Error, NJson::JSON_NULL);
        protected:
            bool DoParseJsonReply(const NJson::TJsonValue& json) override {
                NProtobufJson::Json2Proto(json, InstanceInfoContainer, NProtobufJson::TJson2ProtoConfig().SetUseJsonName(true));
                return true;
            }
            bool DoParseJsonError(const NJson::TJsonValue& json) override {
                Error["response_code"] = GetCode();
                Error["response_body"] = json;
                return true;
            }
        };
    };

    TVector<TPodLocator> TNannyPodLocatorsStorage::GetPodLocators(const TVector<TServiceName>& services) const {
        TVector<TPodLocator> result;

        const auto gMethodAndApi = TFLRecords::StartContext().Method("TNannyPodLocatorsStorage::GetPodLocators")("api_call", "TNannyServiceCurrentStateRequest");

        TMap<TServiceName, NThreading::TFuture<TNannyServiceCurrentStateRequest::TResponse>> serviceStateFutureResponses;
        for (auto&& serviceName : MakeSet(services)) {
            serviceStateFutureResponses.emplace(serviceName, NannyClient->SendRequestAsync<TNannyServiceCurrentStateRequest>(serviceName));
        }

        TVector<TServiceName> failedServices;
        const TInstant deadline = NannyClient->GetConfig().GetRequestConfig().CalcRequestDeadline();
        for (auto&& [serviceName, futureNannyResponse] : serviceStateFutureResponses) {
            const auto gApiArg = TFLRecords::StartContext()("api_arg", serviceName);

            if (!futureNannyResponse.Wait(deadline)) {
                TFLEventLog::Error("Timeout of while waiting for response from Nanny")("api_timeout", NannyClient->GetConfig().GetRequestConfig().GetGlobalTimeout());
                failedServices.emplace_back(serviceName);
                continue;
            }

            if (futureNannyResponse.HasException() || !futureNannyResponse.HasValue()) {
                TFLEventLog::Error("Exception in Future from Nanny.")("api_exception", SerializeFutureExceptionToJson(futureNannyResponse));
                failedServices.emplace_back(serviceName);
                continue;
            }

            auto response = futureNannyResponse.ExtractValue();
            if (!response.GetError().IsNull()) {
                TFLEventLog::Error("Error response from Nanny.")("api_error", response.GetError());
                failedServices.emplace_back(serviceName);
                continue;
            }

            for (auto&& instanceInfo : response.GetInstanceInfoContainer().GetResult()) {
                TPodLocator podLocator;
                if (podLocator.DeserializeFrom(instanceInfo, GetDeploySystem(), serviceName)) {
                    result.emplace_back(std::move(podLocator));
                } else {
                    TFLEventLog::Error("Invalid value in response from Nanny. Can't deserialize TPodLocator from TInstanceInfo.");
                }
            }
        }

        UpdateCache(result);
        FillFromCacheForFailed(result, failedServices);

        return result;
    }

    void TNannyPodLocatorsStorage::UpdateCache(const TVector<TPodLocator>& podLocators) const {
        TGuard<TMutex> g(CachedServicePodLocatorsMutex);
        for (auto&& podLocator : podLocators) {
            CachedServicePodLocators[podLocator.GetService()].clear();
        }
        for (auto&& podLocator : podLocators) {
            CachedServicePodLocators[podLocator.GetService()].emplace_back(podLocator);
        }
    }

    void TNannyPodLocatorsStorage::FillFromCacheForFailed(TVector<TPodLocator>& podLocators, const TVector<TServiceName>& failedServices) const {
        TGuard<TMutex> g(CachedServicePodLocatorsMutex);
        for (auto&& failedService : failedServices) {
            auto it = CachedServicePodLocators.find(failedService);
            if (it != CachedServicePodLocators.end()) {
                for (auto&& podLocator : it->second) {
                    podLocators.emplace_back(podLocator);
                }
            }
        }
    }

    EDeploySystem TNannyPodLocatorsStorage::GetDeploySystem() const {
        return Type();
    }
}
