#pragma once
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/abstract/frontend.h>

class TUserRoleContainer {
private:
    CSA_PROTECTED_DEF(TUserRoleContainer, TString, RoleName);
    CSA_PROTECTED(TUserRoleContainer, ui64, Revision, 0);
public:
    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
    static NFrontend::TScheme GetScheme(const IBaseServer& server);
};

class TUserRoleInfo: public TUserRoleContainer {
private:
    using TBase = TUserRoleContainer;
    CSA_DEFAULT(TUserRoleInfo, TSet<TString>, ItemIds);
    CSA_DEFAULT(TUserRoleInfo, TSet<TString>, RoleIds);
public:
    TUserRoleInfo() = default;
    TUserRoleInfo(const TUserRoleContainer& base)
        : TBase(base)
    {

    }
    NJson::TJsonValue SerializeToJson() const;
    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

    static NFrontend::TScheme GetScheme(const IBaseServer& server);
};

