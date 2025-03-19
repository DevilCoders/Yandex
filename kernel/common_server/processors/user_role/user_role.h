#pragma once
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TUserRolePermissions: public TSearchAdministrativePermissions {
    private:
        using TBase = TSearchAdministrativePermissions;
        static TFactory::TRegistrator<TUserRolePermissions> Registrator;

    public:
        static TString GetTypeName() {
            return "user_role";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };


    class TUserRolesInfoProcessor: public TCommonSystemHandler<TUserRolesInfoProcessor> {
    private:
        using TBase = TCommonSystemHandler<TUserRolesInfoProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "user-roles-info";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TUserRolesUpsertProcessor: public TCommonSystemHandler<TUserRolesUpsertProcessor> {
    private:
        using TBase = TCommonSystemHandler<TUserRolesUpsertProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "user-roles-upsert";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

}
