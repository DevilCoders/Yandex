#include "queue.h"
#include "object.h"
#include <kernel/common_server/util/instant_model.h>
#include <kernel/common_server/api/common.h>

namespace NCS {
    namespace NPQ {
        namespace NDatabase {
            class TQueue::TDBPQMessageSimple: public TPQMessageSimple {
            private:
                using TBase = TPQMessageSimple;
                mutable const TQueue* Queue = nullptr;
            public:
                TDBPQMessageSimple(TBlob&& content, const TString& messageId, const TQueue* queue)
                    : TBase(std::move(content), messageId)
                    , Queue(queue)
                {

                }

                void Release() const {
                    Queue = nullptr;
                }

                ~TDBPQMessageSimple() {
                    if (Queue) {
                        Queue->DropMessage(*this);
                    }
                }
            };

            bool TQueue::Refresh() {
                NStorage::TObjectRecordsSet<TObject> objects;
                {
                    auto tr = Database->CreateTransaction(true);
                    TSRSelect select(GetTableName(), &objects);
                    if (!tr->ExecRequest(select)->IsSucceed()) {
                        return false;
                    }
                }
                auto gLogging = TFLRecords::StartContext()("&pq_client_id", GetClientId());
                TFLEventLog::LTSignal("pq_size", objects.size());

                if (UpdateTasksLock()) {
                    TFLEventLog::Info("tasks lock updated").Signal("update_tasks_lock")("&code", "success");
                } else {
                    TFLEventLog::Error("cannot update tasks lock").Signal("update_tasks_lock")("&code", "failed");
                }
                return true;
            }

            IPQClient::TPQResult TQueue::DoWriteMessage(const IPQMessage& message) const {
                TObject dbMessage(message);
                dbMessage.SetConstructionInstant(ModelingNow());

                auto dbTable = Database->GetTable(GetTableName());
                auto tr = Database->CreateTransaction(false);
                const auto result = dbTable->AddRow(dbMessage.SerializeToTableRecord(), tr, "", nullptr)->IsSucceed() && tr->Commit();
                return TPQResult(result).SetId(message.GetMessageId());
            }

            void TQueue::RemoveTaskWatching(const TString& messageId) const {
                TWriteGuard wg(Mutex);
                WaitingTaskIds.erase(messageId);
            }

            void TQueue::AddTaskWatching(const TString& messageId) const {
                TWriteGuard wg(Mutex);
                WaitingTaskIds.emplace(messageId, ModelingNow());
            }

            TMap<TString, TInstant> TQueue::GetTaskWatchingIds() const {
                TReadGuard wg(Mutex);
                return WaitingTaskIds;
            }

            void TQueue::UpdateWatchingTasks(const TSet<TString>& messageIds, const TInstant actualInstant) const {
                TWriteGuard wg(Mutex);
                for (auto&& i : messageIds) {
                    WaitingTaskIds[i] = actualInstant;
                }
            }

            TString TQueue::GetTableName() const {
                return "cs_pq_" + GetClientId();
            }

            IPQClient::TPQResult TQueue::DoAckMessage(const IPQMessage& message) const {
                auto* pqMessage = dynamic_cast<const TDBPQMessageSimple*>(&message);
                if (!pqMessage) {
                    TFLEventLog::Alert("cannot ack messag with incorrect class");
                } else {
                    pqMessage->Release();
                }
                TReadGuard rg(UpdateTasksMutex);
                RemoveTaskWatching(message.GetMessageId());
                TSRDelete srDelete(GetTableName());
                srDelete.InitCondition<TSRBinary>("message_id", message.GetMessageId());
                auto tr = Database->CreateTransaction(false);
                if (tr->ExecRequest(srDelete)->IsSucceed() && tr->Commit()) {
                    return TPQResult(true).SetId(message.GetMessageId());
                } else {
                    return TPQResult(false).SetId(message.GetMessageId());
                }
            }

            IPQClient::TPQResult TQueue::DropMessage(const TDBPQMessageSimple& message) const {
                TReadGuard rg(UpdateTasksMutex);
                RemoveTaskWatching(message.GetMessageId());
                TSRUpdate srUpdate(GetTableName());
                srUpdate.InitCondition<TSRBinary>("message_id", message.GetMessageId());
                auto& srMulti = srUpdate.RetUpdate<TSRMulti>(ESRMulti::ObjectsSet);
                srMulti.InitNode<TSRBinary>("last_ping_instant", 0);
                srMulti.InitNode<TSRNullInit>("current_queue_agent_id");
                
                auto tr = Database->CreateTransaction(false);
                if (tr->ExecRequest(srUpdate)->IsSucceed() && tr->Commit()) {
                    return TPQResult(true).SetId(message.GetMessageId());
                } else {
                    return TPQResult(false).SetId(message.GetMessageId());
                }
            }

            TDuration TQueue::GetPingTimeout(const TString& queueId) const {
                return ReadSettings.GetValueDef({ "defaults.cs_pq.timeout_queue." + queueId, "defaults.cs_pq.timeout_queue.*" }, "", TDuration::Seconds(120));
            }

            TDuration TQueue::GetPingInterval() const {
                return ReadSettings.GetValueDef("defaults.cs_pq.ping_interval", TDuration::Seconds(10));
            }

            bool TQueue::UpdateTasksLock() const {
                TWriteGuard wg(UpdateTasksMutex);
                const TMap<TString, TInstant> allIds = GetTaskWatchingIds();
                TSet<TString> ids;
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
                TSRUpdate srUpdate(GetTableName());
                auto& srMulti = srUpdate.RetCondition<TSRMulti>();
                srMulti.InitNode<TSRBinary>("current_queue_agent_id", CurrentQueueAgentId);
                srMulti.InitNode<TSRBinary>("message_id", ids);
                srUpdate.InitUpdate<TSRBinary>("last_ping_instant", now.Seconds());

                TEntitySession session(Database->CreateTransaction(false));
                if (!session.ExecRequest(srUpdate) || !session.Commit()) {
                    return false;
                }
                UpdateWatchingTasks(ids, now);
                return true;
            }

            bool TQueue::DoStartImpl() {
                {
                    NCS::NStorage::TCreateTableQuery ctq(GetTableName());
                    ctq.AddColumn("message_id").SetType(NCS::NStorage::EColumnType::Text).Primary();
                    ctq.AddColumn("content").SetType(NCS::NStorage::EColumnType::Binary);
                    ctq.AddColumn("construction_instant").SetType(NCS::NStorage::EColumnType::I32);
                    ctq.AddColumn("last_ping_instant").SetType(NCS::NStorage::EColumnType::I32);
                    ctq.AddColumn("current_queue_agent_id").SetType(NCS::NStorage::EColumnType::Text);
                    CHECK_WITH_LOG(Database->CreateTable(ctq)) << GetClientId() << Endl;
                }
                {
                    NCS::NStorage::TCreateIndexQuery ciq(GetTableName());
                    ciq.AddColumn("construction_instant");
                    CHECK_WITH_LOG(Database->CreateIndex(ciq)) << GetClientId() << Endl;
                }
                return true;
            }

            bool TQueue::DoRestoreMessage(const TString& messageId, IPQMessage::TPtr& result) const {
                NStorage::TObjectRecordsSet<TObject> records;
                {
                    TSRSelect reqSelect(GetTableName(), &records);
                    reqSelect.InitCondition<TSRBinary>("message_id", messageId);
                    NCS::TEntitySession session(Database->TransactionMaker().ReadOnly().Build());
                    if (!session.ExecRequest(reqSelect)) {
                        return false;
                    }
                }
                if (records.size() > 1) {
                    TFLEventLog::Alert("incorrect messages count")("count", records.size());
                    return false;
                }
                if (records.size() == 1) {
                    result = MakeAtomicShared<TPQMessageSimple>(std::move(records.front().DetachMutableContent()), records.front().GetMessageId());
                } else {
                    result = nullptr;
                }
                return true;
            }

            bool TQueue::DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const {
                NStorage::TObjectRecordsSet<TObject> records;
                {
                    const i32 iLockDuration = ReadSettings.GetValueDef<i32>("defaults.cs_pq.lock_timeout", timeout.Seconds());
                    NStorage::TAbstractLock::TPtr lockPtr;
                    if (iLockDuration >= 0) {
                        lockPtr = Database->Lock(GetTableName(), true, TDuration::Seconds(iLockDuration));
                        if (!lockPtr) {
                            TFLEventLog::Info("cannot lock for queue");
                            return false;
                        }
                    }
                    const TDuration qTimeout = GetPingTimeout(GetClientId());
                    const TInstant now = ModelingNow();
                    TSRSelect reqSelect(GetTableName());
                    {
                        auto& srMulti = reqSelect.RetCondition<TSRMulti>();
                        srMulti.InitNode<TSRBinary>("last_ping_instant", (now - qTimeout).Seconds(), ESRBinary::Less);
                        reqSelect.InitOrderBy<TSRFieldName>("construction_instant").SetCountLimit(maxSize);
                        reqSelect.InitFields<TSRFieldName>("message_id");
                    }
                    TSRUpdate reqUpdate(GetTableName(), &records);
                    {
                        reqUpdate.RetCondition<TSRBinary>(ESRBinary::In).InitLeft<TSRFieldName>("message_id").SetRight(reqSelect);
                        auto& srMulti = reqUpdate.RetUpdate<TSRMulti>(ESRMulti::ObjectsSet);
                        srMulti.InitNode<TSRBinary>("last_ping_instant", now.Seconds());
                        srMulti.InitNode<TSRBinary>("current_queue_agent_id", CurrentQueueAgentId);
                    }
                    NCS::TEntitySession session(Database->TransactionMaker().NotReadOnly().RepeatableRead(iLockDuration < 0).ExpectFail().Build());
                    if (!session.ExecRequest(reqUpdate) || !session.Commit()) {
                        return false;
                    }
                }
                TVector<IPQMessage::TPtr> resultLocal;
                for (auto&& i : records.DetachObjects()) {
                    resultLocal.emplace_back(new TDBPQMessageSimple(std::move(i.DetachMutableContent()), i.GetMessageId(), this));
                }
                if (failedMessages) {
                    *failedMessages = 0;
                }
                std::swap(resultLocal, result);
                return true;
            }

        }
    }
}
