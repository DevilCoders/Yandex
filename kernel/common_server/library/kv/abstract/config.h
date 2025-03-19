#pragma once
#include "storage.h"
#include <util/stream/output.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/object_factory/object_factory.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/interfaces/container.h>

namespace NCS {
    namespace NKVStorage {
        class IConfig {
        public:
            using TPtr = TAtomicSharedPtr<IConfig>;
            using TFactory = NObjectFactory::TObjectFactory<IConfig, TString>;
        private:
            CSA_READONLY_DEF(TString, StorageName);
            CSA_READONLY(ui32, ThreadsCount, 4);
        protected:
            virtual void DoInit(const TYandexConfig::Section* /*section*/) = 0;
            virtual void DoToString(IOutputStream& /*os*/) const = 0;
            virtual IStorage::TPtr DoBuildStorage() const = 0;
        public:
            IStorage::TPtr BuildStorage() const {
                return DoBuildStorage();
            }
            void Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;
            virtual ~IConfig() = default;
            virtual TString GetClassName() const = 0;
        };

        class TConfigContainer: public TBaseInterfaceContainer<IConfig> {
        private:
            using TBase = TBaseInterfaceContainer<IConfig>;
        public:
            using TBase::TBase;
        };
    }
}
