#include "worker.h"

#include "constants.h"
#include "diskspace_notifier.h"
#include "graph_change_watcher.h"
#include "http.h"
#include "log.h"
#include "modstate.h"
#include "remote.h"
#include "resource_manager.h"
#include "state_file.h"
#include "worker_target_graph.h"
#include "worker_variables.h"
#include "worker_workflow.h"

#include <tools/clustermaster/common/listening_thread.h>
#include <tools/clustermaster/common/thread_util.h>
#include <tools/clustermaster/common/util.h>
#include <tools/clustermaster/communism/client/client.h>
#include <tools/clustermaster/communism/util/daemon.h>
#include <tools/clustermaster/communism/util/dirut.h>
#include <tools/clustermaster/communism/util/file_reopener_by_signal.h>
#include <tools/clustermaster/communism/util/pidfile.h>
#include <tools/clustermaster/proto/worker_options.pb.h>

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/dynamic/table.h>
#include <yt/yt/core/misc/shutdown.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/random/random.h>
#include <util/system/daemon.h>
#include <util/system/fs.h>
#include <util/system/sigset.h>
#include <util/system/sysstat.h>

TWorkerGlobals::TWorkerGlobals() {
    HttpPort = 0;
    WorkerPort = CLUSTERMASTER_WORKER_PORT;
    NetworkHeartbeat = 1;
    WorkerHeartbeatSeconds = 10;

    VarDirPath = "./var";
    AuthKeyPath = "";
    DoNotTrackFlagPath = "";

    UrlPrefix = "";

    DefaultResourcesHost = "localhost";
    PriorityRange = std::pair<float, float>(0.0f, 1.0f);

    SolverHttpPort = 3132;
    NTargetLogs = 0;
}

void sigchld(int) {
}

int worker(int argc, char **argv, TWorkerGlobals& workerGlobals) {
    LogLevel::Level() = GetLogLevelFromEnvOrDefault(ELogLevel::Info);
    TLogLevel<LogSubsystem::http>::Level() = GetLogLevelFromEnvOrDefault(ELogLevel::Info);

    NGetoptPb::TGetoptPb getoptPb;
    NClusterMaster::TWorkerOptions workerOptionsPb;
    getoptPb.AddOptions(workerOptionsPb);
    getoptPb.GetOpts().AddLongOption('V', "version", "print svn version and exit").NoArgument().Handler(&PrintSvnVersionAndExit0);

    TString errorMsg;
    bool ret = getoptPb.ParseArgs(argc, const_cast<const char**>(argv), workerOptionsPb, errorMsg);
    if (ret) {
        Cerr << "Using settings:\n"
             << "====================\n";
        getoptPb.DumpMsg(workerOptionsPb, Cerr);
        Cerr << "====================\n";
    } else {
        Cerr << errorMsg;
    }

    bool foreground = workerOptionsPb.GetForeground();
    bool archives = workerOptionsPb.GetArchives();
    TString pidFilePath = workerOptionsPb.GetPidFilePath();

    workerGlobals.WorkerPort = workerOptionsPb.GetWorkerPort();
    workerGlobals.NetworkHeartbeat = workerOptionsPb.GetNetworkHeartbeat();
    workerGlobals.WorkerHeartbeatSeconds = workerOptionsPb.GetWorkerHeartbeatSeconds();
    workerGlobals.DefaultResourcesHost = workerOptionsPb.GetDefaultResourcesHost();

    TString arg = workerOptionsPb.GetPriorityRange();
    if (arg) {
        const size_t colon = TStringBuf(arg).find(':');

        if (colon != TStringBuf::npos) {
            workerGlobals.PriorityRange.first = FromString<float>(TStringBuf(arg, 0, colon));
            workerGlobals.PriorityRange.second = FromString<float>(TStringBuf(arg, colon + 1, TString::npos));
        } else {
            workerGlobals.PriorityRange.first = FromString<float>(arg);
            workerGlobals.PriorityRange.second = workerGlobals.PriorityRange.first;
        }

        if (workerGlobals.PriorityRange.first < 0.0)
            workerGlobals.PriorityRange.first = 0.0;

        if (workerGlobals.PriorityRange.first > 1.0)
            workerGlobals.PriorityRange.first = 1.0;

        if (workerGlobals.PriorityRange.second < 0.0)
            workerGlobals.PriorityRange.second = 0.0;

        if (workerGlobals.PriorityRange.second > 1.0)
            workerGlobals.PriorityRange.second = 1.0;

        if (workerGlobals.PriorityRange.second < workerGlobals.PriorityRange.first)
            ythrow yexception() << "invalid priority range";
    }

    workerGlobals.UrlPrefix = workerOptionsPb.GetUrlPrefix();
    workerGlobals.VarDirPath = workerOptionsPb.GetVarDirPath();
    Y_ENSURE(!workerGlobals.VarDirPath.empty(),
        "Need VarDirPath (local directory for variable data) to be specified");

    workerGlobals.YtProxy = workerOptionsPb.GetYtProxy();
    workerGlobals.YtVarDir = workerOptionsPb.GetYtVarDir();
    workerGlobals.YtTokenPath = workerOptionsPb.GetYtTokenPath();
    workerGlobals.YtUser = workerOptionsPb.GetYtUser();
    workerGlobals.EnableLostTargetRestart = workerOptionsPb.GetEnableLostTargetRestart();
    workerGlobals.DoNotTrackFlagPath = workerOptionsPb.GetDoNotTrackFlagPath();
    workerGlobals.AuthKeyPath = workerOptionsPb.GetAuthKeyPath();
    workerGlobals.LogFilePath = workerOptionsPb.GetLogFilePath();

    if (workerOptionsPb.GetDebugLog()) {
        LogLevel::Level() = Debug;
    }

    workerGlobals.HttpPort = workerOptionsPb.GetHttpPort();
    workerGlobals.SolverHttpPort = workerOptionsPb.GetSolverHttpPort();
    workerGlobals.NTargetLogs = workerOptionsPb.GetNTargetLogs();

    // prepare YT storage
    if (workerGlobals.YtProxy.empty() != workerGlobals.YtVarDir.empty()) {
        ythrow yexception() << "Need both -c (ytproxy) and -y (ytvardir) to be specified.";
    }
    if (!workerGlobals.YtProxy.empty()) {
        NYT::NApi::IClientPtr client = NYT::NApi::CreateClient(
            workerGlobals.YtProxy,
            workerGlobals.YtUser,
            ReadYtToken(workerGlobals.YtTokenPath)
        );
        workerGlobals.YtStorage.ConstructInPlace(
            workerGlobals.YtVarDir,
            "worker_state",
            client,
            workerOptionsPb.GetYtBackoffPeriodMin(),
            workerOptionsPb.GetYtBackoffPeriodMax()
        );
    }

    // create dirs (XXX: check success)
    workerGlobals.ConfigPath = workerGlobals.VarDirPath + "/config";
    workerGlobals.ScriptPath = workerGlobals.VarDirPath + "/script";
    workerGlobals.LogDirPath = workerGlobals.VarDirPath + "/logs";
    if (archives)
        workerGlobals.ArchiveDirPath = workerGlobals.VarDirPath + "/archive";
    workerGlobals.StateDirPath = workerGlobals.VarDirPath + "/state";


    Mkdir(workerGlobals.VarDirPath.data(), MODE0755);
    if (archives)
        Mkdir(workerGlobals.ArchiveDirPath.data(), MODE0755);
    Mkdir(workerGlobals.LogDirPath.data(), MODE0755);
    Mkdir(workerGlobals.StateDirPath.data(), MODE0755);

#ifndef _win_

    // ignore all signals - they're to be cought with sigtimedwait in main thread
    sigset_t newMask, oldMask;
    SigFillSet(&newMask);
    SigProcMask(SIG_BLOCK, &newMask, &oldMask);

    InitializeDaemonGeneric(workerGlobals.LogFilePath, pidFilePath, foreground);

    // listman
    TWorkerListManager listManager;

    // read script
    TWorkerGraph targetGraph(&listManager, &workerGlobals);

    try {
        targetGraph.LoadState();
        /* sync variables in all stores */
        targetGraph.SaveVariables();
    } catch (const TWorkerGraph::TNoConfigException &e) {
        LOG("No saved config found, doing nothing until master connects");
    } catch (const yexception &e) {
        ERRORLOG("Cannot load graph state: " << e.what() << ", doing nothing until master connects");
    }

    // start communism client
    LOG("Starting communism client");
    GetResourceManager().Start();

    // start HTTP server
    LOG("Starting HTTP server");

    InitNetworkSubSystem();

    THolder<TWorkerHttpServer> serv;

    unsigned maxServerStartTries = 1000;

    if (workerGlobals.HttpPort == 0) {
        // TODO starting http server on random port could cause problems with other programs running on host. -h parameter was added to
        // specify port explicitly. Remove this branch and make -h parameter mandatory after all rc.d scripts would be updated.
        do {
            workerGlobals.HttpPort = 1024 + (RandomNumber<ui16>() % 8192);
            serv.Reset(new TWorkerHttpServer(targetGraph, THttpServerOptions(workerGlobals.HttpPort)));
            serv->RestartRequestThreads(1, 100);
        } while (--maxServerStartTries && !serv->Start());
        if (!maxServerStartTries)
            ythrow yexception() << "failed to start http server on random port";
    } else {
        serv.Reset(new TWorkerHttpServer(targetGraph, THttpServerOptions(workerGlobals.HttpPort)));
        serv->RestartRequestThreads(1, 100);
        if (!serv->Start())
            ythrow yexception() << "failed to start http server on given port";
    }

    LOG("Started http server on port " << workerGlobals.HttpPort);

    // start thread for master connects
    LOG("Starting listening thread");
    TListeningThread listener("::", workerGlobals.WorkerPort, [&targetGraph](THolder<TStreamSocket> socket)
        {
            return MakeHolder<TWorkerControlWorkflow>(targetGraph, std::move(socket));
        });

    LOG("Starting diskspace notifier thread");
    TDiskspaceNotifier::TParams notifierParams;
    notifierParams.SetMountPoint(workerGlobals.VarDirPath.data());
    TDiskspaceNotifier diskspaceNotifier(notifierParams);

    LOG("Starting resources events processing thread");
    TWorkerGraph::TResourcesEventsThread resourcesEventsThread(&targetGraph);
    resourcesEventsThread.Start();

    // restore old signal mask with adding SIGCHLD to it
    // SIGCHLD has to be blocked in the thread and processed in sigtimedwait
    SigAddSet(&oldMask, SIGCHLD);
    SigProcMask(SIG_SETMASK, &oldMask, nullptr);

    struct sigaction sa;

    // setup noop handler for SIGCHLD, otherwise sigtimedwait won't ever exit on FreeBSD
    Zero(sa);
    sa.sa_handler = sigchld;
    SigEmptySet(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP|SA_RESTART;
    sigaction(SIGCHLD, &sa, nullptr);

    // terminate by SIGHUP if in foreground and don't touch handler otherwise
    if (foreground) {
        Zero(sa);
        sa.sa_handler = SIG_DFL;
        SigEmptySet(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGHUP, &sa, nullptr);
    }

    LOG("Starting main loop");
    SetCurrentThreadName("main thread");

    // ...and wait for results
    SigEmptySet(&newMask);
    SigAddSet(&newMask, SIGCHLD);
    struct timespec timeout;
    timeout.tv_sec = workerGlobals.WorkerHeartbeatSeconds;
    timeout.tv_nsec = 0;
    siginfo_t info;

    time_t lastExpiredTasksCheck = 0;
    time_t lastFreeSpaceCheck = 0;
    bool needToRestartLostTarget = workerGlobals.EnableLostTargetRestart;

    /* Wait until we reach a master, so that we can fill in CMAPI in starting tasks */
    if (needToRestartLostTarget) {
        LOG("Waiting master connection...");
        int polls = 30;
        int masters = 0;
        while (polls-- > 0 && masters == 0) {
            if (polls % 5 == 0) {
                LOG("Still waiting master connection...");
            }
            Sleep(TDuration::Seconds(10));
            masters = Singleton<TMasterMultiplexor>()->GetNumMasters();
        }
        LOG("Number of connected masters: " << masters);
    }

    while (1) {
        targetGraph.ProcessLostTasks(needToRestartLostTarget);
        needToRestartLostTarget = false;

#ifndef _darwin_
        sigtimedwait(&newMask, &info, &timeout);
#else
        abort();
#endif

        targetGraph.TryReapSomeTasks();

        time_t now = time(nullptr);

        if (now >= lastExpiredTasksCheck + workerGlobals.WorkerHeartbeatSeconds) {
            targetGraph.CheckExpiredTasks();
            lastExpiredTasksCheck = now;
        }

        if (now >= lastFreeSpaceCheck + 10) {
            targetGraph.CheckLowFreeSpace();
            lastFreeSpaceCheck = now;
        }
    }
#endif

    workerGlobals.YtStorage.Clear(); // destroy yt client before NYT::Shutdown()

    LOG("Normal exit");
    NYT::Shutdown();

    return 0;
}

int run_substitute(int argc, char **argv, int (*new_main)(int, char**)) {
    char *new_argv[argc-1];
    new_argv[0] = argv[0];
    for (int i = 2; i < argc; ++i)
        new_argv[i-1] = argv[i];
    return new_main(argc-1, new_argv);
}

int WorkerMain(int argc, char **argv) {
    InstallSegvHandler();
    TWorkerGlobals workerGlobals;
    // RealPath is not good in nanny environment (USERFEAT-1351) because when re-deploy happens, the binary is removed, only link stays.
    TFsPath binaryPath = argv[0];
    if (binaryPath.IsRelative()) {
        binaryPath = TFsPath(NFs::CurrentWorkingDirectory()) / binaryPath;
    }
    workerGlobals.BinaryPath = binaryPath;
    if (argc >= 2 && TString(argv[1]) == "--cmremote")
        return run_substitute(argc, argv, remote);
    if (argc >= 2 && TString(argv[1]) == "--mod-state")
        return run_substitute(argc, argv, modstate);
    return worker(argc, argv, workerGlobals);
}
