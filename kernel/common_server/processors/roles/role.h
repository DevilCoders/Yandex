#pragma once

#include "permissions.h"
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TRolesInfoProcessor: public TCommonSystemHandler<TRolesInfoProcessor> {
    private:
        using TBase = TCommonSystemHandler<TRolesInfoProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "roles-info";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TRolesUpsertProcessor: public TCommonSystemHandler<TRolesUpsertProcessor> {
    private:
        using TBase = TCommonSystemHandler<TRolesUpsertProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "roles-upsert";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TRolesRemoveProcessor: public TCommonSystemHandler<TRolesRemoveProcessor> {
    private:
        using TBase = TCommonSystemHandler<TRolesRemoveProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "roles-remove";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

    class TUserRolesUpsertingProposition: public TCommonSystemHandler<TUserRolesUpsertingProposition> {
    private:
        using TBase = TCommonSystemHandler<TUserRolesUpsertingProposition>;

    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "roles-propose-upsert";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

    class TUserRolesRemovingProposition: public TCommonSystemHandler<TUserRolesRemovingProposition> {
    private:
        using TBase = TCommonSystemHandler<TUserRolesRemovingProposition>;

    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "roles-propose-remove";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override;
    };

}
