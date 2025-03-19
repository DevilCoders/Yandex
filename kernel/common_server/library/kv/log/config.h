#pragma once
#include "storage.h"
#include <kernel/common_server/library/kv/abstract/config.h>

namespace NCS {
    namespace NKVStorage {
        class TLogConfig: public IConfig {
        private:
            CSA_READONLY_FLAG(MemoryMapUsage, false)
            static TFactory::TRegistrator<TLogConfig> Registrator;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override {
                MemoryMapUsageFlag = section->GetDirectives().Value("MemoryMapUsage", MemoryMapUsageFlag);
            }
            virtual void DoToString(IOutputStream& os) const override {
                os << "MemoryMapUsage: " << MemoryMapUsageFlag << Endl;
            }
            virtual IStorage::TPtr DoBuildStorage() const override;
        public:
            static TString GetTypeName() {
                return "log";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
