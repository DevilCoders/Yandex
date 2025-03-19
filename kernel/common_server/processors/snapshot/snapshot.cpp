#include "snapshot.h"

#include <kernel/common_server/api/snapshots/objects/mapped/raw_fetcher.h>
#include <kernel/common_server/api/snapshots/abstract.h>

namespace NCS {
    namespace NSnapshots {
        TSnapshotPermissions::TFactory::TRegistrator<TSnapshotPermissions> TSnapshotPermissions::Registrator(TSnapshotPermissions::GetTypeName());

        void TSnapshotsRemoveProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
            TSet<ui32> objectIds;
            for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
                ReqCheckCondition(i.IsString(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect objects list");
                ui32 sId;
                ReqCheckCondition(TryFromString(i.GetString(), sId), ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot id");
                objectIds.emplace(sId);
            }

            auto session = GetServer().GetSnapshotsController().GetSnapshotsManager().BuildNativeSession(false);

            TVector<TDBSnapshot> objects;
            if (!GetServer().GetSnapshotsController().GetSnapshotsManager().RestoreByIds(objectIds, objects, session)) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
            TSet<ui32> remove;
            TSet<ui32> removePermanently;
            for (auto&& i : objects) {
                if (i.GetStatus() == TDBSnapshot::ESnapshotStatus::Ready) {
                    ReqCheckPermissions<TSnapshotPermissions>(permissions, ESnapshotAction::Remove);
                    remove.emplace(i.GetSnapshotId());
                } else if (i.GetStatus() == TDBSnapshot::ESnapshotStatus::Removed) {
                    ReqCheckPermissions<TSnapshotPermissions>(permissions, ESnapshotAction::RemovePermanently);
                    removePermanently.emplace(i.GetSnapshotId());
                }
            }

            if (!GetServer().GetSnapshotsController().GetSnapshotsManager().UpdateForDelete(remove, permissions->GetUserId(), session)) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
            if (!GetServer().GetSnapshotsController().GetSnapshotsManager().RemovePermanently(removePermanently, permissions->GetUserId(), session) || !session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
        }

        void TSnapshotsUpsertProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TSnapshotPermissions>(permissions, ESnapshotAction::Add);

            ReqCheckCondition(GetJsonData()["objects"].IsArray(), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "'objects' is not array");
            TVector<NCS::TDBSnapshot> objects;
            for (auto&& i : GetJsonData()["objects"].GetArraySafe()) {
                NCS::TDBSnapshot object;
                ReqCheckCondition(TBaseDecoder::DeserializeFromJson(object, i), ConfigHttpStatus.SyntaxErrorStatus, ELocalizationCodes::SyntaxUserError, "incorrect object record");
                objects.emplace_back(std::move(object));
            }

            auto session = GetServer().GetSnapshotsController().GetSnapshotsManager().BuildNativeSession(false);
            for (auto&& i : objects) {
                if (!GetServer().GetSnapshotsController().GetSnapshotsManager().UpsertObject(i, permissions->GetUserId(), session)) {
                    session.DoExceptionOnFail(ConfigHttpStatus);
                }
            }
            if (!session.Commit()) {
                session.DoExceptionOnFail(ConfigHttpStatus);
            }
            g.SetCode(HTTP_OK);
        }

        void TSnapshotsInfoProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            ReqCheckPermissions<TSnapshotPermissions>(permissions, ESnapshotAction::Observe);

            const TCgiParameters& cgi = Context->GetCgiParameters();
            const auto actuality = GetTimestamp(cgi, "actuality", Context->GetRequestStartTime());
            TMap<NCS::TDBSnapshot::TId, NCS::TDBSnapshot> objects;

            ReqCheckCondition(GetServer().GetSnapshotsController().GetSnapshotsManager().GetObjects(objects, actuality), ConfigHttpStatus.UnknownErrorStatus, ELocalizationCodes::InternalServerError, "cannot_fetch_settings");
            NJson::TJsonValue objectsJson(NJson::JSON_ARRAY);
            for (auto&& i : objects) {
                objectsJson.AppendValue(i.second.SerializeToJson());
            }
            g.MutableReport().AddReportElement("objects", std::move(objectsJson));
            g.SetCode(HTTP_OK);
        }

        void TSnapshotFillProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) {
            const TString snapshotGroup = GetString(GetCgiParameters(), "snapshot_group");
            const TString snapshotCode = GetString(GetCgiParameters(), "snapshot_code");
            const TString delimiter = GetStringMaybe(GetCgiParameters(), "delimiter", false).GetOrElse(",");
            const bool upsertDiff = GetValue<bool>(GetCgiParameters(), "upsert_diff").GetOrElse(true);
            ReqCheckPermissions<TSnapshotPermissions>(permissions, upsertDiff ? ESnapshotAction::FillUpsert : ESnapshotAction::FillRebuild);
            auto dbSnapshotGroup = GetServer().GetSnapshotsController().GetGroupsManager().GetCustomObject(snapshotGroup);
            ReqCheckCondition(!!dbSnapshotGroup, ConfigHttpStatus.UserErrorState, "incorrect_snapshot_group");

            ReqCheckCondition(!!snapshotCode, ConfigHttpStatus.UserErrorState, "incorrect_snapshot_code");
            auto dbSnapshot = GetServer().GetSnapshotsController().GetSnapshotsManager().GetCustomObject(snapshotCode);
            if (!dbSnapshot) {
                TDBSnapshot snapshot;
                snapshot.SetEnabled(false).SetSnapshotGroupId(snapshotGroup).SetSnapshotCode(snapshotCode);
                ui32 snapshotId;
                ReqCheckCondition(GetServer().GetSnapshotsController().GetSnapshotsManager().StartSnapshotConstruction(snapshot, snapshotId, permissions->GetUserId(), GetServer().GetSnapshotsController()),
                    ConfigHttpStatus.UnknownErrorStatus, "cannot start snapshot");
                dbSnapshot = snapshot;
                dbSnapshot->SetSnapshotId(snapshotId);
            }
            ReqCheckCondition(!!dbSnapshot, ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_construction_snapshot");

            THolder<NCS::NSnapshots::TRecordsSnapshotFetcher> fetcher = MakeHolder<NCS::NSnapshots::TRecordsSnapshotFetcher>();
            fetcher->SetRawData(GetRawData());
            fetcher->SetDelimiter(delimiter);
            fetcher->SetUpsertDiffMode(upsertDiff);
            ReqCheckCondition(fetcher->FetchData(*dbSnapshot, GetServer().GetSnapshotsController(), GetServer(), permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot_fetch_snapshot_data");
        }
    }
}
