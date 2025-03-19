#pragma once

#include "config.h"

#include <kernel/common_server/service_monitor/pods/deploy_locators.h>
#include <kernel/common_server/service_monitor/pods/nanny_locators.h>
#include <kernel/common_server/service_monitor/pods/server_info_storage.h>
#include <kernel/common_server/service_monitor/pods/resources_storage.h>
#include <kernel/common_server/service_monitor/services/yp.h>

#include <kernel/common_server/server/server.h>


namespace NServiceMonitor {
    class TServer: public TBaseServer {
    public:
        using TConfig = NServiceMonitor::TServerConfig;

    public:
        TServer(const TConfig& config);

        const TServerInfoStorage& GetServerInfoStorage() const {
            CHECK_WITH_LOG(!!ServerInfoStorage);
            return *ServerInfoStorage;
        }

        const TYpResourcesInfoStorage& GetResourcesInfoStorage() const {
            CHECK_WITH_LOG(!!ResourcesInfoStorage);
            return *ResourcesInfoStorage;
        }

        IPodLocatorsStorage::TPtr GetPodLocatorsStoragePtr(const EDeploySystem& deploySystem) const {
            auto* ptr = MapFindPtr(PodLocatorsStorages, deploySystem);
            return ptr ? *ptr : nullptr;
        }

    protected:
        void DoRun() override;

    private:
        THolder<TServerInfoStorage> ServerInfoStorage;
        THolder<TYpResourcesInfoStorage> ResourcesInfoStorage;
        TMap<EDeploySystem, IPodLocatorsStorage::TPtr> PodLocatorsStorages;
        TYpServicesOperator YpServicesOperator;

        const NServiceMonitor::TServerConfig& Config;
    };
}
