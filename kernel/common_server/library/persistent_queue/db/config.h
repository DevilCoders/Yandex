#pragma once

#include <kernel/common_server/library/persistent_queue/abstract/config.h>
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/util/accessor.h>
#include <util/folder/path.h>

namespace NCS {
    namespace NPQ {
        namespace NDatabase {
            class TConfig: public IPQClientConfig {
            private:
                TString DBName;
                static TFactory::TRegistrator<TConfig> Registrator;
            protected:
                virtual void DoInit(const TYandexConfig::Section* section) override;
                virtual void DoToString(IOutputStream& os) const override;
                virtual THolder<IPQClient> DoConstruct(const IPQConstructionContext& context) const override;
            public:
                static TString GetTypeName() {
                    return "db";
                }
                virtual TString GetClassName() const override {
                    return GetTypeName();
                }
            };
        }
    }
}
