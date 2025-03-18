#include "config.h"

#include <library/cpp/json/json_reader.h>
#include <util/stream/file.h>

namespace NAapi {

TConfig TConfig::FromJson(const NJson::TJsonValue& params) {
    TConfig config;

    config.DiscStorePath = params["disc_store_path"].GetStringSafe("store");
    config.RamStoreSize = params["ram_store_size"].GetUIntegerSafe(1000000);

    config.YtProxy = params["yt_proxy"].GetStringSafe("hahn");
    config.YtTable = params["yt_table"].GetStringSafe("//home/devtools/vcs/data");
    config.YtSvnHead = params["yt_svn_head"].GetStringSafe("//home/devtools/vcs/svn_head");
    config.YtToken = params["yt_token"].GetStringSafe("");

    config.Host = params["host"].GetStringSafe("pepe.search.yandex.net");
    config.Port = params["port"].GetUIntegerSafe(6666);
    config.AsyncPort = params["async_port"].GetUIntegerSafe(7777);
    config.DebugOutput = params["debug_output"].GetUIntegerSafe(0);
    config.EventlogPath = params["eventlog_path"].GetStringSafe("event.log");
    config.InnerPoolSize = params["inner_pool_size"].GetUIntegerSafe(48);
    config.AsyncServerThreads = params["async_server_threads"].GetUIntegerSafe(8);

    config.SensorsPort = params["sensors_port"].GetUIntegerSafe(6677);
    config.MonitorPort = params["monitor_port"].GetUIntegerSafe(6699);

    if (params.Has("warmup")) {
        if (params["warmup"].Has("paths")) {
            const auto& paths = params["warmup"]["paths"];
            for (const auto& item : paths.GetArraySafe()) {
                config.WarmupPaths.push_back(item.GetStringSafe());
            }
        }
    }

    config.HgBinaryPath = params["hg_path"].GetStringSafe("hg");
    config.HgServer = params["hg_server"].GetStringSafe("ssh://arcadia-hg.yandex-team.ru/arcadia.hg");
    config.HgUser = params["hg_user"].GetStringSafe("arcadia-devtools");
    config.HgKey = params["hg_key"].GetStringSafe("");

    return config;
}

TConfig TConfig::FromFile(const TString& path) {
    using namespace NJson;

    TJsonValue jsonConfig;
    TFileInput jsonInput(path);
    Y_ENSURE(ReadJsonTree(&jsonInput, &jsonConfig));
    return FromJson(jsonConfig);
}

}  // namespace NAapi
