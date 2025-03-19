#pragma once
#include <kernel/common_server/common/manager_config.h>

namespace NCS {
    namespace NObfuscator {

        class TDBManagerConfig: public NCommon::TManagerConfig {
        private:
            using TBase = NCommon::TManagerConfig;
            CSA_READONLY(bool, TotalObfuscateByDefault, false);

        public:
            virtual void Init(const TYandexConfig::Section* section) override;
            virtual void ToString(IOutputStream& os) const override;
        };

    }
}
