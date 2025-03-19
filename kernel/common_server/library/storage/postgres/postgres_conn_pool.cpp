#include "postgres_conn_pool.h"

#include <library/cpp/digest/md5/md5.h>

namespace NCS {
    namespace NStorage {
        pqxx::connection* TPostgresConnectionsPool::AllocateObject() {
            auto gLogging = TFLRecords::StartContext().Method("TPostgresConnectionsPool::AllocateConnection");
            try {
                TCSSignals::Signal("pg_database")("code", "connect_allocation")("db_name", GetPoolName());
                auto result = MakeHolder<pqxx::connection>(ConnectionString);
                TCSSignals::Signal("pg_database")("code", "connect_allocated")("db_name", GetPoolName());
                return result.Release();
            } catch (const pqxx::failure& e) {
                TFLEventLog::Error("Can't create connection")("exception", e.what());
            } catch (...) {
                TFLEventLog::Error("Can't create connection")("exception", CurrentExceptionMessage());
            }
            TCSSignals::Signal("pg_database")("code", "connect_allocation_fail")("db_name", GetPoolName());
            return nullptr;
        }

        TPostgresConnectionsPool::TActiveConnection TPostgresConnectionsPool::GetConnection() {
            TCSSignals::Signal("pg_database")("code", "connect_accepting")("db_name", GetPoolName());
            auto result = TBase::GetObject();
            TCSSignals::Signal("pg_database")("code", "connect_accepted")("db_name", GetPoolName());
            return result;
        }

        void TPostgresConnectionsPool::OnEvent() {
            TCSSignals::LSignal("pg_database", AtomicGet(ActiveObjects))("code", "active_objects")("db_name", GetPoolName());
            TCSSignals::LSignal("pg_database", AtomicGet(PassiveObjects))("code", "passive_objects")("db_name", GetPoolName());
        }

        TPostgresConnectionsPool::TPostgresConnectionsPool(const TString& connectionString)
            : ConnectionString(connectionString) {
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(connectionString, params);
            const TString dbName = params.contains("dbname") ? params["dbname"] : "undef_dbname";
            SetPoolName(dbName);
        }

        TAtomicSharedPtr<TPostgresConnectionsPool> TGlobalPostgresConnectionsPool::Register(const TString& connectionString) {
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(connectionString, params);
            TString paramsStrStable;
            for (auto&& i : params) {
                paramsStrStable += i.first + " : " + i.second + ";";
            }
            const TString key = MD5::Calc(paramsStrStable);
            TGuard<TMutex> g(Mutex);
            auto it = Pools.find(key);
            if (it == Pools.end()) {
                auto pool = MakeAtomicShared<TPostgresConnectionsPool>(connectionString);
                pool->Start();
                return Pools.emplace(key, pool).first->second;
            } else {
                return it->second;
            }
        }
    }
}
