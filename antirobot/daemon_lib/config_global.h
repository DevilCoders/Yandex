#pragma once

#include "config.h"

#include <util/generic/ptr.h>

class IInputStream;

namespace NAntiRobot {
    class TAntirobotConfig {
    public:
        static TAntirobotConfig& Get();

        TAntirobotConfig();
        void LoadFromPath(const TString& filePath);

        TAntirobotDaemonConfig& GetDaemonConfig() {
            return *DaemonConfig.Get();
        }

        TString FilePath;
        TString NonBrandedPartners;

    private:
        THolder<TAntirobotDaemonConfig> DaemonConfig;
    };
}

#define ANTIROBOT_CONFIG_MUTABLE TAntirobotConfig::Get()
#define ANTIROBOT_CONFIG (static_cast<const TAntirobotConfig&>(ANTIROBOT_CONFIG_MUTABLE))


#define ANTIROBOT_DAEMON_CONFIG_MUTABLE ANTIROBOT_CONFIG_MUTABLE.GetDaemonConfig()
#define ANTIROBOT_DAEMON_CONFIG (static_cast<const TAntirobotDaemonConfig&>(ANTIROBOT_DAEMON_CONFIG_MUTABLE))

