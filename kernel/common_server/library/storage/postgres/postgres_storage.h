#pragma once

#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/storage/postgres/table_accessor.h>

#include "config.h"
#include "postgres_conn_pool.h"

namespace NCS {
    namespace NStorage {
        class TPostgresLock: public TAbstractLock {
        private:
            using TTrans = pqxx::transaction<pqxx::read_committed, pqxx::read_write>;

            TPostgresConnectionsPool::TActiveConnection Connection;
            TString Key;
            TString LockTable;
            TString AdvisoryLockStatement;
            TDuration Timeout;
            bool WriteLock;
            bool IsLockedFlag;
            THolder<TTrans> Txn;

        public:
            TPostgresLock(TPostgresConnectionsPool::TActiveConnection&& conn, const TString& table, const TString& key, TDuration timeout, bool writeLock);
            virtual ~TPostgresLock() override;

            virtual bool IsLockTaken() const override;
            virtual bool IsLocked() const override;
            bool ReleaseLock();
            bool BuildLock();

        private:
            TString BuildAdvisoryLockStatement() const;
            TString BuildTableLockStatement();
            bool LockStatement(TTrans& txn, const TString& statement) const;
        };
    }
}
