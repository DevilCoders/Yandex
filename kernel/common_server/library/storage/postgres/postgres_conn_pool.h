#pragma once

#include "config.h"

#include <pqxx/pqxx>

#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/logging/tskv_log.h>
#include <kernel/common_server/util/objects_pool.h>

#include <util/generic/ptr.h>
#include <util/thread/lfstack.h>
#include <util/system/mutex.h>
#include <util/system/condvar.h>
#include <util/system/guard.h>
#include <util/thread/pool.h>
#include <util/datetime/base.h>

namespace NCS {
    namespace NStorage {
        class TPostgresConnectionsPool: public IConstrainedObjectsPool<pqxx::connection> {
        private:
            using TBase = IConstrainedObjectsPool<pqxx::connection>;

            virtual pqxx::connection* AllocateObject() override;

            virtual bool ActivateObject(pqxx::connection& /*connection*/) override {
                TFLEventLog::JustSignal("pg_database")("&code", "activate_connection");
                return true;
            }

            virtual void DeactivateObject(pqxx::connection& /*obj*/) override {
                TFLEventLog::JustSignal("pg_database")("&code", "connect_deactivation");
            }

        public:

            using TActiveConnection = TBase::TActiveObject;
            ~TPostgresConnectionsPool() = default;
            TActiveConnection GetConnection();
            virtual void OnEvent() override;
            TPostgresConnectionsPool(const TString& connectionString);

        private:
            TString ConnectionString;
        };

        class TGlobalPostgresConnectionsPool {
        private:
            TMutex Mutex;
            TMap<TString, TAtomicSharedPtr<TPostgresConnectionsPool>> Pools;
        public:
            TGlobalPostgresConnectionsPool() {

            }

            TAtomicSharedPtr<TPostgresConnectionsPool> Register(const TString& connectionString);
        };
    }

}
