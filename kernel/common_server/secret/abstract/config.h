#pragma once
#include <kernel/common_server/roles/abstract/manager.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/common/manager_config.h>
#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/digest/md5/md5.h>
#include "manager.h"

namespace NCS {
    namespace NSecret {
        class ISecretsManagerConfig {
        private:
            CSA_READONLY_DEF(TString, ManagerId);
        protected:
            virtual void DoInit(const TYandexConfig::Section* /*section*/) {

            }
            virtual void DoToString(IOutputStream& /*os*/) const {

            }
        public:
            using TPtr = TAtomicSharedPtr<ISecretsManagerConfig>;
            using TFactory = NObjectFactory::TObjectFactory<ISecretsManagerConfig, TString>;
            virtual ~ISecretsManagerConfig() = default;
            void Init(const TYandexConfig::Section* section) {
                ManagerId = section->Name;
                DoInit(section);
            }
            void ToString(IOutputStream& os) const {
                DoToString(os);
            }
            virtual TAtomicSharedPtr<ISecretsManager> Construct(const IBaseServer& server) const = 0;
            virtual TString GetClassName() const = 0;
        };

        class TSecretsManagerConfig: public TBaseInterfaceContainer<ISecretsManagerConfig> {
        private:
            using TBase = TBaseInterfaceContainer<ISecretsManagerConfig>;
        public:
            using TBase::TBase;
        };
    }
}
