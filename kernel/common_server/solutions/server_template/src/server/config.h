#pragma once

#include <kernel/common_server/server/config.h>
#include <kernel/common_server/api/snapshots/controller.h>

namespace NCS {
    namespace NServerTemplate {
        class TServerConfig: public TBaseServerConfig {
        private:
            using TBase = TBaseServerConfig;
        protected:
            virtual void DoToString(IOutputStream& os) const override;
        public:
            using TBase::TBase;
            virtual void Init(const TYandexConfig::Section* section) override;
        };
    }
}
