#pragma once

#include <kernel/common_server/server/config.h>
#include <kernel/common_server/service_monitor/pods/server_info_storage.h>
#include <kernel/common_server/service_monitor/services/yp.h>


namespace NServiceMonitor {

    class TServerConfig: public TBaseServerConfig {
        CSA_READONLY_DEF(TYpApiConfig, YpApiConfig);
        CSA_READONLY_DEF(TServerInfoStorageConfig, ServerInfoStorageConfig);

    private:
        using TBase = TBaseServerConfig;

    public:
        using TBase::TBase;

        virtual void Init(const TYandexConfig::Section* section) override;

    protected:
        virtual void DoToString(IOutputStream& os) const override;
    };

}
