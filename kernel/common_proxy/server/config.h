#pragma once

#include <kernel/common_proxy/common/link_config.h>
#include <kernel/common_proxy/common/processor_config.h>
#include <kernel/common_proxy/unistat_signals/signals.h>
#include <kernel/daemon/config/config_constructor.h>

namespace NCommonProxy {

    class TServerConfig : public  IServerConfig {
    public:
        typedef TVector<TLinkConfig> TLinks;

    public:
        TServerConfig(const TServerConfigConstructorParams& params);
        virtual const TDaemonConfig& GetDaemonConfig() const override;
        virtual TSet<TString> GetModulesSet() const override;
        TString ToString() const;

        const TProcessorsConfigs& GetProccessorsConfigs() const {
            return ProccessorsConfigs;
        }

        const TLinks& GetLinks() const {
            return Links;
        }

        const TUnistatSignalsConfig& GetUnistatSignals() const {
            return UnistatSignals;
        }

        TDuration GetStartTimeout() const {
            return StartTimeout;
        }

    private:
        const TDaemonConfig& Daemon;
        TProcessorsConfigs ProccessorsConfigs;
        TLinks Links;
        TUnistatSignalsConfig UnistatSignals;
        TDuration StartTimeout;
    };

}
