#include "resources_storage.h"

#include <kernel/common_server/service_monitor/server/config.h>
#include <kernel/common_server/util/algorithm/container.h>


namespace NServiceMonitor {
    TYpResourcesInfoStorage::TYpResourcesInfoStorage(const TYpServicesOperator& ypServices)
        : YpServices(ypServices)
    {
    }

    TVector<NProto::TResourceRequests> TYpResourcesInfoStorage::GetResourcesInfo(const TVector<TPodLocator>& podLocators) const {
        TVector<NProto::TResourceRequests> result;
        result.resize(podLocators.size());

        TMap<TPodLocator, size_t> locatorsArgumentsIndex;
        for (size_t i = 0; i < podLocators.size(); ++i) {
            locatorsArgumentsIndex[podLocators[i]] = i;
        }

        TMap<TDataCenter, TVector<TPodLocator>> dcToPodLocators;
        for (auto&& podLocator : podLocators) {
            dcToPodLocators[podLocator.GetDataCenter()].emplace_back(podLocator);
        }

        TVector<TAtomicSharedPtr<NYP::NClient::TClient>> keepAliveYpClients;
        TDuration timeout = {};
        TMap<TDataCenter, NThreading::TFuture<TVector<NYP::NClient::TSelectorResult>>> dcToFuturePodSpecs;
        for (auto&& [dataCenter, podLocators] : dcToPodLocators) {
            TVector<TString> dcLocalPodIds;
            for (auto&& podLocator : podLocators) {
                dcLocalPodIds.emplace_back(podLocator.GetDcLocalPodId());
            }

            auto ypClient = YpServices.GetClient(dataCenter);
            if (!ypClient) {
                TFLEventLog::Error("Unknown YP datacenter: \"" + dataCenter + "\".")("yp_datacenter", dataCenter);
                continue;
            }
            timeout = Max(timeout, ypClient->Options().Timeout());
            keepAliveYpClients.emplace_back(ypClient);

            NThreading::TFuture<TVector<NYP::NClient::TSelectorResult>> ypFutureReponse;
            try {
                ypFutureReponse = ypClient->GetObjects<NYP::NClient::TPod>(dcLocalPodIds, {"/spec/resource_requests"});
            } catch (...) {
                ypFutureReponse = NThreading::MakeErrorFuture<TVector<NYP::NClient::TSelectorResult>>(std::current_exception());
            }
            dcToFuturePodSpecs.emplace(dataCenter, ypFutureReponse);
        }

        const TInstant deadline = TInstant::Now() + timeout;
        for (auto&& [dataCenter, futureDcLocalPodSpecs] : dcToFuturePodSpecs) {
            const auto gMethodAndApi = TFLRecords::StartContext().Method("TYpResourcesInfoStorage::GetResourcesInfo")
                ("api_address", YpServices.GetAddress(dataCenter))
                ("api_call", R"(GetObjects<NYP::NClient::TPod>(dcLocalPodIds, {"/spec/resource_requests"})");

            if (!futureDcLocalPodSpecs.Wait(deadline)) {
                TFLEventLog::Error("Timeout of " + timeout.ToString() + " while waiting for response from YP")("api_timeout", timeout);
                continue;
            }

            if (futureDcLocalPodSpecs.HasException() || !futureDcLocalPodSpecs.HasValue()) {
                TFLEventLog::Error("Exception in Future from YP endpoint \"" + YpServices.GetAddress(dataCenter) + "\"")
                    ("api_exception", SerializeFutureExceptionToJson(futureDcLocalPodSpecs));
                continue;
            }

            const auto dcLocalPodLocators = MakeVector(dcToPodLocators[dataCenter]);
            const auto& dcLocalPodSpecs = futureDcLocalPodSpecs.GetValue();
            if (dcLocalPodLocators.size() != dcLocalPodSpecs.size()) {
                TFLEventLog::Error("Collection of results has different size than requested")
                    ("sizes", "dcLocalPodLocators: " + ToString(dcLocalPodLocators.size()) + ", dcLocalPodSpecs: " + ToString(dcLocalPodSpecs.size()));
                continue;
            }

            for (size_t i = 0; i < dcLocalPodSpecs.size(); ++i) {
                const TPodLocator& podLocator = dcLocalPodLocators[i];
                const NYP::NClient::TSelectorResult& podSpec = dcLocalPodSpecs[i];

                NYP::NClient::TPod pod;
                podSpec.Fill(pod.MutableSpec()->mutable_resource_requests());

                NProto::TResourceRequests resourceRequests;
                resourceRequests.SetVcpuLimit(pod.Spec().resource_requests().vcpu_limit());
                resourceRequests.SetMemoryLimit(pod.Spec().resource_requests().memory_limit());

                const auto itIndex = locatorsArgumentsIndex.find(podLocator);
                if (itIndex == locatorsArgumentsIndex.end()) {
                    TFLEventLog::Error("Internal logic error. We somehow received PodLocator which is not in locatorsArgumentsIndex")
                        ("pod_locator", podLocator.SerializeToJson());
                    continue;
                }
                const size_t argumentIndex = itIndex->second;
                result[argumentIndex] = std::move(resourceRequests);
            }
        }
        return result;
    }
}
