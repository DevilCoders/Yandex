#pragma once
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TActionPermissions: public TAdministrativePermissions {
    private:
        using TBase = TAdministrativePermissions;
        static TFactory::TRegistrator<TActionPermissions> Registrator;

    public:
        static TString GetTypeName() {
            return "action";
        }

        virtual TString GetClassName() const override {
            return GetTypeName();
        }

        bool Check(const EObjectAction& action) const {
            return GetActions().contains(action);
        }
    };


    class TActionsInfoProcessor: public TCommonSystemHandler<TActionsInfoProcessor> {
    private:
        using TBase = TCommonSystemHandler<TActionsInfoProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "actions-info";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TActionsUpsertProcessor: public TCommonSystemHandler<TActionsUpsertProcessor> {
    private:
        using TBase = TCommonSystemHandler<TActionsUpsertProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "actions-upsert";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TActionsRemoveProcessor: public TCommonSystemHandler<TActionsRemoveProcessor> {
    private:
        using TBase = TCommonSystemHandler<TActionsRemoveProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "actions-remove";
        }

        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

}
