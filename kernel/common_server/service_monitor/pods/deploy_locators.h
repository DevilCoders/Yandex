#pragma once

#include "abstract.h"

#include <yp/cpp/yp/request_model.h>
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/service_monitor/services/yp.h>

namespace NServiceMonitor {

    class TServerConfig;

    class TDeployPodLocatorsStorage: public IPodLocatorsStorage {
    public:
        TDeployPodLocatorsStorage(const TYpServicesOperator& ypServices);

        TVector<TPodLocator> GetPodLocators(const TVector<TStageName>& stages) const override;
        EDeploySystem GetDeploySystem() const override;

    public:
        static EDeploySystem Type();
        static const TString& TypeStr();

    private:
        void UpdateCache(const TVector<TPodLocator>& podLocators) const;
        void FillFromCacheForFailed(TVector<TPodLocator>& podLocators, const TSet<std::pair<TStageName, TMaybe<TDataCenter>>>& failedServices) const;
        TVector<TPodLocator> GetPodLocatorsFromCache(const TVector<TStageName>& stages) const;

    private:
        const TYpServicesOperator& YpServices;

        mutable TRWMutex Mutex;
        mutable TMap<TString, TSet<TPodLocator>> Stage2PodLocatorsCache;
    };
}
