#pragma once

#include <kernel/common_server/service_monitor/handlers/proto/service_info.pb.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/json/json_value.h>

enum class EDeploySystem {
    Nanny /* "nanny" */,
    Deploy /* "deploy" */
};

class TPodLocator {
public:
    CS_ACCESS(TPodLocator, EDeploySystem, DeploySystem, EDeploySystem::Nanny);
    CSA_DEFAULT(TPodLocator, TString, Service);  // "someservice_backend_testing"
    CSA_DEFAULT(TPodLocator, TString, DataCenter);  // "man", "vla", etc.
    CSA_DEFAULT(TPodLocator, TString, DcLocalPodId);
    CSA_DEFAULT(TPodLocator, TString, ContainerHostname);
    CSA_DEFAULT(TPodLocator, ui16, ControllerPort);

public:
    Y_WARN_UNUSED_RESULT bool DeserializeFrom(const NServiceMonitor::NProto::TServiceInstanceInfo& instanceInfo, const EDeploySystem deploySystem, const TString& service);

    NServiceMonitor::NProto::TInstanceInfo SerializeToProto(const NServiceMonitor::NProto::TServerInfo& serverInfo, const NServiceMonitor::NProto::TResourceRequests& resources) const;
    NJson::TJsonValue SerializeToJson() const;

public:
    static TString GetDataCenter(const NServiceMonitor::NProto::TServiceInstanceInfo& instanceInfo);

    bool operator<(const TPodLocator& right) const noexcept;
};
