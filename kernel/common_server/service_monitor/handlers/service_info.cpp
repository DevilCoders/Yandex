#include "service_info.h"

#include <kernel/common_server/service_monitor/pods/errors.h>
#include <kernel/common_server/service_monitor/server/server.h>
#include <kernel/common_server/service_monitor/handlers/proto/service_info.pb.h>
#include <kernel/common_server/util/network/simple.h>

#include <yp/cpp/yp/client.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/iterator/concatenate.h>

#include <kernel/common_server/util/algorithm/container.h>

#include <util/generic/yexception.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/system/env.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

#include <functional>

namespace {
    class TServiceQuery {
        CS_ACCESS(TServiceQuery, EDeploySystem, DeploySystem, EDeploySystem::Nanny);
        CSA_DEFAULT(TServiceQuery, TString, Service);
        CSA_DEFAULT(TServiceQuery, ui16, ControllerPort);

        static constexpr TStringBuf Format = "DeploySystem:ServiceName[:ControllerPort]";

    public:
        bool DeserializeFromString(const TString& rawQuery) {
            TVector<TString> parts = SplitString(rawQuery, ":");
            parts.resize(3);
            const TString& deploySystem = parts[0];
            Service = parts[1];
            const TString& controllerPort = parts[2];

            if (deploySystem.empty() || Service.empty()) {
                TFLEventLog::Error("Cannot deserialize TServiceQuery from string: DeploySystem or ServiceName is empty.")
                    ("query_raw", rawQuery)("query_format", Format)("deploy_system", DeploySystem)("service_name", Service);
                return false;
            }

            if (!TryFromString(deploySystem, DeploySystem)) {
                TFLEventLog::Error("Cannot deserialize TServiceQuery, unknown DeploySystem.")
                    ("deploy_system", deploySystem)("query_raw", rawQuery)("query_format", Format)("known_deploy_systems", GetEnumAllNames<EDeploySystem>());
                return false;
            }

            if (!controllerPort.empty()) {
                if (!TryFromString(controllerPort, ControllerPort)) {
                    TFLEventLog::Error("Cannot deserialize TServiceQuery, cannot parse ControllerPort.")
                        ("controller_port_raw", controllerPort)("query_raw", rawQuery)("query_format", Format);
                    return false;
                }
            }

            return true;
        }

        static TVector<TServiceQuery> DeserializeServiceQueries(const TVector<TString>& rawQueries) {
            TVector<TServiceQuery> result;
            for (auto&& rawQuery : rawQueries) {
                TServiceQuery serviceQuery;
                if (serviceQuery.DeserializeFromString(rawQuery)) {
                    result.emplace_back(serviceQuery);
                }
            }
            return result;
        }

        class TPortKey {
            CS_ACCESS(TPortKey, EDeploySystem, DeploySystem, EDeploySystem::Nanny);
            CSA_DEFAULT(TPortKey, TString, Service);
        public:
            bool operator<(const TPortKey& right) const noexcept {
                return std::tie(DeploySystem, Service) < std::tie(right.DeploySystem, right.Service);
            }
        };

        static TPortKey GetPortKey(auto&& x) {
            TPortKey portKey;
            portKey.SetDeploySystem(x.GetDeploySystem()).SetService(x.GetService());
            return portKey;
        };
    };
}


namespace NServiceMonitor {
    TServiceInfoHandler::TServiceInfoHandler(
        const TConfig& config, IReplyContext::TPtr context, IAuthModule::TPtr authModule, const IBaseServer* server)
        : TBase(config, context, authModule, server)
    {
    }

    void TServiceInfoHandler::ProcessHttpRequest(TJsonReport::TGuard& g, IAuthInfo::TPtr authInfo) {
        Y_UNUSED(authInfo);

        const auto& server = GetServerAs<NServiceMonitor::TServer>();
        const auto arrayMaxItems = GetHandlerSettingDef<ui64>("parameter_array_max_items", 32);

        TVector<TString> serviceFromCgi = GetValues<TString>(Context->GetCgiParameters(), "service", false, arrayMaxItems);
        if (serviceFromCgi.empty()) {
            serviceFromCgi = Config.GetDefaultServiceList();
        }
        const TVector<TServiceQuery> serviceQueries = TServiceQuery::DeserializeServiceQueries(serviceFromCgi);

        TMap<EDeploySystem, TVector<TServiceName>> deployToServices;
        for (auto&& serviceQuery : serviceQueries) {
            deployToServices[serviceQuery.GetDeploySystem()].emplace_back(serviceQuery.GetService());
        }

        TMap<TServiceQuery::TPortKey, ui16> controllerPorts;
        for (auto&& serviceQuery : serviceQueries) {
            controllerPorts[TServiceQuery::GetPortKey(serviceQuery)] = serviceQuery.GetControllerPort();
        }

        TVector<TPodLocator> podLocators;
        for (auto&& [deploySystem, serviceNames] : deployToServices) {
            if (const auto& storagePtr = server.GetPodLocatorsStoragePtr(deploySystem)) {
                for (auto&& podLocator : storagePtr->GetPodLocators(serviceNames)) {
                    podLocator.SetControllerPort(controllerPorts[TServiceQuery::GetPortKey(podLocator)]);
                    podLocators.emplace_back(std::move(podLocator));
                }
            } else {
                TFLEventLog::Error("Unknown deploy system.")
                    ("deploy_system", deploySystem)
                    ("deploy_system_num", ToUnderlying(deploySystem))
                    ("deploy_known_systems", JoinSeq(", ", GetEnumAllValues<EDeploySystem>()));
            }
        }

        Sort(podLocators);  // To fix pods order in the report.

        const TVector<NProto::TServerInfo> serverInfos = server.GetServerInfoStorage().GetServerInfo(podLocators);
        ReqCheckCondition(podLocators.size() == serverInfos.size(), ConfigHttpStatus.UnknownErrorStatus, TStringBuilder() << "Got " << serverInfos.size() << " TServerInfo results, when requested " << podLocators.size() << " results.");

        const TVector<NProto::TResourceRequests> resources = server.GetResourcesInfoStorage().GetResourcesInfo(podLocators);
        ReqCheckCondition(podLocators.size() == resources.size(), ConfigHttpStatus.UnknownErrorStatus, TStringBuilder() << "Got " << resources.size() << " TResourceRequests results, when requested " << podLocators.size() << " results.");

        TMap<TServiceName, TMap<TDataCenter, TVector<NProto::TInstanceInfo>>> indexedResult;
        for (size_t i = 0; i < podLocators.size(); ++i) {
            const TPodLocator& podLocator = podLocators[i];
            const NProto::TServerInfo& serverInfo = serverInfos[i];
            const NProto::TResourceRequests& resourceRequests = resources[i];
            indexedResult[podLocator.GetService()][podLocator.GetDataCenter()].emplace_back(podLocator.SerializeToProto(serverInfo, resourceRequests));
        }

        NProto::TServiceFrontendInfoContainer result;
        for (auto&& serviceQuery : serviceQueries) {
            NProto::TServiceFrontendInfo serviceInfo;
            serviceInfo.SetName(serviceQuery.GetService());
            for (auto&& [dataCenter, instanceInfos] : indexedResult[serviceQuery.GetService()]) {
                NProto::TDataCenterFrontendInfo dataCenterInfo;
                dataCenterInfo.SetName(dataCenter);
                for (auto&& instanceInfo : instanceInfos) {
                    *dataCenterInfo.AddInstances() = std::move(instanceInfo);
                }
                *serviceInfo.AddDataCenters() = std::move(dataCenterInfo);
            }
            *result.AddServices() = std::move(serviceInfo);
        }

        NJson::TJsonValue json;
        NProtobufJson::Proto2Json(result, json, NProtobufJson::TProto2JsonConfig().SetUseJsonName(true));
        g.AddReportElement("result", std::move(json));
        g.SetCode(HTTP_OK);
    }

    bool TServiceInfoHandlerConfig::InitFeatures(const TYandexConfig::Section* section) {
        StringSplitter(section->GetDirectives().Value<TString>("DefaultServiceList")).SplitBySet(", ").SkipEmpty().Collect(&DefaultServiceList);
        return true;
    }


    void TServiceInfoHandlerConfig::ToStringFeatures(IOutputStream& os) const {
        os << "DefaultServiceList: " << JoinSeq(", ", DefaultServiceList) << Endl;
    }

}

