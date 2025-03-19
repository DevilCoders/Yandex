#include "config.h"
#include <util/stream/file.h>
#include <kernel/common_server/util/logging/tskv_log.h>
#include <library/cpp/digest/md5/md5.h>
#include "postgres_conn_pool.h"

namespace NCS {
    namespace NStorage {

        bool TPostgresStorageOptionsImpl::Init(const TYandexConfig::Section* section) {
            CHECK_WITH_LOG(!!section);
            const TYandexConfig::Directives& directives = section->GetDirectives();
            directives.GetValue("ConnectionString", SimpleConnectionString);
            AssertCorrectConfig(!!SimpleConnectionString, "no 'ConnectionString' field");
            directives.GetValue("LocksTableName", LocksTableName);
            if (!LocksTableName) {
                LocksTableName = section->GetDirectives().Value<TString>("TableName") + "_lock";
            }
            MainNamespace = directives.Value("MainNamespace", MainNamespace);
            directives.GetValue("LockTimeout", LockTimeout);
            directives.GetValue("StatementTimeout", StatementTimeout);
            ConnectionTimeout = section->GetDirectives().Value("ConnectionTimeout", ConnectionTimeout);
            TCPUserTimeout = section->GetDirectives().Value("TCPUserTimeout", TCPUserTimeout);
            AgeForKillConnection = section->GetDirectives().Value("AgeForKillConnection", AgeForKillConnection);
            PasswordFileName = section->GetDirectives().Value("PasswordFileName", PasswordFileName);
            SSLCert = section->GetDirectives().Value("SSLCert", SSLCert);
            HardLimitPoolSize = section->GetDirectives().Value("HardLimitPoolSize", HardLimitPoolSize);
            HardLimitWaitDuration = section->GetDirectives().Value("HardLimitWaitDuration", HardLimitWaitDuration);
            LogLevel = section->GetDirectives().Value("LogLevel", LogLevel);
            ActivateTimeout = section->GetDirectives().Value("ActivateTimeout", ActivateTimeout);
            ActivationSleep = section->GetDirectives().Value("ActivationSleep", ActivationSleep);
            PanicOnDisconnect = section->GetDirectives().Value("PanicOnDisconnect", PanicOnDisconnect);
            IncorrectLagMin = section->GetDirectives().Value("IncorrectLagMin", IncorrectLagMin);
            UseBalancing = section->GetDirectives().Value("UseBalancing", UseBalancing);

            KeepaliveIdle = section->GetDirectives().Value("KeepaliveIdle", KeepaliveIdle);
            KeepalivesInterval = section->GetDirectives().Value("KeepalivesInterval", KeepalivesInterval);
            KeepalivesCount = section->GetDirectives().Value("KeepalivesCount", KeepalivesCount);

            auto subsections = section->GetAllChildren();

            auto version = subsections.find("Versioning");
            if (version != subsections.end()) {
                VersioningEnable = version->second->GetDirectives().Value("Enable", VersioningEnable);
                VersioningTableName = version->second->GetDirectives().Value("TableName", VersioningTableName);
            }
            if (!VersioningTableName) {
                VersioningTableName = section->GetDirectives().Value<TString>("TableName") + "_version";
            }
            return true;
        }

        void TPostgresStorageOptionsImpl::ToString(IOutputStream& os) const {
            os << "AgeForKillConnection: " << AgeForKillConnection << Endl;
            os << "ConnectionString: " << GetFullConnectionString(false, true) << Endl;
            os << "PasswordFileName: " << MD5::Calc(PasswordFileName) << Endl;
            os << "SSLCert: " << MD5::Calc(SSLCert) << Endl;
            os << "LocksTableName: " << LocksTableName << Endl;
            os << "LockTimeout: " << LockTimeout << Endl;
            os << "StatementTimeout: " << StatementTimeout << Endl;
            os << "KeepaliveIdle" << KeepaliveIdle << Endl;
            os << "KeepalivesInterval" << KeepalivesInterval << Endl;
            os << "KeepalivesCount" << KeepalivesCount << Endl;
            os << "TCPUserTimeout: " << TCPUserTimeout << Endl;
            os << "ConnectionTimeout: " << ConnectionTimeout << Endl;
            os << "HardLimitPoolSize: " << HardLimitPoolSize << Endl;
            os << "HardLimitWaitDuration: " << HardLimitWaitDuration << Endl;
            os << "LogLevel: " << LogLevel << Endl;
            os << "ActivationSleep: " << ActivationSleep << Endl;
            os << "ActivateTimeout: " << ActivateTimeout << Endl;
            os << "PanicOnDisconnect: " << PanicOnDisconnect << Endl;
            os << "<Versioning>" << Endl;
            os << "Enable: " << VersioningEnable << Endl;
            os << "TableName: " << VersioningEnable << Endl;
            os << "IncorrectLagMin: " << IncorrectLagMin << Endl;
            os << "UseBalancing: " << UseBalancing << Endl;
            os << "MainNamespace: " << MainNamespace << Endl;
            os << "</Versioning>" << Endl;
        }

        TAtomicSharedPtr<TPostgresConnectionsPool> TPostgresStorageOptionsImpl::ConstructRPool() const {
            TAtomicSharedPtr<TPostgresConnectionsPool> pool = Singleton<TGlobalPostgresConnectionsPool>()->Register(GetFullConnectionString(true));
            pool->SetHardLimitInfo(GetHardLimitPoolSize(), GetHardLimitWaitDuration());
            pool->SetAgeForKill(GetAgeForKillConnection());
            return pool;
        }

        TAtomicSharedPtr<TPostgresConnectionsPool> TPostgresStorageOptionsImpl::ConstructRWPool() const {
            TAtomicSharedPtr<TPostgresConnectionsPool> pool = Singleton<TGlobalPostgresConnectionsPool>()->Register(GetFullConnectionString(false));
            pool->SetHardLimitInfo(GetHardLimitPoolSize(), GetHardLimitWaitDuration());
            pool->SetAgeForKill(GetAgeForKillConnection());
            return pool;
        }

        TVector<TString> TPostgresStorageOptionsImpl::GetHosts() const {
            TVector<TString> result;
            TString connectionString = GetSimpleConnectionString();
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(connectionString, params);
            auto it = params.find("host");
            if (it == params.end()) {
                return result;
            }
            StringSplitter(it->second).SplitBySet(", ").SkipEmpty().Collect(&result);
            return result;
        }

        void TPostgresStorageOptionsImpl::SwitchHosts(const TString& hosts) {
            TVector<TString> result;
            TString connectionString = GetSimpleConnectionString();
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(connectionString, params);
            params["host"] = hosts;
            TString newConnectionString;
            for (auto&& i : params) {
                newConnectionString += i.first + "=" + i.second + " ";
            }
            SetSimpleConnectionString(newConnectionString);
        }

        TString TPostgresStorageOptionsImpl::GetFullConnectionString(const bool readOnly, const bool forPublishing) const {
            TString connectionString = GetSimpleConnectionString();
            TMap<TString, TString> params;
            NUtil::TTSKVRecordParser::Parse<' ', '='>(connectionString, params);
            TString targetNew;
            {
                auto it = params.find("target_session_attrs");
                if (it == params.end()) {
                    params.emplace("target_session_attrs", readOnly ? "any" : "read-write");
                } else {
                    CHECK_WITH_LOG(it->second == "any" || it->second == "read-write");
                    it->second = readOnly ? "any" : "read-write";
                }
            }
            if (!params.contains("connect_timeout")) {
                params.emplace("connect_timeout", ::ToString(ConnectionTimeout.Seconds()));
            }
            if (!params.contains("tcp_user_timeout")) {
                params.emplace("tcp_user_timeout", ::ToString(TCPUserTimeout.Seconds()));
            }
            if (!params.contains("keepalives_idle")) {
                params.emplace("keepalives_idle", ::ToString(KeepaliveIdle.Seconds()));
            }
            if (!params.contains("keepalives_interval")) {
                params.emplace("keepalives_interval", ::ToString(KeepalivesInterval.Seconds()));
            }
            if (!params.contains("keepalives_count")) {
                params.emplace("keepalives_count", ::ToString(KeepalivesCount));
            }

            connectionString = "";
            TString sslCert = SSLCert;
            TString password = PasswordFileName ? TIFStream(Strip(PasswordFileName)).ReadAll() : "";
            for (auto&& i : params) {
                if (i.first == "sslrootcert") {
                    if (!sslCert) {
                        sslCert = i.second;
                    }
                } else if (i.first == "password") {
                    if (!password) {
                        password = i.second;
                    }
                } else {
                    connectionString += i.first + "=" + i.second + " ";
                }
            }
            if (!!password) {
                if (forPublishing) {
                    connectionString += " password=" + MD5::Calc(password);
                } else {
                    connectionString += " password=" + password;
                }
            }

            if (!!SSLCert) {
                if (forPublishing) {
                    connectionString += " sslmode=verify-full sslrootcert=" + MD5::Calc(sslCert);
                } else {
                    connectionString += " sslmode=verify-full sslrootcert=" + sslCert;
                }
            }
            return connectionString;
        }

    }
}
