#pragma once
#include "abstract.h"

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/abstract/external_api.h>

namespace NCS {
    class IExternalServicesOperator;
}

namespace NServiceMonitor {
    class TNannyPodLocatorsStorage : public IPodLocatorsStorage {
    public:
        TNannyPodLocatorsStorage(const NCS::IExternalServicesOperator& externalServicesOperator);

        TVector<TPodLocator> GetPodLocators(const TVector<TServiceName>& services) const override;
        EDeploySystem GetDeploySystem() const override;

    public:
        static EDeploySystem Type() {
            return EDeploySystem::Nanny;
        }

        static const TString& TypeStr() {
            static const TString typeStr = ToString(Type());
            return typeStr;
        }

    private:
        void UpdateCache(const TVector<TPodLocator>& podLocators) const;
        void FillFromCacheForFailed(TVector<TPodLocator>& podLocators, const TVector<TServiceName>& failedServices) const;

    private:
        TAtomicSharedPtr<NExternalAPI::TSender> NannyClient;

        mutable TMutex CachedServicePodLocatorsMutex;
        mutable TMap<TServiceName, TVector<TPodLocator>> CachedServicePodLocators;
    };
}
