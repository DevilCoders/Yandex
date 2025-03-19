#pragma once
#include <kernel/common_server/common/manager_config.h>
#include <kernel/common_server/secret/abstract/config.h>
#include <kernel/common_server/library/kv/abstract/config.h>

namespace NCS {
    namespace NSecret {

        class TKVManagerConfig: public ISecretsManagerConfig {
        private:
            CSA_READONLY_FLAG(AllowDuplicate, false);
            NKVStorage::TConfigContainer KVConfig;
            static TFactory::TRegistrator<TKVManagerConfig> Registrator;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override;
            virtual void DoToString(IOutputStream& os) const override;
        public:
            virtual TAtomicSharedPtr<ISecretsManager> Construct(const IBaseServer& server) const override;

            static TString GetTypeName() {
                return "key-value-storage";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

    }
}
