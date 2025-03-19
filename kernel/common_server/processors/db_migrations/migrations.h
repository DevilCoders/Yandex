#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NCS {
    namespace NHandlers {

        class TDBMigrationsPermissions : public TAdministrativePermissions {
        private:
            using TBase = TAdministrativePermissions;
            static TFactory::TRegistrator<TDBMigrationsPermissions> Registrator;

        public:
            static TString GetTypeName();
            virtual TString GetClassName() const override;
            bool Check(const EObjectAction& action) const;
        };

        class TDBMigrationsInfoProcessor : public TCommonSystemHandler<TDBMigrationsInfoProcessor> {
        private:
            using TBase = TCommonSystemHandler<TDBMigrationsInfoProcessor>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "db-migrations-info";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };


        class TDBMigrationsApplyProcessor : public TCommonSystemHandler<TDBMigrationsApplyProcessor> {
        private:
            using TBase = TCommonSystemHandler<TDBMigrationsApplyProcessor>;
        public:
            using TBase::TBase;

            static TString GetTypeName() {
                return "db-migrations-upsert";
            }

            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
        };

    }
}
