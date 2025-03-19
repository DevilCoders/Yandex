#pragma once
#include <kernel/common_server/common/manager_config.h>
#include <kernel/common_server/secret/abstract/config.h>

namespace NCS {
    namespace NSecret {

        class TFakeManagerConfig: public ISecretsManagerConfig {
        private:
            using TBase = NCommon::TManagerConfig;
            static TFactory::TRegistrator<TFakeManagerConfig> Registrator;
            CSA_DEFAULT(TFakeManagerConfig, TString, HashSalt);
        protected:
            virtual void DoInit(const TYandexConfig::Section* /*section*/) override {
            }
            virtual void DoToString(IOutputStream& /*os*/) const override {
            }
        public:
            virtual TAtomicSharedPtr<ISecretsManager> Construct(const IBaseServer& server) const override;

            static TString GetTypeName() {
                return "fake";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };

    }
}
