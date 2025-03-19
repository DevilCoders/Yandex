#include "idm.h"
#include "permissions.h"

#include <kernel/common_server/roles/db_roles/manager.h>

namespace NCS {

    constexpr TStringBuf GROUP_PREFIX("staff_group:");


    template<class TDerived>
    void NCS::TIdmProcessorBase<TDerived>::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        try {
            DoProcessRequestWithPermissions(g, permissions);
        } catch (const TCodedException& e) {
            g.MutableReport().AddReportElement("code", e.GetCode());
            g.MutableReport().AddReportElement("error", e.GetErrorMessage());
        }
    }

    template<class TDerived>
    TString NCS::TIdmProcessorBase<TDerived>::GetRoleName() const {
        TCgiParameters cgi(this->GetRawData().AsStringBuf());
        const auto role = this->GetString(this->GetXFormUrlEncodedData(), "role");;
        NJson::TJsonValue json;
        this->ReqCheckCondition(NJson::ReadJsonTree(role, &json, false), this->ConfigHttpStatus.SyntaxErrorStatus, "role is not json");
        return this->GetString(json, this->Config.GetSlug(), true, true);
    }

    template<class TDerived>
    TString NCS::TIdmProcessorBase<TDerived>::GetUid() const {
        const auto data = this->GetXFormUrlEncodedData();
        if (const auto group = this->GetString(data, "group", false)) {
            return GROUP_PREFIX + group;
        }
        return this->GetString(this->GetXFormUrlEncodedData(), "uid", true, true);
    }

    void TIdmRemoveRoleProcessor::DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Remove);
        const auto role = GetRoleName();
        ReqCheckPermissions<TRolePermissions>(permissions, role);
        const auto uid = GetUid();
        TUserRolesCompiled userRoles;
        ReqCheckCondition(GetServer().GetPermissionsManager().Restore(uid, userRoles), ConfigHttpStatus.UnknownErrorStatus, "cannot restore roles for user:" + uid);
        if (userRoles.MutableRoleNames().erase(role)) {
            ReqCheckCondition(GetServer().GetPermissionsManager().Upsert(userRoles, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot store roles for user:" + uid);
        }
        g.MutableReport().AddReportElement("code", 0);
    }

    void TIdmAddRoleProcessor::DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
        const auto role = GetRoleName();
        ReqCheckPermissions<TRolePermissions>(permissions, role);
        const auto uid = GetUid();
        TUserRolesCompiled userRoles;
        ReqCheckCondition(GetServer().GetPermissionsManager().Restore(uid, userRoles), ConfigHttpStatus.UnknownErrorStatus, "cannot restore roles for user:" + uid);
        if (userRoles.MutableRoleNames().emplace(role).second) {
            ReqCheckCondition(GetServer().GetPermissionsManager().Upsert(userRoles, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot store roles for user:" + uid);
        }
        g.MutableReport().AddReportElement("code", 0);
    }

    void TIdmInfoProcessor::DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);
        const TDBFullRolesManager* dbRolesManager = dynamic_cast<const TDBFullRolesManager*>(&GetServer().GetRolesManager());
        ReqCheckCondition(dbRolesManager, ConfigHttpStatus.UnknownErrorStatus, "roles manager is not compatible");
        TVector<TDBRole> dbRoles;
        ReqCheckCondition(dbRolesManager->GetRolesManager().GetAllObjects(dbRoles), ConfigHttpStatus.UnknownErrorStatus, "cannot get roles");
        g.MutableReport().AddReportElement("code", 0);
        NJson::TJsonValue rolesJson(NJson::JSON_MAP);
        rolesJson["slug"] = Config.GetSlug();
        rolesJson["name"] = Config.GetSlugRu();
        auto& valuesJson = rolesJson.InsertValue("values", NJson::JSON_ARRAY);
        for (const auto& role: dbRoles) {
            if (permissions->Check<TRolePermissions>(role.GetRoleName())) {
                auto& roleJson = valuesJson[role.GetRoleName()];
                roleJson["name"] = role.GetRoleName();
                roleJson["unique_id"] = ::ToString(role.GetRoleId());
            }
        }
        g.MutableReport().AddReportElement("roles", std::move(rolesJson));
    }

    void TIdmGetAllRolesProcessor::DoProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);
        TVector<TUserRolesCompiled> objectLinks;
        ReqCheckCondition(GetServer().GetPermissionsManager().RestoreAll(objectLinks), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_items");
        g.MutableReport().AddReportElement("code", 0);
        NJson::TJsonValue usersJson(NJson::JSON_ARRAY);
        NJson::TJsonValue groupsJson(NJson::JSON_ARRAY);
        for (const auto& user: objectLinks) {
            TStringBuf userId(user.GetSystemUserId());
            const bool isGroup = userId.SkipPrefix(GROUP_PREFIX);
            ui64 id;
            if (!TryFromString(userId, id)) { // not uid user
                continue;
            }
            NJson::TJsonValue userRoles(NJson::JSON_ARRAY);
            for (const auto& role: user.GetRoleNames()) {
                if (permissions->Check<TRolePermissions>(role)) {
                    auto& lastRole = userRoles.AppendValue(NJson::JSON_MAP);
                    lastRole[Config.GetSlug()] = role;
                }
            }
            if (userRoles.GetArray().empty()) {
                continue;
            }
            if (isGroup) {
                auto& userJson = groupsJson.AppendValue(NJson::JSON_MAP);
                userJson["group"] = id;
                userJson["roles"] = std::move(userRoles);
            } else {
                auto& userJson = usersJson.AppendValue(NJson::JSON_MAP);
                userJson["uid"] = userId;
                userJson["roles"] = std::move(userRoles);
            }
        }
        g.MutableReport().AddReportElement("users", std::move(usersJson));
        g.MutableReport().AddReportElement("groups", std::move(groupsJson));
    }

    bool TIdmProcessorConfig::InitFeatures(const TYandexConfig::Section* section) {
        const auto& dir = section->GetDirectives();
        if (!dir.GetNonEmptyValue("Slug", Slug)) {
            TFLEventLog::Error("Slug must be set");
            return false;
        }
        SlugRu = dir.Value("SlugRu", Slug);
        return true;
    }

    void TIdmProcessorConfig::ToStringFeatures(IOutputStream& os) const {
        os << "Slug: " << Slug << Endl;
        os << "SlugRu: " << SlugRu << Endl;
    }
}
