#pragma once
#include <kernel/common_server/library/persistent_queue/abstract/pq.h>
#include <kernel/common_server/settings/abstract/abstract.h>

namespace NCS {
    namespace NPQ {
        namespace NDatabase {
            class TQueue: public IPQRandomAccess {
            private:
                using TBase = IPQRandomAccess;
                NStorage::IDatabase::TPtr Database;
                const IReadSettings& ReadSettings;
                const TString CurrentQueueAgentId = TGUID::CreateTimebased().AsUuidString();
                mutable TRWMutex Mutex;
                mutable TRWMutex UpdateTasksMutex;
                mutable TMap<TString, TInstant> WaitingTaskIds;
                TDuration GetPingTimeout(const TString& queueId) const;
                TDuration GetPingInterval() const;
                bool UpdateTasksLock() const;
                void RemoveTaskWatching(const TString& messageId) const;
                void AddTaskWatching(const TString& messageId) const;
                TMap<TString, TInstant> GetTaskWatchingIds() const;
                void UpdateWatchingTasks(const TSet<TString>& messageIds, const TInstant actualInstant) const;
                TString GetTableName() const;
                class TDBPQMessageSimple;
                TPQResult DropMessage(const TDBPQMessageSimple& message) const;
            protected:
                virtual bool DoStartImpl() override;
                virtual bool DoStopImpl() override {
                    return true;
                }
                virtual bool DoRestoreMessage(const TString& messageId, IPQMessage::TPtr& result) const override;
                virtual bool DoReadMessages(TVector<IPQMessage::TPtr>& result, ui32* failedMessages, const size_t maxSize, const TDuration timeout) const override;
                virtual TPQResult DoWriteMessage(const IPQMessage& result) const override;
                virtual TPQResult DoAckMessage(const IPQMessage& message) const override;
                virtual bool DoFlushWritten() const override {
                    return true;
                }
                virtual bool IsReadable() const override {
                    return true;
                }
                virtual bool IsWritable() const override {
                    return true;
                }

                virtual bool Refresh() override;
            public:
                TQueue(const TString& clientId, NCS::NStorage::IDatabase::TPtr db, const IReadSettings& readSettings)
                    : TBase(clientId)
                    , Database(db)
                    , ReadSettings(readSettings)
                {
                    CHECK_WITH_LOG(Database);
                }

            };
        }
    }
}
