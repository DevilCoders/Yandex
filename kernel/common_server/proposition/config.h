#pragma once
#include <kernel/common_server/common/manager_config.h>

namespace NCS {
    namespace NPropositions {

        class TDBManagerConfig: public NCommon::TManagerConfig {
            CSA_READONLY(ui32, NeedApprovesCount, 0);
        private:
            using TBase = NCommon::TManagerConfig;
        public:
            TDBManagerConfig() = default;

            virtual void Init(const TYandexConfig::Section* section) override;

            virtual void ToString(IOutputStream& os) const override;
        };

    }
}
