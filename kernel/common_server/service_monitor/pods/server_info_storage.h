#pragma once

#include "abstract.h"

#include <kernel/common_server/library/tvm_services/abstract/abstract.h>
#include <kernel/common_server/library/cache/cache_with_live_time.h>

class TBaseServerConfig;

namespace NServiceMonitor {

    class TServerInfoStorageConfig {
        CSA_DEFAULT(TServerInfoStorageConfig, NExternalAPI::TSenderConfig, DynamicHostAPIConfig);
        CS_ACCESS(TServerInfoStorageConfig, TDuration, ConnectionLiveTime, TDuration::Minutes(1));
    public:
        void Init(const TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
    };

    class TServerInfoStorage: public IServerInfoStorage {
    public:
        explicit TServerInfoStorage(const TServerInfoStorageConfig& config);

        TVector<NProto::TServerInfo> GetServerInfo(const TVector<TPodLocator>& podLocators) const override;
        TAtomicSharedPtr<NExternalAPI::TSender> GetSender(const TString& podHostname, const ui16 controllerPort = 0) const;

    private:
        const TServerInfoStorageConfig& Config;
        mutable TCacheWithLiveTime<TString, TAtomicSharedPtr<NExternalAPI::TSender>> ConnectionsCache;
    };
}
