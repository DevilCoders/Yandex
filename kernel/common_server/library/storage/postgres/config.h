#pragma once

#include <kernel/common_server/library/storage/config.h>
#include <kernel/common_server/util/accessor.h>

namespace NCS {
    namespace NStorage {
        class TPostgresConnectionsPool;

        class TPostgresStorageOptionsImpl {
        public:
            enum ELogging {
                None,
                Medium,
                High
            };

        private:
            CSA_READONLY(TDuration, HardLimitWaitDuration, TDuration::Minutes(1));
            CS_ACCESS(TPostgresStorageOptionsImpl, ui32, HardLimitPoolSize, 350);
            CSA_READONLY(TDuration, AgeForKillConnection, TDuration::Minutes(5));
            CSA_READONLY(TDuration, LockTimeout, TDuration::Minutes(1));
            CSA_READONLY(TDuration, StatementTimeout, TDuration::Minutes(2));
            CSA_READONLY(TDuration, ConnectionTimeout, TDuration::Seconds(15));
            CSA_READONLY(TDuration, TCPUserTimeout, TDuration::Seconds(15));
            CSA_DEFAULT(TPostgresStorageOptionsImpl, TString, LocksTableName);
            CSA_DEFAULT(TPostgresStorageOptionsImpl, TString, SimpleConnectionString);
            CSA_READONLY_DEF(TString, PasswordFileName);
            CSA_READONLY_DEF(TString, SSLCert);
            CSA_READONLY(bool, VersioningEnable, false);
            CSA_READONLY_DEF(TString, VersioningTableName);
            CSA_READONLY(ui32, LogLevel, ELogging::None);
            CSA_READONLY(TDuration, ActivateTimeout, TDuration::Max());
            CSA_READONLY(TDuration, ActivationSleep, TDuration::MilliSeconds(100));
            CSA_READONLY(bool, PanicOnDisconnect, true);
            CSA_READONLY(TDuration, IncorrectLagMin, TDuration::Seconds(15));
            CS_ACCESS(TPostgresStorageOptionsImpl, bool, UseBalancing, true);
            CSA_READONLY(TDuration, KeepaliveIdle, TDuration::Seconds(10));
            CSA_READONLY(TDuration, KeepalivesInterval, TDuration::Seconds(5));
            CSA_READONLY(ui32, KeepalivesCount, 3);
            CSA_DEFAULT(TPostgresStorageOptionsImpl, TString, MainNamespace);
        public:
            TPostgresStorageOptionsImpl() = default;

            TVector<TString> GetHosts() const;
            void SwitchHosts(const TString& hosts);

            TString GetFullConnectionString(const bool readOnly, const bool forPublishing = false) const;

            bool Init(const TYandexConfig::Section* section);
            void ToString(IOutputStream& os) const;

            TAtomicSharedPtr<TPostgresConnectionsPool> ConstructRPool() const;
            TAtomicSharedPtr<TPostgresConnectionsPool> ConstructRWPool() const;
        };

    }
}
