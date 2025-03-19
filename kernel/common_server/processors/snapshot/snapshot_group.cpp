#include "snapshot_group.h"
#include <kernel/common_server/api/snapshots/abstract.h>

namespace NCS {
    namespace NSnapshots {

        TSnapshotsGroupPermissions::TFactory::TRegistrator<TSnapshotsGroupPermissions> TSnapshotsGroupPermissions::Registrator(TSnapshotsGroupPermissions::GetTypeName());

        void TSnapshotGroupsRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TSnapshotsGroupPermissions>(permissions, TAdministrativePermissions::EObjectAction::Remove);

            ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
            TSet<NCS::TDBSnapshotsGroup::TId> objectIds;
            for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
                ReqCheckCondition(i.IsString(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects list");
                objectIds.emplace(i.GetString());
            }

            auto session = GetServer().GetSnapshotsController().GetSnapshotsManager().BuildNativeSession(false);
            if (!GetServer().GetSnapshotsController().GetGroupsManager().RemoveObject(objectIds, permissions->GetUserId(), session) || !session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        void TSnapshotGroupsUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TSnapshotsGroupPermissions>(permissions, TAdministrativePermissions::EObjectAction::Add);

            ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
            TVector<NCS::TDBSnapshotsGroup> objects;
            for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
                NCS::TDBSnapshotsGroup object;
                ReqCheckCondition(object.DeserializeFromJson(i), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect object record");
                objects.emplace_back(std::move(object));
            }

            auto session = GetServer().GetSnapshotsController().GetSnapshotsManager().BuildNativeSession(false);
            for (auto&& i : objects) {
                if (!GetServer().GetSnapshotsController().GetGroupsManager().UpsertObject(i, permissions->GetUserId(), session)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
            if (!session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        void TSnapshotGroupsInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TSnapshotsGroupPermissions>(permissions, TAdministrativePermissions::EObjectAction::Observe);

            const TCgiParameters& cgi = Context->GetCgiParameters();
            const auto actuality = GetTimestamp(cgi, "actuality", Context->GetRequestStartTime());
            TMap<NCS::TDBSnapshotsGroup::TId, NCS::TDBSnapshotsGroup> objects;

            ReqCheckCondition(GetServer().GetSnapshotsController().GetGroupsManager().GetObjects(objects, actuality), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_settings");
            NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
            for (auto&& i : objects) {
                objectsJson.AppendValue(i.second.SerializeToJson());
            }
            g.MutableReport().AddReportElement("objects", std::move(objectsJson));
        }
    }
}
