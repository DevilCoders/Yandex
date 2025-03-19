#pragma once
#include "pod_locator.h"

#include <kernel/common_server/service_monitor/handlers/proto/service_info.pb.h>
#include <util/generic/map.h>


namespace NServiceMonitor {
    using TServiceName = TString;
    using TStageName = TString;
    using TDataCenter = TString;
    using TDeploySystem = TString;

    class IPodLocatorsStorage {
    public:
        using TPtr = TAtomicSharedPtr<IPodLocatorsStorage>;

        virtual ~IPodLocatorsStorage() = default;
        virtual TVector<TPodLocator> GetPodLocators(const TVector<TServiceName>& services) const = 0;
        virtual EDeploySystem GetDeploySystem() const = 0;
    };

    class IServerInfoStorage {
    public:
        using TPtr = TAtomicSharedPtr<IServerInfoStorage>;

        virtual ~IServerInfoStorage() = default;
        virtual TVector<NProto::TServerInfo> GetServerInfo(const TVector<TPodLocator>& podLocators) const = 0;
    };

    class IResourcesInfoStorage {
    public:
        using TPtr = TAtomicSharedPtr<IResourcesInfoStorage>;

        virtual ~IResourcesInfoStorage() = default;
        virtual TVector<NProto::TResourceRequests> GetResourcesInfo(const TVector<TPodLocator>& podLocators) const = 0;
    };
}
