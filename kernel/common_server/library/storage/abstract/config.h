#pragma once
#include <kernel/common_server/library/interfaces/tvm_manager.h>
#include <kernel/common_server/util/accessor.h>

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/yconf/conf.h>

#include <util/stream/output.h>
#include <util/folder/path.h>
#include <util/datetime/base.h>

namespace NCS {
    namespace NStorage {
        class IDatabase;

        class IDatabaseConstructionContext {
        private:
            CSA_DEFAULT(IDatabaseConstructionContext, TString, DBName);
        public:
            virtual const ITvmManager* GetTvmManager() const;
            virtual ~IDatabaseConstructionContext() = default;
        };

        class IDatabaseConfig {
        private:
            CSA_READONLY_DEF(TString, DBInternalId);
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) = 0;
            virtual void DoToString(IOutputStream& os) const = 0;
        public:
            virtual ~IDatabaseConfig() {}
            virtual TAtomicSharedPtr<IDatabase> ConstructDatabase(const IDatabaseConstructionContext* context = nullptr) const = 0;
            void Init(const TYandexConfig::Section* section) {
                DBInternalId = section->Name;
                DoInit(section);
            }
            void ToString(IOutputStream& os) const {
                DoToString(os);
            }

        public:
            using TFactory = NObjectFactory::TParametrizedObjectFactory<IDatabaseConfig, TString>;
            using TPtr = TAtomicSharedPtr<IDatabaseConfig>;
        };

        class TDatabaseConfig {
        public:
            void Init(const TYandexConfig::Section* section);

            void ToString(IOutputStream& os) const;

            TAtomicSharedPtr<IDatabase> ConstructDatabase(const IDatabaseConstructionContext* context = nullptr) const;
        private:
            TString Type;
            IDatabaseConfig::TPtr ConfigImpl;
        };
    }
}
