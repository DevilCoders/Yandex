#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/string.h>

namespace NAapi {

struct TConfig {
    // Disc cache
    TString DiscStorePath;

    // Ram cache
    ui64 RamStoreSize;

    // Yt
    TString YtProxy;
    TString YtTable;
    TString YtSvnHead;
    TString YtToken;

    // Server
    TString Host;
    ui64 Port;
    ui64 AsyncPort;
    bool DebugOutput;
    TString EventlogPath;
    ui64 InnerPoolSize;
    TVector<TString> WarmupPaths;
    ui64 AsyncServerThreads;

    // Solomon
    ui64 SensorsPort;
    ui64 MonitorPort;

    // Hg
    TString HgBinaryPath;
    TString HgServer;
    TString HgUser;
    TString HgKey;

    static TConfig FromJson(const NJson::TJsonValue& params);
    static TConfig FromFile(const TString& path);
};

}  // namespace NAapi
