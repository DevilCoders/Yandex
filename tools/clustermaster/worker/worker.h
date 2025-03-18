#pragma once

#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/proto/state_row.pb.h>

#include <kernel/yt/dynamic/table.h>

// Constants
extern const char cssData[];

struct TWorkerGlobals {
    // Args
    int HttpPort;
    int WorkerPort;
    int NetworkHeartbeat;
    int WorkerHeartbeatSeconds;

    TString YtProxy;
    TString YtVarDir;
    TString YtTokenPath;
    TString YtUser;

    /**
     * If the worker reads PID of a started process during initial load,
     * but that process is not found and considered "lost",
     * then we might want to restart it automatically.
     * This is helpful together with YT storage in case your machine is unstable and frequently respawned.
     */
    bool EnableLostTargetRestart;

    TString VarDirPath;
    TString AuthKeyPath;

    TString UrlPrefix;

    // Global data
    TMaybe<TYtStorage> YtStorage;

    TString ConfigPath;
    TString ScriptPath;
    TString StateDirPath;
    TString LogDirPath;
    TString ArchiveDirPath;
    TString LogFilePath;
    TString BinaryPath;
    TString DefaultResourcesHost;
    TString DoNotTrackFlagPath;
    std::pair<float, float> PriorityRange;
    int SolverHttpPort;
    int NTargetLogs;
    TString ResStatFile;

    TWorkerGlobals();
};
