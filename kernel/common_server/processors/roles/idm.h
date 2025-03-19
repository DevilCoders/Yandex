#pragma once

#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    class TIdmProcessorConfig {
        CSA_READONLY_DEF(TString, Slug);
        CSA_READONLY_DEF(TString, SlugRu);
    public:
        bool InitFeatures(const TYandexConfig::Section* section);
        void ToStringFeatures(IOutputStream& os) const;
    };

    template<class TDerived>
    class TIdmProcessorBase : public TCommonSystemHandler<TDerived, TIdmProcessorConfig> {
    private:
        using TBase = TCommonSystemHandler<TDerived, TIdmProcessorConfig>;
    public:
        using TBase::TBase;
        virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override final;

    protected:
        virtual void DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) = 0;
        TString GetUid() const;
        TString GetRoleName() const;
    };

    class TIdmInfoProcessor: public TIdmProcessorBase<TIdmInfoProcessor> {
    private:
        using TBase = TIdmProcessorBase<TIdmInfoProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "idm-info";
        }

        virtual void DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TIdmAddRoleProcessor: public TIdmProcessorBase<TIdmAddRoleProcessor> {
    private:
        using TBase = TIdmProcessorBase<TIdmAddRoleProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "idm-add-role";
        }

        virtual void DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };


    class TIdmRemoveRoleProcessor: public TIdmProcessorBase<TIdmRemoveRoleProcessor> {
    private:
        using TBase = TIdmProcessorBase<TIdmRemoveRoleProcessor>;
    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "idm-remove-role";
        }

        virtual void DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

    class TIdmGetAllRolesProcessor: public TIdmProcessorBase<TIdmGetAllRolesProcessor> {
    private:
        using TBase = TIdmProcessorBase<TIdmGetAllRolesProcessor>;

    public:
        using TBase::TBase;

        static TString GetTypeName() {
            return "idm-get-all-roles";
        }

        virtual void DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) override;
    };

}
