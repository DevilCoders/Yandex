#include "manager.h"
#include <kernel/common_server/util/instant_model.h>
#include "object.h"

namespace NCS {
    class TTaskManagerInfo {
        CSA_READONLY(TString, OwnerId, "undefined");
        CSA_READONLY(TString, QueueId, "undefined");
        CSA_READONLY(TString, ClassName, "undefined");
        CSA_READONLY(ui64, Count, 0);
    public:
        class TDecoder: public TBaseDecoder {
        private:
            DECODER_FIELD(ClassName);
            DECODER_FIELD(OwnerId);
            DECODER_FIELD(QueueId);
            DECODER_FIELD(Count);
        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase) {
                ClassName = GetFieldDecodeIndex("class_name", decoderBase);
                OwnerId = GetFieldDecodeIndex("owner_id", decoderBase);
                QueueId = GetFieldDecodeIndex("queue_id", decoderBase);
                Count = GetFieldDecodeIndex("count", decoderBase);
            }
        };
        Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE_OPT(decoder, values, ClassName);
            READ_DECODER_VALUE_OPT(decoder, values, OwnerId);
            READ_DECODER_VALUE_OPT(decoder, values, QueueId);
            READ_DECODER_VALUE_OPT(decoder, values, Count);
            return true;
        }
    };

    namespace NBackground {
        TDBTasksManagerConfig::TFactory::TRegistrator<TDBTasksManagerConfig> TDBTasksManagerConfig::Registrator(TDBTasksManagerConfig::GetTypeName());

        TAtomicSharedPtr<NCS::NBackground::ITasksManager> TDBTasksManagerConfig::Construct(const IBaseServer& server) const {
            auto db = server.GetDatabase(DBName);
            AssertCorrectConfig(!!db, "incorrect db name %s", DBName.data());
            return MakeAtomicShared<TDBTasksManager>(GetTasksManagerId(), server, *this, db);
        }

        bool TDBTasksManager::Refresh() {
            auto tr = Database->CreateTransaction(true);
            NStorage::TObjectRecordsSet<TTaskManagerInfo> objects;
            TStringStream condition;
            condition << "SELECT queue_id, owner_id, class_name, count(*) AS count"
                    << " FROM " << Config.GetTableName() << " GROUP BY (queue_id, owner_id, class_name)";
            if (!tr->Exec(condition.Str(), &objects)->IsSucceed()) {
                return false;
            }

            for (auto&& i : SignalsInfo) {
                i.second = 0;
            }
            for (auto&& i : objects) {
                TSignalTagsSet tags;
                tags("tasks_manager_id", TasksManagerId)("owner_id", i.GetOwnerId())("queue_id", i.GetQueueId())("class_name", i.GetClassName());
                SignalsInfo[tags] = i.GetCount();
            }
            for (auto&& i : SignalsInfo) {
                TCSSignals::LTSignal("db_tasks_manager_info", i.second)(i.first);
            }

            if (UpdateTasksLock()) {
                TFLEventLog::Info("tasks lock updated").Signal("update_tasks_lock")("&code", "success")("&tasks_manager_id", TasksManagerId);
            } else {
                TFLEventLog::Error("cannot update tasks lock").Signal("update_tasks_lock")("&code", "failed")("&tasks_manager_id", TasksManagerId);
            }
            return true;
        }

        bool TDBTasksManager::DoAddTasks(const TVector<TRTQueueTaskContainer>& tasks) const {
            if (tasks.empty()) {
                return true;
            }
            NCS::NStorage::TRecordsSet records;
            for (auto&& i : tasks) {
                if (!i) {
                    TFLEventLog::Error("incorrect task (empty)");
                    return false;
                }
                TDBBackgroundTask dbTask(*i);
                dbTask.SetConstructionInstant(ModelingNow());
                records.AddRow(dbTask.SerializeToTableRecord());
            }
            auto dbTable = Database->GetTable(Config.GetTableName());
            auto tr = Database->CreateTransaction(false);
            if (!dbTable->AddRows(records, tr, "", nullptr)->IsSucceed() || !tr->Commit()) {
                TFLEventLog::Error(tr->GetStringReport());
                return false;
            }
            return true;
        }

        bool TDBTasksManager::DoRestoreOwnerTasks(const TString& queueId, const TString& ownerId, TVector<TRTQueueTaskContainer>& result, const bool useReadReplica) const {
            NStorage::TObjectRecordsSet<TDBBackgroundTask> records;
            {
                auto tr = Database->CreateTransaction(useReadReplica);
                TSRSelect select(Config.GetTableName(), &records);
                auto& mConf = select.RetCondition<TSRMulti>();
                mConf.RetNode<TSRBinary>("owner_id", ownerId);
                if (!!queueId && queueId != "*") {
                    mConf.RetNode<TSRBinary>("queue_id", queueId);
                }
                if (!tr->ExecRequest(select)->IsSucceed()) {
                    TFLEventLog::Error(tr->GetStringReport());
                    return false;
                }
            }
            TVector<TRTQueueTaskContainer> resultLocal;
            for (auto&& i : records.DetachObjects()) {
                resultLocal.emplace_back(TRTQueueTaskContainer(new TDBBackgroundTask(std::move(i))));
            }
            std::swap(resultLocal, result);
            return true;
        }

        void TDBTasksManager::RemoveTaskWatching(const ui64 taskId) const {
            TWriteGuard wg(Mutex);
            WaitingTaskIds.erase(taskId);
        }

        void TDBTasksManager::AddTaskWatching(const ui64 taskId) const {
            TWriteGuard wg(Mutex);
            WaitingTaskIds.emplace(taskId, ModelingNow());
        }

        TMap<ui64, TInstant> TDBTasksManager::GetTaskWatchingIds() const {
            TReadGuard wg(Mutex);
            return WaitingTaskIds;
        }

        void TDBTasksManager::UpdateWatchingTasks(const TSet<ui64>& taskIds, const TInstant actualInstant) const {
            TWriteGuard wg(Mutex);
            for (auto&& i : taskIds) {
                WaitingTaskIds[i] = actualInstant;
            }
        }

        bool TDBTasksManager::DoRemoveTask(const TRTQueueTaskContainer& task) const {
            auto* item = task.GetAs<TDBBackgroundTask>();
            CHECK_WITH_LOG(item) << "incorrect task class for remove from db tasks manager " << task.GetInternalTaskId() << "/" << task.GetOwnerId() << Endl;

            TReadGuard rg(UpdateTasksMutex);
            RemoveTaskWatching(item->GetSequenceTaskId());
            TSRDelete srDelete(Config.GetTableName());
            srDelete.InitCondition<TSRBinary>("sequence_task_id", item->GetSequenceTaskId());
            auto tr = Database->CreateTransaction(false);
            if (!tr->ExecRequest(srDelete)->IsSucceed() || !tr->Commit()) {
                return false;
            }
            return true;
        }

        bool TDBTasksManager::DoDropTaskExecution(const TRTQueueTaskContainer& task) const {
            auto* item = task.GetAs<TDBBackgroundTask>();
            CHECK_WITH_LOG(item) << "incorrect task class for drop from db tasks manager " << task.GetInternalTaskId() << "/" << task.GetOwnerId() << Endl;

            TReadGuard rg(UpdateTasksMutex);
            RemoveTaskWatching(item->GetSequenceTaskId());
            TSRUpdate srUpdate(Config.GetTableName());
            srUpdate.InitCondition<TSRBinary>("sequence_task_id", item->GetSequenceTaskId());
            auto& srMulti = srUpdate.RetUpdate<TSRMulti>(ESRMulti::ObjectsSet);
            srMulti.InitNode<TSRBinary>("last_ping_instant", 0);
            srMulti.InitNode<TSRNullInit>("current_host");

            auto tr = Database->CreateTransaction(false);
            if (!tr->ExecRequest(srUpdate)->IsSucceed() || !tr->Commit()) {
                return false;
            }
            return true;
        }

        TDuration TDBTasksManager::GetPingTimeout(const TString& queueId) const {
            return Server.GetSettings().GetValueDef({ "defaults.backgrounds.timeout_queue." + queueId, "defaults.backgrounds.timeout_queue.*" }, "", TDuration::Seconds(120));
        }

        TDuration TDBTasksManager::GetPingInterval() const {
            return Server.GetSettings().GetValueDef("defaults.backgrounds.ping_interval", TDuration::Seconds(10));
        }

        TString TDBTasksManager::GetAgentToken() const {
            return HostName() + ":" + UniqueToken;
        }

        bool TDBTasksManager::UpdateTasksLock() const {
            TWriteGuard wg(UpdateTasksMutex);
            const TMap<ui64, TInstant> allIds = GetTaskWatchingIds();
            TSet<ui64> ids;
            const TInstant now = ModelingNow();
            const TDuration qTimeout = GetPingInterval();
            for (auto&& i : allIds) {
                if (ModelingNow() > i.second + qTimeout) {
                    ids.emplace(i.first);
                }
            }
            if (ids.empty()) {
                return true;
            }
            TSRUpdate srUpdate(Config.GetTableName());
            auto& srMulti = srUpdate.RetCondition<TSRMulti>();
            srMulti.InitNode<TSRBinary>("current_host", GetAgentToken());
            srMulti.InitNode<TSRBinary>("sequence_task_id", ids);
            srUpdate.InitUpdate<TSRBinary>("last_ping_instant", now.Seconds());

            NCS::TEntitySession session(Database->CreateTransaction(false));
            if (!session.ExecRequest(srUpdate) || !session.Commit()) {
                return false;
            }
            UpdateWatchingTasks(ids, now);
            return true;
        }

        bool TDBTasksManager::DoGetTasksForExecution(const TString& queueId, const ui32 tasksLimit, TVector<TRTQueueTaskContainer>& tasks) const {
            NStorage::TObjectRecordsSet<TDBBackgroundTask> records;
            {
                const i32 iLockDuration = Server.GetSettings().GetValueDef<i32>("defaults.tasks_manager_lock_timeout", 10);
                NStorage::TAbstractLock::TPtr lockPtr;
                if (iLockDuration >= 0) {
                    lockPtr = Database->Lock(queueId, true, TDuration::Seconds(iLockDuration));
                    if (!lockPtr) {
                        TFLEventLog::Info("cannot lock for queue")("queue_id", queueId);
                        return false;
                    }
                }
                const TDuration qTimeout = GetPingTimeout(queueId);

                const TInstant now = ModelingNow();

                TSRSelect reqSelect(Config.GetTableName());
                {
                    auto& srMulti = reqSelect.RetCondition<TSRMulti>();
                    if (!!queueId && queueId != "*") {
                        srMulti.InitNode<TSRBinary>("queue_id", queueId);
                    }
                    srMulti.InitNode<TSRBinary>("last_ping_instant", (now - qTimeout).Seconds(), ESRBinary::Less);
                    srMulti.InitNode<TSRBinary>("start_instant", now.Seconds(), ESRBinary::LessOrEqual);
                    reqSelect.InitOrderBy<TSRFieldName>("sequence_task_id").SetCountLimit(tasksLimit);
                    reqSelect.InitFields<TSRFieldName>("sequence_task_id");
                }
                TSRUpdate reqUpdate(Config.GetTableName(), &records);
                {
                    reqUpdate.RetCondition<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("sequence_task_id").SetRight(reqSelect);
                    auto& srMulti = reqUpdate.RetUpdate<TSRMulti>(ESRMulti::ObjectsSet);
                    srMulti.InitNode<TSRBinary>("last_ping_instant", now.Seconds());
                    srMulti.InitNode<TSRBinary>("current_host", GetAgentToken());
                }
                NCS::TEntitySession session(Database->TransactionMaker().NotReadOnly().RepeatableRead(iLockDuration < 0).ExpectFail().Build());
                if (!session.ExecRequest(reqUpdate) || !session.Commit()) {
                    return false;
                }
            }
            TVector<TRTQueueTaskContainer> resultLocal;
            for (auto&& i : records.DetachObjects()) {
                AddTaskWatching(i.GetSequenceTaskId());
                resultLocal.emplace_back(TRTQueueTaskContainer(new TDBBackgroundTask(std::move(i))));
            }
            std::swap(resultLocal, tasks);
            return true;
        }

    }
}
