#include "config.h"
#include <kernel/daemon/config/patcher.h>
#include <util/system/env.h>
#include <library/cpp/testing/common/env.h>

namespace NServerTest {
    const TString CommonDaemonConfiguration = R"(
    CType: tests
    AccessLog : access.log
    IndexLog :
    LoggerType : console
    LogLevel : ${LogLevel}
    LogRotation : false
    MetricsMaxAge : 10
    MetricsPrefix : Refresh_
    StdErr : current.stderr
    StdOut : current.stdout
    <Controller>
        ClientTimeout : 200
        ConfigsControl : false
        ConfigsRoot : /
        Log : current-controller.log
        MaxConnections : 0
        MaxQueue : 0
        Port : ${DaemonPort}
        StartServer : 1
        StateRoot : /
        Threads : 20
        <DMOptions>
            ConnectionTimeout : 100
            CType : prestable
            Enabled : 0
            Host : localhost
            InteractionTimeout : 60000
            Port : 11111
            ServiceType : server
            Slot : asdasda:12312
            TimeoutS : 45
        </DMOptions>
    </Controller>
)";

    const TString CommonServerConfiguration = R"(
    EventLog: cerr
    <HttpServer>
        Port: ${BasePort}
        Threads: 16
        CompressionEnabled: true
    </HttpServer>

    <TagDescriptions>
        DBName: main-db
    </TagDescriptions>

    <Monitoring>
        Port: ${MonitoringPort}
        <Labels>
            service: logistic-dispatcher
            project: taxi
            cluster: testing
        </Labels>
    </Monitoring>

    <RequestHandlers>
        <default>
        ThreadsCount: 16
        </default>
    </RequestHandlers>

    <RequestProcessing>
    </RequestProcessing>

    <Databases>
    </Databases>

    <LocksManager>
        Type: local
        Prefix: test-lock-storage:
    </LocksManager>

    <Settings>
        Type: settings-pack
        <Segments>
            <default>
                Type: db
                DBName: main-db
                Freshness: 1s
            </default>
        </Segments>
    </Settings>

    DBName: main-db

    <RequestPolicy>
    </RequestPolicy>
    <Localization>
        DBName: main-db
    </Localization>

    <RTBackgroundManager>
        DBName: main-db
        <TaskManagers>
            <test>
                Type: db
                DBName: main-db
            </test>
        </TaskManagers>
    </RTBackgroundManager>
)";

    TString TConfigGenerator::BuildMigrationsFromFolders(const TString& folders) const {
        TStringBuilder sb;
        sb << R"(
            <Migrations>
                <Sources>
        )";
        const auto dirs = StringSplitter(folders).SplitBySet(", ").SkipEmpty().ToList<TString>();
        CHECK_WITH_LOG(!dirs.empty());
        ui32 idx = 0;
        for (const auto& dir : dirs) {
            sb << "<m" << ::ToString(++idx) << ">" << Endl;
            sb << "Type: folder" << Endl;
            sb << "Path: " << dir << Endl;
            sb << "</m" << ::ToString(idx) << ">" << Endl;
        }
        sb << R"(
                </Sources>
            </Migrations>
        )";
        return sb;
    }

    TString TConfigGenerator::GetExternalDatabasesConfiguration() const {
        TStringBuilder sb;
        sb << R"(
        <main-db>
            Type: Postgres
            ConnectionString: host=${DBHost} port=${DBPort} target_session_attrs=read-write dbname=${DBName} user=${DBUser} password=${PPass}
            MainNamespace: ${DBMainNamespace}
            UseBalancing: true
          )";
        sb << BuildMigrationsFromFolders(GetEnv("PG_MIGRATIONS_DIR"));
        sb << R"(
        </main-db>
          )";
        return sb;
    }

    TString TConfigGenerator::GetFullConfig() const {
        TStringStream os;
        os << "<DaemonConfig>" << Endl;
        os << PatchConfiguration(CommonDaemonConfiguration) << Endl;
        os << "</DaemonConfig>" << Endl;

        os << "<Server>" << Endl;
        os << PatchConfiguration(CommonServerConfiguration) << Endl;
        os << "<ExternalDatabases>" << Endl;
        os << PatchConfiguration(GetExternalDatabasesConfiguration()) << Endl;
        os << "</ExternalDatabases>" << Endl;
        os << "<ExternalQueues>" << Endl;
        os << PatchConfiguration(GetExternalQueuesConfiguration()) << Endl;
        os << "</ExternalQueues>" << Endl;
        os << "<AbstractExternalAPI>" << Endl;
        os << PatchConfiguration(GetExternalApiConfiguration()) << Endl;
        os << "</AbstractExternalAPI>" << Endl;

        os << "<Processors>" << Endl;
        os << "<default_config:for_all>" << Endl;
        os << "AuthModuleName: fake" << Endl;
        os << "</default_config:for_all>" << Endl;
        os << PatchConfiguration(GetProcessorsConfiguration()) << Endl;
        os << "</Processors>" << Endl;

        os << "<AuthModules>" << Endl;
        os << PatchConfiguration(GetAuthConfiguration()) << Endl;
        os << "</AuthModules>" << Endl;

        os << PatchConfiguration(GetCustomManagersConfiguration()) << Endl;
        os << "</Server>" << Endl;

        return os.Str();
    }

    TString TConfigGenerator::PatchConfiguration(const TString& input) const {
        TConfigPatcher patcher("");
        patcher.SetVariable("BasePort", ::ToString(BasePort));
        patcher.SetVariable("DaemonPort", ::ToString(DaemonPort));
        patcher.SetVariable("MonitoringPort", ::ToString(MonitoringPort));
        patcher.SetVariable("LogLevel", ::ToString((int)LogLevel));
        patcher.SetVariable("HomeDir", HomeDir);
        patcher.SetVariable("PPass", PPass);
        patcher.SetVariable("DBHost", DBHost);
        patcher.SetVariable("DBName", DBName);
        patcher.SetVariable("DBUser", DBUser);
        patcher.SetVariable("DBPort", ::ToString(DBPort));
        patcher.SetVariable("WorkDir", WorkDir);
        patcher.SetVariable("DBMainNamespace", DBMainNamespace);
        patcher.SetVariable("ARCADIA_ROOT", ArcadiaSourceRoot());

        TString patched;
        patcher.GetPreprocessor()->Process(input, patched);

        return patched;
    }
}
