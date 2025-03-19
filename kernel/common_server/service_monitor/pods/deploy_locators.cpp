#include "deploy_locators.h"
#include "errors.h"

#include <kernel/common_server/service_monitor/server/config.h>
#include <yp/cpp/yp/client.h>
#include <util/system/env.h>


namespace NServiceMonitor {

    TDeployPodLocatorsStorage::TDeployPodLocatorsStorage(const TYpServicesOperator& ypServices)
        : YpServices(ypServices)
    {
    }


    TVector<TPodLocator> TDeployPodLocatorsStorage::GetPodLocators(const TVector<TStageName>& stages) const {
        using TPodSetId = TString;
        using TReplicaSetId = TString;

        TSet<std::pair<TStageName, TMaybe<TDataCenter>>> failedStages;

        TMap<TStageName, TMap<TDataCenter, TReplicaSetId>> ReplicaSetIdMap;
        TMap<TDataCenter, TMap<TStageName, TSet<TPodSetId>>> podSetIds;

        {
            constexpr char DeployClusterName[] = "deploy";
            auto deployClient = YpServices.GetClient(DeployClusterName);
            if (!deployClient) {
                TFLEventLog::Error("Unknown YP datacenter.")("yp_datacenter", DeployClusterName);
                return GetPodLocatorsFromCache(stages);
            }

            const auto gApiLogContext = TFLRecords::StartContext()
                ("api_call", R"(GetObjects<NYP::NClient::TStage>(stages, {"/status"}))")
                ("api_arg", JoinSeq(", ", stages))
                ("yp_datacenter", DeployClusterName)
                ("api_address", deployClient->Options().Address());

            using TMultiClusterReplicaSetId = TString;
            TMap<TStageName, TMultiClusterReplicaSetId> stageToMultiClusterReplicaSetIds;
            TMap<TMultiClusterReplicaSetId, TStageName> multiClusterReplicaSetIdToStage;

            NThreading::TFuture<TVector<NYP::NClient::TSelectorResult>> ypFutureStagesResults;
            try {
                ypFutureStagesResults = deployClient->GetObjects<NYP::NClient::TStage>(stages, {"/status"});
            } catch (...) {
                ypFutureStagesResults = NThreading::MakeErrorFuture<TVector<NYP::NClient::TSelectorResult>>(std::current_exception());
            }

            const TInstant deadline = TInstant::Now() + deployClient->Options().Timeout();
            if (!ypFutureStagesResults.Wait(deadline)) {
                TFLEventLog::Error("Timeout while waiting for response from YP")
                    ("api_timeout", deployClient->Options().Timeout());
                return GetPodLocatorsFromCache(stages);
            }

            if (ypFutureStagesResults.HasException() || !ypFutureStagesResults.HasValue()) {
                TFLEventLog::Error("Exception in Future from YP.")
                    ("api_exception", SerializeFutureExceptionToJson(ypFutureStagesResults));
                return GetPodLocatorsFromCache(stages);
            }

            const auto& ypStageResults = ypFutureStagesResults.GetValue();

            if (ypStageResults.size() != stages.size()) {
                TFLEventLog::Error("Collection of results has different size than requested")
                    ("sizes", "ypStageResults: " + ToString(ypStageResults.size()) + ", stages: " + ToString(stages.size()));
                return GetPodLocatorsFromCache(stages);
            }

            TMap<TStageName, NYP::NClient::TStage> stageObjects;
            for (size_t i = 0; i < stages.size(); ++i) {
                NYP::NClient::TStage stage;
                ypStageResults[i].Fill(stage.MutableStatus());
                stageObjects[stages[i]] = std::move(stage);
            }

            for (auto&& [stageName, stageObj] : stageObjects) {
                const google::protobuf::Map<TProtoStringType, NYP::NClient::NApi::NProto::TDeployUnitStatus>& deployUnitStatuses = stageObj.Status().deploy_units();

                for (auto&& [deployUnitName, deployUnitStatus] : deployUnitStatuses) {
                    using DetailsCase = NYP::NClient::NApi::NProto::TDeployUnitStatus::DetailsCase;
                    switch (deployUnitStatus.details_case()) {
                        case DetailsCase::kReplicaSet:
                            for (auto&& [clusterId, clusterStatus] : deployUnitStatus.replica_set().cluster_statuses()) {
                                ReplicaSetIdMap[stageName][clusterId] = clusterStatus.replica_set_id();
                            }
                            break;
                        case DetailsCase::kMultiClusterReplicaSet:
                            stageToMultiClusterReplicaSetIds[stageName] = deployUnitStatus.multi_cluster_replica_set().replica_set_id();
                            multiClusterReplicaSetIdToStage[deployUnitStatus.multi_cluster_replica_set().replica_set_id()] = stageName;
                            break;
                        case DetailsCase::DETAILS_NOT_SET:
                            TFLEventLog::Error("DeployUnitStatus has no replica details in TStage /status response.");
                            failedStages.emplace(stageName, Nothing());
                            continue;
                    }
                }
            }

            TMap<TMultiClusterReplicaSetId, NYP::NClient::TMultiClusterReplicaSet> replicaSetObjects;
            do {
                if (stageToMultiClusterReplicaSetIds.empty()) {
                    break;
                }

                constexpr auto AppendKeys = [](TSet<std::pair<TStageName, TMaybe<TDataCenter>>>& set, const auto& stageMap) {
                    for (auto&& [stage, value] : stageMap) {
                        set.emplace(stage, Nothing());
                    }
                };

                const auto multiClusterReplicaSetIds = MakeVector(NContainer::Values(stageToMultiClusterReplicaSetIds));

                const auto gApiLogContext = TFLRecords::StartContext()
                    ("api_call", R"(GetObjects<NYP::NClient::TMultiClusterReplicaSet>(multiClusterReplicaSetIds, {"/status"}))")
                    ("api_arg", JoinSeq(", ", multiClusterReplicaSetIds));

                NThreading::TFuture<TVector<NYP::NClient::TSelectorResult>> ypFutureResponses;
                try {
                    ypFutureResponses = deployClient->GetObjects<NYP::NClient::TMultiClusterReplicaSet>(multiClusterReplicaSetIds, {"/status"});
                } catch (...) {
                    ypFutureResponses = NThreading::MakeErrorFuture<TVector<NYP::NClient::TSelectorResult>>(std::current_exception());
                    AppendKeys(failedStages, stageToMultiClusterReplicaSetIds);
                    break;
                }

                const TInstant deadline = TInstant::Now() + deployClient->Options().Timeout();
                if (!ypFutureResponses.Wait(deadline)) {
                    TFLEventLog::Error("Timeout while waiting for response from YP")
                        ("api_timeout", deployClient->Options().Timeout());
                    AppendKeys(failedStages, stageToMultiClusterReplicaSetIds);
                    break;
                }

                if (ypFutureResponses.HasException() || !ypFutureResponses.HasValue()) {
                    TFLEventLog::Error("Exception in Future from YP.")
                        ("api_exception", SerializeFutureExceptionToJson(ypFutureResponses));
                    AppendKeys(failedStages, stageToMultiClusterReplicaSetIds);
                    break;
                }

                const auto& ypResponses = ypFutureResponses.GetValue();
                if (ypResponses.size() != multiClusterReplicaSetIds.size()) {
                    TFLEventLog::Error("Collection of results has different size than requested")
                        ("sizes", "ypResponses: " + ToString(ypResponses.size()) + ", multiClusterReplicaSetIds: " + ToString(multiClusterReplicaSetIds.size()));
                    AppendKeys(failedStages, stageToMultiClusterReplicaSetIds);
                    break;
                }

                for (size_t i = 0; i < multiClusterReplicaSetIds.size(); ++i) {
                    NYP::NClient::TMultiClusterReplicaSet replicaSet;
                    ypResponses[i].Fill(replicaSet.MutableStatus());
                    replicaSetObjects[multiClusterReplicaSetIds[i]] = std::move(replicaSet);
                }
            } while (false);

            for (auto&& [replicaSetId, replicaSet] : replicaSetObjects) {
                for (auto&& [dataCenter, deployStatus] : replicaSet.Status().cluster_deploy_statuses()) {
                    // `cluster_deploy_statuses` is deprecated, but somehow he is the working one.
                    // see https://a.yandex-team.ru/arc/trunk/arcadia/yp/yp_proto/yp/client/api/proto/multi_cluster_replica_set.proto?rev=r9024053#L115
                    podSetIds[dataCenter][multiClusterReplicaSetIdToStage[replicaSetId]].emplace(deployStatus.pod_set_id());
                }
                for (auto&& [dataCenter, deployStatus] : replicaSet.Status().mapped_cluster_deploy_statuses()) {
                    podSetIds[dataCenter][multiClusterReplicaSetIdToStage[replicaSetId]].emplace(deployStatus.pod_set_id());
                }
            }
        }

        // Continue collecting `podSetIds`:
        {
            const auto gApiLogContext = TFLRecords::StartContext()
                ("api_call", R"(ypClient->GetObject<NYP::NClient::TReplicaSet>(setId, {"/status"}))");

            TMap<TStageName, TMap<TDataCenter, NThreading::TFuture<NYP::NClient::TSelectorResult>>> ypFuturesReplicaSet;
            TDuration timeout = {};
            for (auto&& [stageName, dcToReplicaSetId] : ReplicaSetIdMap) {
                for (auto&& [dataCenter, setId] : dcToReplicaSetId) {
                    auto ypClient = YpServices.GetClient(dataCenter);
                    if (!ypClient) {
                        TFLEventLog::Error("Unknown YP datacenter")("yp_datacenter", dataCenter);
                        failedStages.emplace(stageName, dataCenter);
                        continue;
                    }
                    const auto gApiLogContext = TFLRecords::StartContext()
                        ("yp_datacenter", dataCenter)
                        ("api_address", ypClient->Options().Address());

                    timeout = Max(timeout, ypClient->Options().Timeout());

                    try {
                        auto selectorResult = ypClient->GetObject<NYP::NClient::TReplicaSet>(setId, {"/status"});
                        ypFuturesReplicaSet[stageName][dataCenter] = std::move(selectorResult);
                    } catch (...) {
                        failedStages.emplace(stageName, dataCenter);
                        ypFuturesReplicaSet[stageName][dataCenter] = NThreading::MakeErrorFuture<NYP::NClient::TSelectorResult>(std::current_exception());
                        continue;
                    }
                }
            }

            const TInstant deadline = TInstant::Now() + timeout;
            for (auto&& [stageName, dcToFuturesReplicaSet] : ypFuturesReplicaSet) {
                for (auto&& [dataCenter, futureReplicaSet] : dcToFuturesReplicaSet) {

                    const auto gSpecificApiLogContext = TFLRecords::StartContext()
                        ("yp_datacenter", dataCenter)
                        ("api_address", YpServices.GetAddress(dataCenter))
                        ("api_arg", JoinSeq(", ", ReplicaSetIdMap[stageName][dataCenter]));

                    if (!futureReplicaSet.Wait(deadline)) {
                        TFLEventLog::Error("Timeout while waiting for response from YP")
                            ("api_timeout", timeout);
                        failedStages.emplace(stageName, dataCenter);
                        continue;
                    }

                    if (futureReplicaSet.HasException() || !futureReplicaSet.HasValue()) {
                        TFLEventLog::Error("Exception in Future from YP endpoint")
                            ("api_exception", SerializeFutureExceptionToJson(futureReplicaSet));
                        failedStages.emplace(stageName, dataCenter);
                        continue;
                    }

                    {
                        NYP::NClient::TReplicaSet replicaSet;
                        const auto& ypReplicaSet = futureReplicaSet.GetValue();
                        ypReplicaSet.Fill(replicaSet.MutableStatus());
                        podSetIds[dataCenter][stageName].emplace(replicaSet.Status().deploy_status().pod_set_id());
                    }
                }
            }
        }

        TDuration timeout = {};
        TMap<TDataCenter, TMap<TStageName, TVector<NThreading::TFuture<NYP::NClient::TSelectObjectsResult>>>> ypFutureResponsesMap;
        for (auto&& [dataCenter, stageToPodSetid] : podSetIds) {
            for (auto&& [stageName, podSetIds] : stageToPodSetid) {
                auto ypClient = YpServices.GetClient(dataCenter);
                if (!ypClient) {
                    TFLEventLog::Error("Unknown YP datacenter.")("yp_datacenter", dataCenter);
                    failedStages.emplace(stageName, dataCenter);
                    continue;
                }
                const auto gApiLogContext = TFLRecords::StartContext()
                    ("yp_datacenter", dataCenter)
                    ("api_address", ypClient->Options().Address());

                timeout = Max(timeout, ypClient->Options().Timeout());

                for (auto&& podSetId : podSetIds) {
                    try {
                        auto future = ypClient->SelectObjects(NYP::NClient::NApi::NProto::EObjectType::OT_POD, {"/meta", "/status/dns"}, {"[/meta/pod_set_id]=\"" + podSetId + "\""});
                        ypFutureResponsesMap[dataCenter][stageName].emplace_back(std::move(future));
                    } catch (...) {
                        failedStages.emplace(stageName, dataCenter);
                        ypFutureResponsesMap[dataCenter][stageName].emplace_back(NThreading::MakeErrorFuture<NYP::NClient::TSelectObjectsResult>(std::current_exception()));
                    }
                }
            }
        }

        const TInstant deadline = TInstant::Now() + timeout;
        using TPodId = TString;
        TMap<TDataCenter, TMap<TStageName, TMap<TPodId, NYP::NClient::TPod>>> podsMap;
        for (auto&& [dataCenter, stageToFutures] : ypFutureResponsesMap) {
            for (auto&& [stageName, ypFutureResponses] : stageToFutures) {

                const auto& podSetIdsVec = MakeVector(podSetIds[dataCenter][stageName]);
                for (size_t i = 0; i < ypFutureResponses.size(); ++i) {
                    auto& futureResponse = ypFutureResponses[i];
                    const auto& podSetId = (i < podSetIdsVec.size() ? podSetIdsVec[i] : TString{"unknown"});

                    const auto gApiLogContext = TFLRecords::StartContext()
                        ("yp_datacenter", dataCenter)
                        ("api_call", R"(SelectObjects(NYP::...::EObjectType::OT_POD, ...)")
                        ("api_address", YpServices.GetAddress(dataCenter))
                        ("api_arg", podSetId);

                    if (!futureResponse.Wait(deadline)) {
                        TFLEventLog::Error("Timeout while waiting for response from YP")
                            ("api_timeout", timeout);
                        failedStages.emplace(stageName, dataCenter);
                        continue;
                    }

                    if (futureResponse.HasException() || !futureResponse.HasValue()) {
                        TFLEventLog::Error("Exception in Future from YP")
                            ("api_exception", SerializeFutureExceptionToJson(futureResponse));
                        failedStages.emplace(stageName, dataCenter);
                        continue;
                    }

                    const auto& ypResponse = futureResponse.GetValue();
                    for (auto&& result : ypResponse.Results) {
                        NYP::NClient::TPod pod;
                        result.Fill(pod.MutableMeta(), pod.MutableStatus()->mutable_dns());
                        podsMap[dataCenter][stageName][pod.Meta().id()] = std::move(pod);
                    }
                }
            }
        }

        TVector<TPodLocator> result;
        for (auto&& [dataCenter, stageToPod] : podsMap) {
            for (auto&& [stageName, idToPod] : stageToPod) {
                for (auto&& [podId, pod] : idToPod) {
                    TPodLocator podLocator;
                    podLocator.SetDeploySystem(GetDeploySystem());
                    podLocator.SetService(stageName);
                    podLocator.SetDataCenter(dataCenter);
                    podLocator.SetDcLocalPodId(podId);
                    podLocator.SetContainerHostname(pod.Status().dns().persistent_fqdn());
                    result.emplace_back(std::move(podLocator));
                }
            }
        }

        FillFromCacheForFailed(result, failedStages);
        UpdateCache(result);

        return result;
    }


    EDeploySystem TDeployPodLocatorsStorage::Type() {
        return EDeploySystem::Deploy;
    }


    const TString& TDeployPodLocatorsStorage::TypeStr() {
        static const TString typeStr = ToString(Type());
        return typeStr;
    }


    EDeploySystem TDeployPodLocatorsStorage::GetDeploySystem() const {
        return Type();
    }


    void TDeployPodLocatorsStorage::UpdateCache(const TVector<TPodLocator>& podLocators) const {
        TWriteGuard g(Mutex);
        for (auto&& podLocator : podLocators) {
            Stage2PodLocatorsCache[podLocator.GetService()].clear();
        }
        for (auto&& podLocator : podLocators) {
            Stage2PodLocatorsCache[podLocator.GetService()].insert(podLocator);
        }
    }


    void TDeployPodLocatorsStorage::FillFromCacheForFailed(TVector<TPodLocator>& podLocators,
                                                          const TSet<std::pair<TStageName, TMaybe<TDataCenter>>>& failedServices) const {
        auto podLocatorsSet = MakeSet(podLocators);
        {
            TReadGuard g(Mutex);
            for (auto&& [stage, maybeDC] : failedServices) {
                auto it = Stage2PodLocatorsCache.find(stage);
                if (it == Stage2PodLocatorsCache.end()) {
                    continue;
                }
                for (auto&& podLocator : it->second) {
                    if (maybeDC && maybeDC.GetRef() == podLocator.GetDataCenter()) {
                        podLocatorsSet.insert(podLocator);
                    } else {
                        podLocatorsSet.insert(podLocator);
                    }
                }
            }
        }
        podLocators = MakeVector(podLocatorsSet);
    }


    TVector<TPodLocator> TDeployPodLocatorsStorage::GetPodLocatorsFromCache(const TVector<TStageName>& stages) const {
        TReadGuard g(Mutex);
        TVector<TPodLocator> result;
        for (auto&& stage : stages) {
            const auto* ptr = MapFindPtr(Stage2PodLocatorsCache, stage);
            if (!ptr) {
                continue;
            }
            result.insert(result.end(), ptr->begin(), ptr->end());
        }
        return result;
    }

}
