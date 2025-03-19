#include "pod_locator.h"

#include <kernel/common_server/library/logging/events.h>
#include <library/cpp/json/json_value.h>
#include <util/string/join.h>

namespace {
    TString GetLowestLevelDomain(const TString& hostname) {
        return TString(hostname.begin(), Find(hostname, '.'));
    }
}

bool TPodLocator::DeserializeFrom(const NServiceMonitor::NProto::TServiceInstanceInfo& instanceInfo, const EDeploySystem deploySystem,
                                  const TString& service) {
    DataCenter = GetDataCenter(instanceInfo);
    if (DataCenter.empty()) {
        return false;
    }

    DeploySystem = deploySystem;
    Service = service;
    DcLocalPodId = GetLowestLevelDomain(instanceInfo.GetContainerHostname());
    ContainerHostname = instanceInfo.GetContainerHostname();
    return true;
}

NServiceMonitor::NProto::TInstanceInfo TPodLocator::SerializeToProto(const NServiceMonitor::NProto::TServerInfo& serverInfo,
                                                                     const NServiceMonitor::NProto::TResourceRequests& resources) const {
    NServiceMonitor::NProto::TInstanceInfo result;
    result.SetName(ContainerHostname);
    result.SetShortName(DcLocalPodId);
    *result.MutableServerInfo() = serverInfo;
    *result.MutableResourceRequests() = resources;
    return result;
}

NJson::TJsonValue TPodLocator::SerializeToJson() const {
    NJson::TJsonValue result;
    result["deploy_system"] = ToString(DeploySystem);
    result["service"] = Service;
    result["datacenter"] = DataCenter;
    result["dc_local_pod_id"] = DcLocalPodId;
    result["container_hostname"] = ContainerHostname;
    result["controller_port"] = ControllerPort;
    return result;
}


TString TPodLocator::GetDataCenter(const NServiceMonitor::NProto::TServiceInstanceInfo& instanceInfo) {
    static const TString DcTagPrefix = "a_dc_";
    for (const TString& itag : instanceInfo.GetItags()) {
        if (itag.StartsWith(DcTagPrefix)) {
            return itag.substr(DcTagPrefix.size());
        }
    }

    TFLEventLog::Error("Can't extract DataCenter from TServiceInstanceInfo.")
        ("datacenter_itag_prefix", DcTagPrefix)
        ("pod_hostname", instanceInfo.GetContainerHostname())
        ("present_itags", JoinSeq(", ", instanceInfo.GetItags()))
        ("message", "No datacenter itag prefix \"" + DcTagPrefix + "\" found for pod host \"" + instanceInfo.GetContainerHostname() + "\" in NProto::TServiceInstanceInfo reply.");

    return {};
}

bool TPodLocator::operator<(const TPodLocator& right) const noexcept {
    return std::tie(DeploySystem, Service, DataCenter, DcLocalPodId, ContainerHostname, ControllerPort) < std::tie(right.DeploySystem, right.Service, right.DataCenter, right.DcLocalPodId, right.ContainerHostname, right.ControllerPort);
}

