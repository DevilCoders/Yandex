#pragma once
#include <kernel/common_server/obfuscator/object.h>
#include <kernel/common_server/library/scheme/handler.h>
#include <kernel/common_server/processors/db_entity/handler.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/roles/actions/common.h>

namespace NCS {
    namespace NHandlers {
        class TObfuscatorPermissions: public TDBObjectPermissions<NObfuscator::TDBObfuscator> {
        private:
            static TFactory::TRegistrator<TObfuscatorPermissions> Registrator;
        };

        class TObfuscatorInfoProcessor: public TDBObjectsInfo<TObfuscatorInfoProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TDBObjectsInfo<TObfuscatorInfoProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer>;

        protected:
            virtual const IDBEntitiesManager<NObfuscator::TDBObfuscator>* GetObjectsManager() const override;

        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "obfuscator-info";
            }
        };

        class TObfuscatorUpsertProcessor: public TDBObjectsUpsert<TObfuscatorUpsertProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TDBObjectsUpsert<TObfuscatorUpsertProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer>;

        protected:
            virtual const IDBEntitiesManager<NObfuscator::TDBObfuscator>* GetObjectsManager() const override;

        public:
            using TBase::TBase;
            static TString GetTypeName() {
                return "obfuscator-upsert";
            }
        };

        class TObfuscatorRemoveProcessor: public TDBObjectsRemove<TObfuscatorRemoveProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer> {
        private:
            using TBase = TDBObjectsRemove<TObfuscatorRemoveProcessor, NObfuscator::TDBObfuscator, TSystemUserPermissions, IBaseServer>;

        protected:
            virtual const IDBEntitiesManager<NObfuscator::TDBObfuscator>* GetObjectsManager() const override;

        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "obfuscator-remove";
            }
        };

        class TObfuscatorDebugProcessor: public TCommonSystemHandler<class TObfuscatorDebugProcessor> {
        private:
            using TBase = TCommonSystemHandler<class TObfuscatorDebugProcessor>;

        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "obfuscator-debug";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };
    }
}
