#include "server.h"

#include <kernel/common_server/service_monitor/pods/deploy_locators.h>
#include <kernel/common_server/service_monitor/pods/nanny_locators.h>

namespace NServiceMonitor {
    TServer::TServer(const NServiceMonitor::TServerConfig& config)
        : TBaseServer(config)
        , YpServicesOperator(config.GetYpApiConfig())
        , Config(config)
    {
    }

    void TServer::DoRun() {
        ServerInfoStorage = MakeHolder<TServerInfoStorage>(Config.GetServerInfoStorageConfig());
        ResourcesInfoStorage = MakeHolder<TYpResourcesInfoStorage>(YpServicesOperator);

        CHECK_WITH_LOG(PodLocatorsStorages[TNannyPodLocatorsStorage::Type()] = MakeHolder<TNannyPodLocatorsStorage>(*this));
        CHECK_WITH_LOG(PodLocatorsStorages[TDeployPodLocatorsStorage::Type()] = MakeHolder<TDeployPodLocatorsStorage>(YpServicesOperator));
    }
}

