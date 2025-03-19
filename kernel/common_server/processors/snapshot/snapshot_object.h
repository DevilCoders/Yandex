#pragma once
#include <kernel/common_server/processors/common/handler.h>
#include <kernel/common_server/roles/actions/common.h>
#include <kernel/common_server/api/snapshots/abstract.h>

namespace NCS {
    namespace NSnapshots {
        template <class TSnapshotObject, class TServer = IBaseServer>
        class TObjectInfoHandler: public NCS::NHandlers::TCommonInfo<TObjectInfoHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer> {
            using TBase = NCS::NHandlers::TCommonInfo<TObjectInfoHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer>;
        protected:
            using TBase::GetServer;
            using TBase::GetJsonData;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;
            void ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr /*permissions*/) {
                TVector<NStorage::TTableRecordWT> objectIds;
                ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "object_ids", objectIds, true), ConfigHttpStatus.SyntaxErrorStatus, "incorrect object_ids field");
                auto& sc = GetServer().GetSnapshotsController();
                TMaybe<TDBSnapshot> dbSnapshot;
                if (GetJsonData().Has("snapshot_code")) {
                    TString snapshotCode;
                    ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "snapshot_code", snapshotCode, true), ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot_code field");
                    dbSnapshot = sc.GetSnapshotsManager().GetCustomObject(snapshotCode);
                } else if (GetJsonData().Has("group_id")) {
                    TString groupId;
                    ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "group_id", groupId, true), ConfigHttpStatus.SyntaxErrorStatus, "incorrect group_id field");
                    dbSnapshot = sc.GetSnapshotsManager().GetActualSnapshotInfo(groupId);
                } else {
                    ReqCheckCondition(false, ConfigHttpStatus.SyntaxErrorStatus, "neither snapshot_id nor group_id fields");
                }
                ReqCheckCondition(!!dbSnapshot, ConfigHttpStatus.UserErrorState, "cannot fetch snapshot info");
                NJson::TJsonValue jsonInfosByObjectId = NJson::JSON_ARRAY;
                if (objectIds.size()) {
                    TVector<TSnapshotObject> info;
                    ReqCheckCondition(sc.GetObjectInfo(objectIds, *dbSnapshot, info), ConfigHttpStatus.UnknownErrorStatus, "cannot fetch info for objects");
                    for (auto&& i : info) {
                        jsonInfosByObjectId.AppendValue(i.SerializeToJson());
                    }
                } else {
                    TVector<TMappedObject> info;
                    NCS::NSnapshots::TObjectsFilter objectsFilter;
                    ReqCheckCondition(TJsonProcessor::ReadObject(GetJsonData(), "filter", objectsFilter), ConfigHttpStatus.SyntaxErrorStatus, "incorrect filter info");
                    if (objectsFilter.HasLimit()) {
                        ++objectsFilter.GetLimitUnsafe();
                    }
                    auto dbObjectsManager = sc.GetSnapshotObjectsManager(*dbSnapshot);
                    ReqCheckCondition(!!dbObjectsManager, ConfigHttpStatus.UserErrorState, "incorrect objects manager for snapshot");
                    ReqCheckCondition(dbObjectsManager->GetAllObjects(dbSnapshot->GetSnapshotId(), info, objectsFilter), ConfigHttpStatus.UnknownErrorStatus, "cannot take all objects");
                    if (objectsFilter.HasLimit() && info.size() == objectsFilter.GetLimitUnsafe()) {
                        info.pop_back();
                        g.AddReportElement("has_more", true);
                    } else {
                        g.AddReportElement("has_more", false);
                    }

                    for (auto&& i : info) {
                        jsonInfosByObjectId.AppendValue(i.SerializeToJson());
                    }
                }
                g.AddReportElement("objects", std::move(jsonInfosByObjectId));

            }

            static TString GetTypeName() {
                return "snapshot_" + TSnapshotObject::GetTypeName() + "_info";
            }
        };

        template <class TSnapshotObject, class TServer = IBaseServer>
        class TObjectUpsertHandler: public NCS::NHandlers::TCommonUpsert<TObjectUpsertHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer> {
            using TBase = NCS::NHandlers::TCommonUpsert<TObjectUpsertHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer>;
        protected:
            using TBase::GetServer;
            using TBase::GetJsonData;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckCondition;
        public:
            using TBase::TBase;
            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr permissions) override {
                TVector<TSnapshotObject> objects;
                ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects, true), ConfigHttpStatus.SyntaxErrorStatus, "incorrect objects field");
                auto& sc = GetServer().GetSnapshotsController();
                TMaybe<TDBSnapshot> dbSnapshot;
                {
                    TString snapshotCode;
                    ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "snapshot_code", snapshotCode, false), ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot_code field");
                    if (snapshotCode) {
                        dbSnapshot = sc.GetSnapshotsManager().GetCustomObject(snapshotCode);
                    } else {
                        TString groupId;
                        ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "group_id", groupId, false), ConfigHttpStatus.SyntaxErrorStatus, "incorrect group_id field");
                        dbSnapshot = sc.GetSnapshotsManager().GetActualSnapshotConstruction(groupId);
                    }
                    ReqCheckCondition(!!dbSnapshot, ConfigHttpStatus.UserErrorState, "cannot restore snapshot object");
                }
                auto objectsOperator = sc.GetSnapshotObjectsManager(*dbSnapshot);
                ReqCheckCondition(!!objectsOperator, ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot operator");
                ReqCheckCondition(objectsOperator->UpsertObjects(objects, dbSnapshot->GetSnapshotId(), permissions->GetUserId()), ConfigHttpStatus.UnknownErrorStatus, "cannot put objects");
            }

            static TString GetTypeName() {
                return "snapshot_" + TSnapshotObject::GetTypeName() + "_upsert";
            }
        };

        template <class TSnapshotObject, class TServer = IBaseServer>
        class TObjectRemoveHandler: public NCS::NHandlers::TCommonRemove<TObjectRemoveHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer> {
            using TBase = NCS::NHandlers::TCommonRemove<TObjectRemoveHandler<TSnapshotObject, TServer>, TSnapshotObject, TSystemUserPermissions, TServer>;
        protected:
            using TBase::GetServer;
            using TBase::GetJsonData;
            using TBase::ConfigHttpStatus;
            using TBase::ReqCheckCondition;
            virtual bool DoFillHandlerScheme(NCS::NScheme::THandlerScheme& scheme, const IBaseServer& /*server*/) const override {
                auto& rMethod = scheme.Method(NCS::NScheme::ERequestMethod::Post);
                auto& hScheme = rMethod.Body().Content("application/json");
                hScheme.Add<TFSString>("snapshot_code");
                auto& fScheme = hScheme.Add<TFSArray>("objects").SetElement<TFSStructure>().SetStructure();
                fScheme.Add<TFSString>("*");
                rMethod.Response(HTTP_OK);
                return true;
            }
        public:
            using TBase::TBase;
            virtual void ProcessRequestWithPermissions(TJsonReport::TGuard& /*g*/, TSystemUserPermissions::TPtr /*permissions*/) override {
                TVector<NStorage::TTableRecordWT> objects;
                ReqCheckCondition(TJsonProcessor::ReadObjectsContainer(GetJsonData(), "objects", objects, true), ConfigHttpStatus.SyntaxErrorStatus, "incorrect object_ids field");
                auto& sc = GetServer().GetSnapshotsController();
                TMaybe<TDBSnapshot> dbSnapshot;
                {
                    TString snapshotCode;
                    ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "snapshot_code", snapshotCode, false), ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot_code field");
                    if (snapshotCode) {
                        dbSnapshot = sc.GetSnapshotsManager().GetCustomObject(snapshotCode);
                    } else {
                        TString groupId;
                        ReqCheckCondition(TJsonProcessor::Read(GetJsonData(), "group_id", groupId, false), ConfigHttpStatus.SyntaxErrorStatus, "incorrect group_id field");
                        dbSnapshot = sc.GetSnapshotsManager().GetActualSnapshotInfo(groupId);
                    }
                }
                ReqCheckCondition(!!dbSnapshot, ConfigHttpStatus.UserErrorState, "snapshot not ready for objects insertion");
                auto objectsOperator = sc.GetSnapshotObjectsManager(*dbSnapshot);
                ReqCheckCondition(!!objectsOperator, ConfigHttpStatus.SyntaxErrorStatus, "incorrect snapshot operator");
                ReqCheckCondition(objectsOperator->RemoveSnapshotObjects(objects, dbSnapshot->GetSnapshotId()), ConfigHttpStatus.UnknownErrorStatus, "cannot put objects");
            }

            static TString GetTypeName() {
                return "snapshot_" + TSnapshotObject::GetTypeName() + "_remove";
            }
        };
    }
}
