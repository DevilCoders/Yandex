#pragma once

#include "config.h"
#include "postgres_conn_pool.h"

#include <kernel/common_server/library/storage/sql/structured.h>
#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/library/storage/balancing/abstract.h>

namespace NCS {
    namespace NStorage {
        class TPostgresBalancer {
        public:
            class TWeightedHost: public NBalancing::TBalancingObject {
            private:
                CSA_DEFAULT(TWeightedHost, TAtomicSharedPtr<TPostgresConnectionsPool>, Pool);
            public:
            };

        private:
            const TPostgresStorageOptionsImpl Config;
            TBalancingPolicyOperator::TPtr BalancingPolicy;
            mutable TMap<TString, TWeightedHost> WeightedHosts;
            mutable TAtomicSharedPtr<TPostgresConnectionsPool> RWPool;
            mutable TVector<TAtomicSharedPtr<TPostgresConnectionsPool>> RPools;
            TString DBName;
            TRWMutex RWMutex;
            TThreadPool ThreadPool;
            TAtomic IsActive = 1;
        private:

            class TRefreshThread: public IObjectInQueue {
            private:
                TPostgresBalancer* Owner;
            public:
                TRefreshThread(TPostgresBalancer* owner)
                    : Owner(owner) {

                }

                virtual void Process(void* /*threadSpecificResource*/) override {
                    while (AtomicGet(Owner->IsActive) == 1) {
                        Owner->RebuildReadOnlyPools();
                        Sleep(TDuration::Seconds(1));
                    }
                }
            };

            TString GetHostsString(const TString& hostName) const;
            void RebuildReadOnlyPools();
        public:

            TPostgresBalancer(const TPostgresStorageOptionsImpl& config, TBalancingPolicyOperator::TPtr bPolicy, const TString& dbName)
                : Config(config)
                , BalancingPolicy(bPolicy)
                , RWPool(Config.ConstructRWPool())
                , DBName(dbName) {
                if (!BalancingPolicy) {
                    BalancingPolicy = MakeAtomicShared<TBalancingPolicyOperator>();
                }
                RebuildReadOnlyPools();
                ThreadPool.Start(1);
                ThreadPool.SafeAddAndOwn(MakeHolder<TRefreshThread>(this));

            }

            ~TPostgresBalancer() {
                AtomicSet(IsActive, 0);
                ThreadPool.Stop();
            }

            TPostgresConnectionsPool::TActiveConnection GetWritableConnection() {
                return RWPool->GetConnection();
            }

            TPostgresConnectionsPool::TActiveConnection GetReadOnlyConnection();
        };

        class TPostgresDB: public NSQL::TDatabase {
        private:
            using TBase = NSQL::TDatabase;
        protected:
            virtual ITransaction::TPtr DoCreateTransaction(const TTransactionFeatures& features) const override;
        public:
            TPostgresDB(const TPostgresStorageOptionsImpl& config, const TString& dbServerId, const TString& dbName, TBalancingPolicyOperator::TPtr bPolicy = nullptr);

            virtual TVector<TString> GetAllTableNames() const override;
            virtual TAbstractLock::TPtr Lock(const TString& lockName, const bool writeLock, const TDuration timeout, const TString& namespaces = "public") const override;

        private:
            const TPostgresStorageOptionsImpl Config;
            const TString ConnectionString;
            const TString LocksTableName;
            mutable THolder<NSQL::TAccessLogger> Logger;
            TString DBName = "db_postgres";
            THolder<TPostgresBalancer> Balancer;
        };

        class TPostgresConfig: public IDatabaseConfig, public TPostgresStorageOptionsImpl {
        public:
            virtual IDatabase::TPtr ConstructDatabase(const IDatabaseConstructionContext* context) const override;
        protected:
            virtual void DoInit(const TYandexConfig::Section* section) override;
            virtual void DoToString(IOutputStream& os) const override;

        private:
            static TFactory::TRegistrator<TPostgresConfig> Registrator;
        };
    }
}
