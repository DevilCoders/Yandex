#pragma once
#include "manager.h"
#include "cache.h"
#include "db_entities_history.h"
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/algorithm/container.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <library/cpp/threading/named_lock/named_lock.h>
#include "db_entities.h"

namespace NCS {
    namespace NPreparation {
        class IAction {
        protected:
            virtual bool DoExecute() = 0;
            virtual void FillLogsContext(NLogging::TLogThreadContext::TGuard& /*logContext*/) const {

            }
        public:
            using TPtr = TAtomicSharedPtr<IAction>;
            virtual ~IAction() = default;
            virtual bool Execute() {
                return DoExecute();
            }

            virtual TString GetClassName() const = 0;
            virtual TString GetActionId() const = 0;
            NLogging::TLogThreadContext::TGuard BuildLogContext() {
                auto result = TFLRecords::StartContext()("&class_name", GetClassName());
                FillLogsContext(result);
                return result;
            }
        };

        class TDBEmptyObject {
        public:
            template <class TServer>
            static NFrontend::TScheme GetScheme(const TServer& /*server*/) {
                NFrontend::TScheme result;
                return result;
            }

            class TDecoder: public TBaseDecoder {
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& /*decoderBase*/) {
                }
            };
            NJson::TJsonValue SerializeToJson() const {
                return NJson::JSON_MAP;
            }

            bool DeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) {
                return true;
            }

            bool DeserializeWithDecoder(const TDecoder& /*decoder*/, const TConstArrayRef<TStringBuf>& /*values*/) {
                return true;
            }
            NStorage::TTableRecord SerializeToTableRecord() const {
                NStorage::TTableRecord result;
                return result;
            }

        };

        class TMarker {
        private:
            CSA_DEFAULT(TMarker, TString, TableName);
            CSA_DEFAULT(TMarker, TString, ObjectId);
            CSA_DEFAULT(TMarker, TString, ActionId);
        public:
            class TDecoder: public TBaseDecoder {
            private:
                DECODER_FIELD(TableName);
                DECODER_FIELD(ObjectId);
                DECODER_FIELD(ActionId);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase) {
                    TableName = GetFieldDecodeIndex("table_name", decoderBase);
                    ObjectId = GetFieldDecodeIndex("object_id", decoderBase);
                    ActionId = GetFieldDecodeIndex("action_id", decoderBase);
                }
            };

            bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
                READ_DECODER_VALUE(decoder, values, TableName);
                READ_DECODER_VALUE(decoder, values, ObjectId);
                READ_DECODER_VALUE(decoder, values, ActionId);
                return true;
            }
            NStorage::TTableRecord SerializeToTableRecord() const {
                NStorage::TTableRecord result;
                result.Set("table_name", TableName);
                result.Set("object_id", ObjectId);
                result.Set("action_id", ActionId);
                return result;
            }

        };

        template <class TBaseObject>
        class TBaseRevisionedObject: public TBaseObject {
        private:
            using TBase = TBaseObject;
            CS_ACCESS(TBaseRevisionedObject, ui32, Revision, 0);
        public:
            using TBase::TBase;
            template <class TServer>
            static NFrontend::TScheme GetScheme(const TServer& server) {
                NFrontend::TScheme result = TBaseObject::GetScheme(server);
                result.Add<TFSNumeric>("revision").SetDefault(0).Required().ReadOnly();
                return result;
            }

            class TDecoder: public TBase::TDecoder {
            private:
                using TBaseDecoder = typename TBase::TDecoder;
                DECODER_FIELD(Revision);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase)
                    : TBaseDecoder(decoderBase)
                {
                    Revision = TBaseDecoder::GetFieldDecodeIndex("revision", decoderBase);
                }
            };
            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = TBase::SerializeToJson();
                result.InsertValue("revision", Revision);
                return result;
            }

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TBase::DeserializeFromJson(jsonInfo)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
                    return false;
                }
                return true;
            }

            bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
                if (!TBase::DeserializeWithDecoder(decoder, values)) {
                    return false;
                }
                READ_DECODER_VALUE(decoder, values, Revision);
                return true;
            }
            NStorage::TTableRecord SerializeToTableRecord() const {
                NStorage::TTableRecord result = TBase::SerializeToTableRecord();
                result.SetNotEmpty("revision", Revision);
                return result;
            }
            TMaybe<ui32> GetRevisionMaybe() const {
                return Revision;
            }

        };

        using TRevisionedObject = TBaseRevisionedObject<TDBEmptyObject>;

        template <class TBaseObject>
        class TPrepareObject: public TBaseObject {
        private:
            using TBase = TBaseObject;
            CS_ACCESS(TPrepareObject, ui32, PreparedRevision, 0);
            CS_ACCESS(TPrepareObject, ui32, PreparationAlgorithm, 0);
        public:
            bool IsPreparedObject(const ui32 currentAlgorithm) const {
                return PreparedRevision == TBase::GetRevision() && PreparationAlgorithm == currentAlgorithm;
            }
            bool IsPreparedObject() const {
                return PreparedRevision == TBase::GetRevision();
            }
            static NFrontend::TScheme GetScheme() {
                NFrontend::TScheme result = TBase::GetScheme();
                result.Add<TFSNumeric>("prepared_revision").SetDefault(0).Required().ReadOnly();
                result.Add<TFSNumeric>("preparation_algorithm").SetDefault(0).Required().ReadOnly();
                return result;
            }

            class TDecoder: public TBase::TDecoder {
            private:
                using TBaseDecoder = typename TBaseObject::TDecoder;
                DECODER_FIELD(PreparedRevision);
                DECODER_FIELD(PreparationAlgorithm);
            public:
                TDecoder() = default;
                TDecoder(const TMap<TString, ui32>& decoderBase)
                    : TBaseDecoder(decoderBase) {
                    PreparedRevision = TBaseDecoder::GetFieldDecodeIndex("prepared_revision", decoderBase);
                    PreparationAlgorithm = TBaseDecoder::GetFieldDecodeIndex("preparation_algorithm", decoderBase);
                }
            };
            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = TBase::SerializeToJson();
                result.InsertValue("prepared_revision", PreparedRevision);
                result.InsertValue("preparation_algorithm", PreparationAlgorithm);
                return result;
            }

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                if (!TBase::DeserializeFromJson(jsonInfo)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "prepared_revision", PreparedRevision)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "preparation_algorithm", PreparationAlgorithm)) {
                    return false;
                }
                return true;
            }

            bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
                if (!TBase::DeserializeWithDecoder(decoder, values)) {
                    return false;
                }
                READ_DECODER_VALUE(decoder, values, PreparedRevision);
                READ_DECODER_VALUE(decoder, values, PreparationAlgorithm);
                return true;
            }
            NStorage::TTableRecord SerializeToTableRecord() const {
                NStorage::TTableRecord result = TBase::SerializeToTableRecord();
                return result;
            }
        };

        template <class TBaseDBObject, class TSnapshotsConstructor, ui32 CurrentAlgorithm>
        class TManager: public TDBMetaEntitiesManager<TBaseDBObject, TSnapshotsConstructor> {
        private:
            using TBase = TDBMetaEntitiesManager<TBaseDBObject, TSnapshotsConstructor>;
            using TPrepareObject = TPrepareObject<TBaseDBObject>;
        public:
            using TEntity = typename TBase::TEntity;
        private:
            bool UpdatePreparedRevision(const typename TEntity::TId id, const ui32 revision, const ui32 currentAlgorithm, const TString& userId, NCS::TEntitySession& session) const {
                NCS::NStorage::TTableRecord update;
                NCS::NStorage::TTableRecord condition;
                return TBase::UpdateRecord(
                    update("prepared_revision", revision)("preparation_algorithm", currentAlgorithm),
                    condition(TEntity::GetIdFieldName(), id), userId, session, nullptr, 1, EObjectHistoryAction::UpdateData, false);
            }

            TThreadPool ThreadPool;

            class TPreparationThread: public IObjectInQueue {
            private:
                TManager* Owner = nullptr;
            public:
                TPreparationThread(TManager* owner)
                    : Owner(owner) {

                }

                virtual void Process(void*) override {
                    while (Owner->IsActive()) {
                        Owner->PrepareAll(false);
                        Sleep(TDuration::Seconds(1));
                    }
                }
            };

            bool RestoreMarkers(const TEntity& object, TSet<TString>& result) const {
                auto session = TBase::BuildNativeSession(true);
                NCS::NStorage::TObjectRecordsSet<TMarker> markers;
                TSRSelect reqSelect("preparation_markers", &markers);
                auto& srMulti = reqSelect.RetCondition<TSRMulti>();
                srMulti.InitNode<TSRBinary>("table_name", TEntity::GetTableName());
                srMulti.InitNode<TSRBinary>("object_id", ::ToString(object.GetInternalId()));
                if (!session.ExecRequest(reqSelect)) {
                    return false;
                }
                TSet<TString> resultLocal;
                for (auto&& i : markers) {
                    resultLocal.emplace(i.GetActionId());
                }
                std::swap(resultLocal, result);
                return true;
            }

            bool PutMarker(const TEntity& object, const TString& actionId) const {
                TMarker marker;
                marker.SetTableName(TEntity::GetTableName())
                    .SetObjectId(::ToString(object.GetInternalId())).SetActionId(actionId);
                TSRInsert reqInsert("preparation_markers");
                reqInsert.AddRecord(marker);

                auto session = TBase::BuildNativeSession(false);
                if (!session.ExecRequest(reqInsert) || !session.Commit()) {
                    return false;
                }
                return true;
            }

            bool SyncMarkers(const TEntity& object, const TSet<TString>& actionIds) const {
                TSRDelete reqDelete("preparation_markers");
                auto& srMulti = reqDelete.RetCondition<TSRMulti>();
                srMulti.InitNode<TSRBinary>("table_name", TEntity::GetTableName());
                srMulti.InitNode<TSRBinary>("object_id", ::ToString(object.GetInternalId()));
                if (actionIds.size()) {
                    srMulti.InitNode<TSRBinary>("action_id", actionIds, ESRBinary::NotIn);
                }

                auto session = TBase::BuildNativeSession(false);
                if (!session.ExecRequest(reqDelete) || !session.Commit()) {
                    return false;
                }
                return true;
            }

            bool PrepareAll(const bool isStart) {
                auto gLogging = TFLRecords::StartContext()("&table_name", TEntity::GetTableName()).SignalId("preparation");
                auto lock = TBase::GetDatabase()->Lock("prepare-" + TEntity::GetTableName(), true, isStart ? TDuration::Hours(1) : TDuration::Zero());
                CHECK_WITH_LOG(!isStart || !!lock);
                if (!lock) {
                    TFLEventLog::Signal()("&code", "non_locked");
                    return true;
                }
                TFLEventLog::Signal()("&code", "full_started");
                NCS::NStorage::TObjectRecordsSet<TPrepareObject> entities;
                {
                    TSRSelect select(TEntity::GetTableName(), &entities);
                    auto session = TBase::BuildNativeSession(false);
                    if (!session.ExecRequest(select)) {
                        TFLEventLog::Signal()("&code", "cannot_restore_objects").Error();
                        return false;
                    }
                }
                for (auto&& i : entities) {
                    PrepareObject(i, "preparation");
                }
                TFLEventLog::Signal()("&code", "full_finished");
                return true;
            }

            bool PrepareObject(const TPrepareObject& object, const TString& userId) const {
                auto gLogging = TFLRecords::StartContext()
                    ("&object_id", object.GetInternalId())
                    ("prepared_revision", object.GetPreparedRevision())("revision", object.GetRevision());
                if (object.IsPreparedObject(CurrentAlgorithm)) {
                    TFLEventLog::JustSignal("preparation_manager")("&code", "prepared_already");
                    return true;
                }
                TFLEventLog::Signal()("&code", "object_preparation_start");
                TVector<IAction::TPtr> actions;
                if (!object.PrepareObjectForUsagePersistently(CurrentAlgorithm, actions)) {
                    TFLEventLog::Signal()("&code", "cannot_construct_actors").Error();
                    return false;
                }

                TSet<TString> currentMarkers;
                if (!RestoreMarkers(object, currentMarkers)) {
                    TFLEventLog::Signal()("&code", "cannot_restore_markers").Error();
                    return false;
                }

                TSet<TString> actualMarkers;
                for (auto&& i : actions) {
                    actualMarkers.emplace(i->GetActionId());
                    auto gLogging = TFLRecords::StartContext()("action_id", i->GetActionId());
                    if (!currentMarkers.emplace(i->GetActionId()).second) {
                        TFLEventLog::Signal()("&code", "skip_action");
                        continue;
                    }
                    while (true) {
                        auto lock = NNamedLock::TryAcquireLock(i->GetActionId());
                        auto gLogging = i->BuildLogContext();
                        if (!!lock && i->Execute()) {
                            TFLEventLog::Signal()("&code", "action_prepared").Notice();
                            break;
                        } else {
                            TFLEventLog::Signal()("&code", "action_preparation_failed").Error();
                            Sleep(TDuration::Seconds(1));
                        }
                    }
                    if (!PutMarker(object, i->GetActionId())) {
                        TFLEventLog::Signal()("&code", "cannot_store_marker");
                        return false;
                    }
                }

                if (!SyncMarkers(object, actualMarkers)) {
                    TFLEventLog::Signal()("&code", "cannot_sync_marker");
                    return false;
                }

                auto session = TBase::BuildNativeSession(false);
                if (!UpdatePreparedRevision(object.GetInternalId(), object.GetRevision(), CurrentAlgorithm, userId, session)) {
                    TFLEventLog::Signal()("&code", "revision_update_fail").Error();
                    return false;
                }
                if (!session.Commit()) {
                    TFLEventLog::Signal()("&code", "revision_update_commit_fail").Error();
                    return false;
                }
                TFLEventLog::Signal()("&code", "object_preparation_finished").Notice();
                return true;
            }
        protected:
            virtual bool DoStart() override {
                if (!TBase::DoStart()) {
                    return false;
                }
                TFLEventLog::Signal("preparation_manager")("&code", "manager_start");
                if (!PrepareAll(true)) {
                    return false;
                }
                TFLEventLog::Signal("preparation_manager")("&code", "manager_started");
                ThreadPool.Start(1);
                ThreadPool.SafeAddAndOwn(MakeHolder<TPreparationThread>(this));
                return true;
            }

            virtual bool DoStop() override {
                ThreadPool.Stop();
                if (!TBase::DoStop()) {
                    return false;
                }
                return true;
            }

            TMaybe<TEntity> GetPreparedObject(const TPrepareObject& baseObject) const {
                if (!baseObject.GetPreparedRevision()) {
                    TFLEventLog::Signal("restore_preparation")("&code", "object_not_prepared").Error();
                    return Nothing();
                }
                if (baseObject.GetPreparedRevision() == baseObject.GetRevision()) {
                    return baseObject;
                }
                auto mbResult = TBase::GetRevisionedObject(baseObject.GetInternalId(), baseObject.GetPreparedRevision());
                if (!mbResult) {
                    TFLEventLog::Signal("restore_preparation")("&code", "cannot_fetch_prepared").Alert();
                    return Nothing();
                }
                return *mbResult;
            }

        public:
            using TBase::TBase;

            bool WaitPreparation(const TDuration timeout) const {
                TInstant startInstant = Now();
                bool isFirst = true;
                while (Now() - startInstant < timeout || isFirst) {
                    isFirst = false;
                    NCS::NStorage::TObjectRecordsSet<TPrepareObject> objects;
                    TSRSelect select(TEntity::GetTableName(), &objects);
                    auto session = TBase::BuildNativeSession(false);
                    if (!session.ExecRequest(select)) {
                        TFLEventLog::Signal("restore_preparation")("&code", "cannot_restore_objects_on_wait").Error();
                        continue;
                    }
                    bool allPrepared = true;
                    for (auto&& i : objects) {
                        if (!i.IsPreparedObject()) {
                            allPrepared = false;
                            break;
                        }
                    }
                    if (allPrepared) {
                        return true;
                    }
                }
                return false;
            }

            bool RestorePreparedObjects(TVector<TEntity>& entities, const TSet<typename TEntity::TId>& ids = {}) const {
                NCS::NStorage::TObjectRecordsSet<TPrepareObject> objects;
                {
                    TSRSelect select(TEntity::GetTableName(), &objects);
                    if (ids.size()) {
                        select.InitCondition<TSRBinary>(TEntity::GetIdFieldName(), ids);
                    }
                    auto session = TBase::BuildNativeSession(true);
                    if (!session.ExecRequest(select)) {
                        TFLEventLog::Signal("restore_preparation")("&code", "cannot_restore_objects").Error();
                        return false;
                    }
                }
                TVector<TEntity> resultLocal;
                for (auto&& i : objects) {
                    auto prepared = GetPreparedObject(i);
                    if (!prepared) {
                        continue;
                    }
                    resultLocal.emplace_back(*prepared);
                }
                std::swap(resultLocal, entities);
                return true;
            }

            TMaybe<TEntity> RestorePreparedObject(const typename TEntity::TId& id) const {
                auto gLogging = TFLRecords::StartContext()("&object_table", TEntity::GetTableName());
                NCS::NStorage::TObjectRecordsSet<TPrepareObject> objects;
                {
                    TSRSelect select(TEntity::GetTableName(), &objects);
                    select.InitCondition<TSRBinary>(TEntity::GetIdFieldName(), id);
                    auto session = TBase::BuildNativeSession(true);
                    if (!session.ExecRequest(select)) {
                        TFLEventLog::Signal("restore_preparation")("&code", "cannot_restore_objects").Error();
                        return Nothing();
                    }
                }

                if (objects.empty()) {
                    return Nothing();
                }
                if (objects.size() != 1) {
                    TFLEventLog::Signal("restore_preparation")("&code", "incorrect_object_id").Error();
                    return Nothing();
                }
                return GetPreparedObject(objects.front());
            }

        };
    }
}

template <class TDBObject, class TSnapshotsConstructor = TDBEntitiesSnapshotsConstructor<TDBObject>, ui32 CurrentAlgorithm = 1>
using TEntityPreparationManager = NCS::NPreparation::TManager<TDBObject, TSnapshotsConstructor, CurrentAlgorithm>;
