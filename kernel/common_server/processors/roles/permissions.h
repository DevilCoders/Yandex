#pragma once

#include <kernel/common_server/roles/actions/common.h>

namespace NCS {

    class TRolePermissions: public TAdministrativePermissions {
    private:
        using TBase = TAdministrativePermissions;
        static TFactory::TRegistrator<TRolePermissions> Registrator;
        CSA_READONLY_DEF(TSet<TString>, ManagedRoles);

    public:
        static TString GetTypeName();
        virtual TString GetClassName() const override;
        bool Check(const EObjectAction& action) const;
        bool Check(const TString& roleName) const;
        virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
        virtual NJson::TJsonValue SerializeToJson() const override;
        virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override;
    };

}
