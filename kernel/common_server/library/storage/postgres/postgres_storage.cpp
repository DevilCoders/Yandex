#include "postgres_storage.h"

#include <pqxx/pqxx>

#include <kernel/daemon/common/time_guard.h>

#include <library/cpp/archive/yarchive.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/digest/fnv.h>
#include <util/generic/set.h>
#include <util/memory/blob.h>
#include <util/string/join.h>
#include <util/string/vector.h>
#include <util/string/subst.h>
#include <util/system/hostname.h>

namespace NCS {
    namespace NStorage {
        namespace {
            TString NormalizeKey(const TString& key) {
                return TFsPath("/" + key).Fix().GetPath();
            }

        }

        /* Locker in PostgresQL.
           To capture lock starts a transaction, inserts key into the '<table>_lock' table, starts a transaction
           and make "SELECT FOR UPDATE" on the key; to release removes the key and commits the transaction.
         */
        TPostgresLock::TPostgresLock(TPostgresConnectionsPool::TActiveConnection&& conn, const TString& lockTable, const TString& key, TDuration timeout, bool writeLock)
            : Connection(std::move(conn))
            , Key(NormalizeKey(key))
            , LockTable(lockTable)
            , Timeout(timeout)
            , WriteLock(writeLock)
            , IsLockedFlag(false)
            , Txn(nullptr) {
            if (!Connection) {
                return;
            }
            AdvisoryLockStatement = BuildAdvisoryLockStatement();
            BuildLock();
        }

        TPostgresLock::~TPostgresLock() {
            ReleaseLock();
        }

        bool TPostgresLock::ReleaseLock() {
            if (IsLockedFlag) {
                Txn.Destroy();
                IsLockedFlag = false;
                return true;
            }
            return false;
        }

        bool TPostgresLock::IsLockTaken() const {
            return IsLockedFlag;
        }

        bool TPostgresLock::IsLocked() const {
            return IsLockedFlag && Txn && Txn->conn().is_open();
        }

        bool TPostgresLock::BuildLock() {
            if (IsLockedFlag) {
                return true;
            }

            if (!Connection) {
                return false;
            }

            try {
                Txn = MakeHolder<TTrans>(*Connection);
                Txn->set_variable("lock_timeout", ToString(Timeout.MilliSeconds()));
                IsLockedFlag = LockStatement(*Txn, AdvisoryLockStatement);
            } catch (const pqxx::failure& e) {
                INFO_LOG << "Can't lock key: " << Key << ": " << e.what() << Endl;
            } catch (...) {
                INFO_LOG << "Can't lock key: " << Key << Endl;
            }
            if (!IsLockedFlag) {
                Txn.Destroy();
            }
            return IsLockedFlag;
        }

        TString TPostgresLock::BuildAdvisoryLockStatement() const {
            TStringBuilder result;
            auto uid = FnvHash<ui32>(LockTable) ^ FnvHash<ui32>(Key);
            if (WriteLock) {
                result << "SELECT pg_try_advisory_xact_lock(CAST(" << uid << " as bigint)) as locked";
            } else {
                result << "SELECT pg_try_advisory_xact_lock_shared(CAST(" << uid << " as bigint)) as locked";
            }
            result << ", '" << LockTable << "' as lock_table";
            result << ", '" << Key << "' as key";
            result << ", '" << HostName() << "' as hostname";
            return result;
        }

        TString TPostgresLock::BuildTableLockStatement() {
            TString readyKey(Connection->quote(Key.data()));
            if (WriteLock) {
                return TStringBuilder() << "SELECT key FROM " << LockTable << " WHERE key=" << readyKey << " FOR UPDATE SKIP LOCKED";
            } else {
                return TStringBuilder() << "SELECT key FROM " << LockTable << " WHERE key=" << readyKey << " FOR KEY SHARE SKIP LOCKED";
            }
        }

        bool TPostgresLock::LockStatement(TTrans& txn, const TString& statement) const {
            if (!statement) {
                return true;
            }
            pqxx::result result = txn.exec(statement);
            if (result.size() > 0) {
                return result.at(0)["locked"].as<bool>();
            } else {
                return false;
            }
        }
    }
}
