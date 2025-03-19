#include "role.h"

#include <kernel/common_server/proposition/manager.h>
#include <kernel/common_server/processors/proposition/permission.h>
#include <kernel/common_server/proposition/actions/role.h>

namespace NCS {

    void TRolesRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Remove);

        TSet<TString> objectIds;
        ReqCheckCondition(TJsonProcessor::ReadContainer(GetJsonData(), "objects", objectIds), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "cannot parse object_ids");
        ReqCheckCondition(GetServer().GetRolesManager().RemoveRoles(objectIds, permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot remove roles");
    }

    void TRolesUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
        TVector<TUserRoleInfo> objects;
        ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects array");
        for (auto&& i : objects) {
            ReqCheckCondition(GetServer().GetRolesManager().UpsertRole(i, permissions->GetUserId(), nullptr), ConfigHttpStatus.UnknownErrorStatus, "cannot_store_role:" + i.GetRoleName());
        }
    }

    void TRolesInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);

        TVector<TUserRoleInfo> objects;
        ReqCheckCondition(GetServer().GetRolesManager().GetRoles(objects), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");

        NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
        for (auto&& object : objects) {
            objectsJson.AppendValue(object.SerializeToJson());
        }
        g.MutableReport().AddReportElement("objects", std::move(objectsJson));
    }

    void TUserRolesUpsertingProposition::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckPermissions<TRolePermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);
        TVector<TUserRoleInfo> objects;
        ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects array");

        auto session = GetServer().GetPropositionsManager() ->BuildNativeSession(false);
        for (auto&& i : objects) {
            NPropositions::TDBProposition proposition(new NPropositions::TRoleActionUpsert(i));
            ReqCheckCondition(proposition.CheckPolicy(GetServer()), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, NPropositions::TDBProposition::GetTableName() + " wrong proposition policy configuration");
            TBase::template ReqCheckPermissions<NHandlers::TPropositionPermissions>(permissions, NHandlers::TPropositionPermissions::EObjectAction::Add, proposition.GetProposedObject());
            if (!GetServer().GetPropositionsManager()->UpsertObject(proposition, permissions->GetUserId(), session, nullptr, nullptr)) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }
        if (!session.Commit()) {
            session.DoExceptionOnFail(ConfigHttpStatus);
        }
    }

    void TUserRolesRemovingProposition::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
        ReqCheckCondition(!!GetServer().GetPropositionsManager(), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "propositions manager not configured");

        TVector<TUserRoleInfo> objects;
        ReqCheckCondition(GetServer().GetRolesManager().GetRoles(objects), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_objects");

        auto session = GetServer().GetPropositionsManager()->BuildNativeSession(false);
        for (auto&& i : objects) {
            NPropositions::TDBProposition proposition(new NPropositions::TRoleActionRemove(i));
            ReqCheckCondition(proposition.CheckPolicy(GetServer()), ConfigHttpStatus.UserErrorState, ELocalizationCodes::SyntaxUserError, NPropositions::TDBProposition::GetTableName() + " wrong proposition policy configuration");
            TBase::template ReqCheckPermissions<NHandlers::TPropositionPermissions>(permissions, NHandlers::TPropositionPermissions::EObjectAction::Add, proposition.GetProposedObject());
            if (!GetServer().GetPropositionsManager()->UpsertObject(proposition, permissions->GetUserId(), session, nullptr, nullptr)) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }
        if (!session.Commit()) {
            session.DoExceptionOnFail(ConfigHttpStatus);
        }
    }
}
