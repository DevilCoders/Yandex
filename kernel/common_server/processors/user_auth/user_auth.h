#pragma once
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TUserAuthPermissions: public TSearchAdministrativePermissions {
    private:
        using TBase = TSearchAdministrativePermissions;
        static TFactory::TRegistrator<TUserAuthPermissions> Registrator;

    public:
        static TString GetTypeName() {
            return "auth_user";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }
    };


    class TAuthUsersInfoProcessor: public TCommonSystemHandler<TAuthUsersInfoProcessor> {
    private:
        using TBase = TCommonSystemHandler<TAuthUsersInfoProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "auth-users-info";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TAuthUsersUpsertProcessor: public TCommonSystemHandler<TAuthUsersUpsertProcessor> {
    private:
        using TBase = TCommonSystemHandler<TAuthUsersUpsertProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "auth-users-upsert";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TAuthUsersRemoveProcessor: public TCommonSystemHandler<TAuthUsersRemoveProcessor> {
    private:
        using TBase = TCommonSystemHandler<TAuthUsersRemoveProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "auth-users-remove";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

}
