#pragma once

#include "revision.h"

#include <tools/clustermaster/common/async_jobs.h>
#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/proto/master_options.pb.h>

#include <yweb/config/hostconfig.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/network/ip.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

// Constants
extern const char cssData[];
extern const char envJSData[];
extern const char updaterJSData[];
extern const char mouseJSData[];
extern const char hintJSData[];
extern const char menuJSData[];
extern const char summaryJSData[];
extern const char workersJSData[];
extern const char targetsJSData[];
extern const char clustersJSData[];
extern const char graphJSData[];
extern const char variablesJSData[];
extern const char monitoringJSData[];
extern const char graphXSLTData[];

// Options
struct TMasterOptions : TNonCopyable {
    bool DoNotDaemonize; // -f
    TString PidFilePath; // -P
    TString LogFilePath; // -l
    bool DebugLogOutput; // -D

    TString ScriptPath; // -s
    TString VarDirPath; // -v
    TVector<TString> HostCfgPaths; // -c
    TString CustomWorkersListPath; // -C

    TIpPort WorkerPort; // -w
    TString AuthKeyPath; // -a
    unsigned NetworkRetry; // -r
    unsigned NetworkHeartbeat; // -b

    TString InstanceName; // -n
    TIpPort HTTPPort; // -h
    TIpPort ROHTTPPort; // -H
    bool DisableGraphviz; // -G
    unsigned GraphvizIterationsLimit; // -z

    unsigned PokeHeartbeat; // --poke-heartbeat
    unsigned MaxPokesPerHeartbeat; // --max-pokes-per-heartbeat

    unsigned HTTPUpdaterDelay; // --http-updater-delay
    unsigned ProxyHttpTimeout; // --proxy-http-timeout

    bool DumpGraphOnReload; // --dump-graph-on-reload
    bool NoUserNotify; // --no-user-notify
    bool EnableJuggler; // --enable-juggler

    bool RetryOnFailureEnabled; // -R
    bool PerSecondTargetsEnabled; // -S
    bool Panic; // --panic

    TString YtProxy;
    TString YtVarDir;
    TString YtTokenPath;
    TString YtUser;
    ui64 BackoffPeriodMin;
    ui64 BackoffPeriodMax;

    TMaybe<TYtStorage> YtStorage;

    ui8 NTargetLogs;
    TString MailSender;
    TString TvmSecretPath;
    TString TelegramSecretPath;
    TString JNSSecretPath;
    TString IdmRole;
    bool Authenticate;
    TString SmtpServer;

    struct TInvalid: yexception {};

    TMasterOptions();

    void Check() const;
    void Fill(NClusterMaster::TMasterOptions& masterOptionsPb);
};

extern TMasterOptions MasterOptions;

class TFileReopenerBySignal;

// Env
struct TMasterEnv {
    volatile bool NeedReloadScript;
    volatile bool NeedExit;
    TString ErrorNotification;
    TRevision ErrorNotificationRevision;
    TMutex ErrorNotificationMutex;
    TString SelfHostname;
    THolder<TFileReopenerBySignal> LogFileReopener;

    TMasterEnv();

    void ClearErrorNotification();
    void SetErrorNotification(TString errorNotification);
    TString GetErrorNotification() const;
};

extern TMasterEnv MasterEnv;
