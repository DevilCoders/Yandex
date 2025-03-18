#include "database.h"
#include "env.h"
#include "server.h"

#include <antirobot/cbb/cbb_fast/protos/config.pb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getoptpb/getoptpb.h>

#include <maps/libs/pgpool/include/pgpool3.h>

#include <util/folder/path.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/vector.h>
#include <util/system/env.h>
#include <util/system/yassert.h>


NCbbFast::TConfig ParseArguments(int argc, const char** argv) {
    NCbbFast::TConfig config = NGetoptPb::GetoptPbOrAbort(argc, argv, { .DumpConfig = false});
    if (!config.HasDbUser()) {
        const auto dbUser = GetEnv("DB_USER");
        if (!dbUser.empty()) {
            config.SetDbUser(dbUser);
        }
    }

    if (!config.HasDbPort()) {
        const auto dbPort = GetEnv("DB_PORT");
        if (!dbPort.empty()) {
            config.SetDbPort(FromString<ui32>(dbPort));
        }
    }

    if (!config.HasDbName()) {
        const auto dbName = GetEnv("MAIN_DB_NAME");
        if (!dbName.empty()) {
            config.SetDbName(dbName);
        }
    }

    if (config.DbHostsSize() == 0) {
        const auto hosts = GetEnv("MAIN_DB_HOSTS");
        if (!hosts.empty()) {
            for (const auto& host : SplitString(hosts, ",")) {
                config.AddDbHosts(host);
            }
        }
    }

    if (!config.HasDbPassword()) {
        const auto password = GetEnv("DB_PASSWORD");
        if (!password.empty()) {
            config.SetDbPassword(password);
        }
    }

    if (!config.HasTvmClientId()) {
        const auto clientId = GetEnv("TVM_CLIENT_ID");
        if (!clientId.empty()) {
            config.SetTvmClientId(FromString<ui32>(clientId));
        }
    }

    const auto port = config.GetDbPort();
    Y_VERIFY(port > 0 && port < Max<ui16>());
    Y_VERIFY(config.HasDbUser());
    Y_VERIFY(config.HasDbName());
    Y_VERIFY(config.HasDbPassword());

    if (!config.GetDebug()) {
        Y_VERIFY(config.DbHostsSize() >= 3);
    }

    return config;
}

int main(int argc, const char** argv) {
    const auto config = ParseArguments(argc, argv);

    NCbbFast::TEnv env(config);

    NServer::THttpServerConfig serverConfig;
    serverConfig.SetThreads(config.GetThreads());
    serverConfig.SetPort(config.GetPort());
    serverConfig.SetLogFile(config.GetLogFile());

    NCbbFast::TCbbServer server(serverConfig, &env);
    server.Start();
    server.Wait();

    return 0;
}
