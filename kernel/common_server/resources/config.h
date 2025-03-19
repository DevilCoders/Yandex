#pragma once
#include <kernel/common_server/roles/abstract/manager.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/common/manager_config.h>

namespace NCS {
    namespace NResources {

        class TDBManagerConfig: public NCommon::TManagerConfig {
        private:
            using TBase = NCommon::TManagerConfig;
        public:
            virtual void Init(const TYandexConfig::Section* section) override {
                TBase::Init(section);
            }
            virtual void ToString(IOutputStream& os) const override {
                TBase::ToString(os);
            }
        };

    }
}
