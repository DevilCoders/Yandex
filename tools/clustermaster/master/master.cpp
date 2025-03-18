#include "master.h"

#include "constants.h"
#include "http.h"
#include "lockablehandle.h"
#include "log.h"
#include "master_list_manager.h"
#include "master_profiler.h"
#include "master_target_graph.h"
#include "signal_blocking_guard.h"
#include "worker_pool.h"

#include <tools/clustermaster/common/thread_util.h>
#include <tools/clustermaster/communism/util/daemon.h>
#include <tools/clustermaster/communism/util/dirut.h>
#include <tools/clustermaster/communism/util/file_reopener_by_signal.h>
#include <tools/clustermaster/communism/util/pidfile.h>
#include <tools/clustermaster/proto/state_row.pb.h>

#include <kernel/yt/dynamic/client.h>
#include <kernel/yt/dynamic/table.h>
#include <yt/yt/core/misc/shutdown.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/generic/ptr.h>
#include <util/network/init.h>
#include <util/stream/file.h>
#include <util/system/backtrace.h>
#include <util/system/daemon.h>
#include <util/system/fs.h>
#include <util/system/hostname.h>
#include <util/system/sigset.h>
#include <util/system/thread.h>

#ifndef _win_
#   include <resolv.h>
#endif // !_win_

// Constants
//const char* shell = _PATH_BSHELL;

TMasterOptions::TMasterOptions()
    : DoNotDaemonize(false)
    , PidFilePath()
    , LogFilePath()
    , DebugLogOutput(false)

    , ScriptPath()
    , VarDirPath()
    , HostCfgPaths()
    , CustomWorkersListPath()

    , WorkerPort(CLUSTERMASTER_WORKER_PORT)
    , AuthKeyPath()
    , NetworkRetry(5)
    , NetworkHeartbeat(1)

    , InstanceName()
    , HTTPPort(CLUSTERMASTER_MASTER_HTTP_PORT)
    , ROHTTPPort(0)
    , DisableGraphviz(false)
    , GraphvizIterationsLimit(0)

    , PokeHeartbeat(5)
    , MaxPokesPerHeartbeat(100)

    , HTTPUpdaterDelay(1)
    , ProxyHttpTimeout(10)

    , DumpGraphOnReload(false)
    , NoUserNotify(false)
    , EnableJuggler(false)

    , RetryOnFailureEnabled(false)
    , PerSecondTargetsEnabled(false)
    , Panic(false)
    , YtProxy()
    , YtVarDir()
    , NTargetLogs()
    , MailSender()
    , TvmSecretPath()
    , TelegramSecretPath()
    , IdmRole()
    , Authenticate(false)
    , SmtpServer()
{
}

void TMasterOptions::Check() const {
    if (VarDirPath.empty())
        throw TInvalid() << "no VarDirPath specified";
    if (ScriptPath.empty())
        throw TInvalid() << "no script specified";
    if (WorkerPort == 0)
        throw TInvalid() << "worker port should be non-zero";
    if (NetworkRetry == 0)
        throw TInvalid() << "network retry should be non-zero";
    if (NetworkHeartbeat == 0)
        throw TInvalid() << "network heartbeat should be non-zero";
    if (HTTPPort == 0)
        throw TInvalid() << "HTTP port should be non-zero";
    if (PokeHeartbeat == 0)
        throw TInvalid() << "poke heartbeat should be non-zero";
    if (HTTPUpdaterDelay == 0)
        throw TInvalid() << "http updater delay should be non-zero";
    if (YtProxy.empty() != YtVarDir.empty()) {
        throw TInvalid() << "yt-proxy and yt-var-dir can be specified only together";
    }
    if (YtProxy.empty() != YtTokenPath.empty()) {
        throw TInvalid() << "YtProxy and YtTokenPath can be specified only together";
    }
    if (YtProxy.empty() != YtUser.empty()) {
        throw TInvalid() << "YtProxy and YtUser can be specified only together";
    }
    if (NTargetLogs >= 9) {
        throw TInvalid() << "It is voluntarily forbidden to show more than 8 links for previous logs";
    }
}

void TMasterOptions::Fill(NClusterMaster::TMasterOptions& masterOptionsPb) {
    DoNotDaemonize = masterOptionsPb.GetDoNotDaemonize();
    PidFilePath = masterOptionsPb.GetPidFilePath();
    LogFilePath = masterOptionsPb.GetLogFilePath();
    DebugLogOutput = masterOptionsPb.GetDebugLogOutput();

    VarDirPath = masterOptionsPb.GetVarDirPath();
    ScriptPath = masterOptionsPb.GetScriptPath();
    for (const TString& hostCfgPath : masterOptionsPb.GetHostCfgPaths()) {
        HostCfgPaths.push_back(hostCfgPath);
    }
    CustomWorkersListPath = masterOptionsPb.GetCustomWorkersListPath();

    WorkerPort = masterOptionsPb.GetWorkerPort();
    AuthKeyPath = masterOptionsPb.GetAuthKeyPath();
    NetworkRetry = masterOptionsPb.GetNetworkRetry();
    NetworkHeartbeat = masterOptionsPb.GetNetworkHeartbeat();

    InstanceName = masterOptionsPb.GetInstanceName();
    HTTPPort = masterOptionsPb.GetHTTPPort();
    ROHTTPPort = masterOptionsPb.GetROHTTPPort();
    DisableGraphviz = masterOptionsPb.GetDisableGraphviz();
    GraphvizIterationsLimit = masterOptionsPb.GetGraphvizIterationsLimit();

    PokeHeartbeat = masterOptionsPb.GetPokeHeartbeat();
    MaxPokesPerHeartbeat = masterOptionsPb.GetMaxPokesPerHeartbeat();

    HTTPUpdaterDelay = masterOptionsPb.GetHTTPUpdaterDelay();
    ProxyHttpTimeout = masterOptionsPb.GetProxyHttpTimeout();

    DumpGraphOnReload = masterOptionsPb.GetDumpGraphOnReload();
    NoUserNotify = masterOptionsPb.GetNoUserNotify();
    EnableJuggler = masterOptionsPb.GetEnableJuggler();

    RetryOnFailureEnabled = masterOptionsPb.GetRetryOnFailureEnabled();
    PerSecondTargetsEnabled = masterOptionsPb.GetPerSecondTargetsEnabled();
    Panic = masterOptionsPb.GetPanic();

    YtProxy = masterOptionsPb.GetYtProxy();
    YtVarDir = masterOptionsPb.GetYtVarDir();
    YtTokenPath = masterOptionsPb.GetYtTokenPath();
    YtUser = masterOptionsPb.GetYtUser();
    BackoffPeriodMin = masterOptionsPb.GetYtBackoffPeriodMin();
    BackoffPeriodMax = masterOptionsPb.GetYtBackoffPeriodMax();

    NTargetLogs = masterOptionsPb.GetNTargetLogs();

    MailSender = masterOptionsPb.GetMailSender();
    TvmSecretPath = masterOptionsPb.GetTvmSecretPath();
    TelegramSecretPath = masterOptionsPb.GetTelegramSecretPath();
    JNSSecretPath = masterOptionsPb.GetJNSSecretPath();
    IdmRole = masterOptionsPb.GetIdmRole();
    Authenticate = masterOptionsPb.GetAuthenticate();
    SmtpServer = masterOptionsPb.GetSmtpServer();
}

TMasterOptions MasterOptions;

TMasterEnv::TMasterEnv()
    : NeedReloadScript(true)
    , NeedExit(false)
    , ErrorNotification()
    , ErrorNotificationRevision()
    , ErrorNotificationMutex()
    , SelfHostname()
{}

void TMasterEnv::ClearErrorNotification() {
    TGuard<TMutex> guard(ErrorNotificationMutex);
    MasterEnv.ErrorNotification.clear();
    MasterEnv.ErrorNotificationRevision.Up();
}

void TMasterEnv::SetErrorNotification(TString errorNotification) {
    TGuard<TMutex> guard(ErrorNotificationMutex);
    MasterEnv.ErrorNotification = errorNotification;
    MasterEnv.ErrorNotificationRevision.Up();
}

TString TMasterEnv::GetErrorNotification() const {
    TGuard<TMutex> guard(ErrorNotificationMutex);
    return ErrorNotification;
}

TMasterEnv MasterEnv;

static void sigcleanup(int) {
    MasterEnv.NeedExit = true;
}

namespace NAsyncJobs {

struct TMaintainCountersJob: TAsyncJobs::IJob {

    void Process(void*) override {
        GetMasterProfiler().Counters->Maintain();
        GetMessageProfiler2().Counters->Maintain();

        AsyncJobs().Add(MakeHolder<TMaintainCountersJob>(), TInstant::Now() + TDuration::Seconds(1));
    }
};

enum EProcessCronEntriesStep {
    PCES_MAIN,
    PCES_ADDITIONAL_FIRST,
    PCES_ADDITIONAL_SECOND
};

static const TDuration ADDITIONAL_STEP_DELAY = TDuration::Seconds(20);
static const TDuration ADDITIONAL_STEP_DELAY_FOR_SECONDS = TDuration::MilliSeconds(1000);

struct TCronEntriesProcessJob: TAsyncJobs::IJob {
    TInstant Now;
    bool CheckTriggeredCron;
    TLockableHandle<TMasterGraph> Graph;
    TLockableHandle<TWorkerPool> Pool;
    bool PerSecond;

    TCronEntriesProcessJob(TInstant now, bool checkTriggeredCron,
                           TLockableHandle<TMasterGraph> graph, TLockableHandle<TWorkerPool> pool, bool perSecond)
        : Now(now)
        , CheckTriggeredCron(checkTriggeredCron)
        , Graph(graph)
        , Pool(pool)
        , PerSecond(perSecond)
    {
    }

    void Process(void*) override {
        TLockableHandle<TMasterGraph>::TWriteLock graph(Graph);
        TLockableHandle<TWorkerPool>::TWriteLock pool(Pool);

        if (TInstant::Now() - Now > TDuration::Seconds(60)) {
            // This is possible if there are tons of cron tasks in graph. See CLUSTERMASTER-76
            LOG("Cron entries processing in behind schedule. Currently processing entries scheduled at " << Now.ToStringUpToSeconds());
        }
        if(!PerSecond)
            graph->ProcessCronEntries(pool, Now, CheckTriggeredCron);
        else
            graph->ProcessCronEntriesPerSecond(pool, Now);
    }
};

struct TCronEntriesScheduleProcessingJob: TAsyncJobs::IJob {
    /*
     * We need Step field to make restart_on_success faster. restart_on_success leads to reset-wait-run cycle
     * (see TMasterGraph::ProcessCronEntries). So we want to run TProcessCronEntriesJob more often than once in
     * a minute but to trigger cron expression only once in a minute and to process other steps of cycle in
     * other runs
     */
    EProcessCronEntriesStep Step;
    TLockableHandle<TMasterGraph> Graph;
    TLockableHandle<TWorkerPool> Pool;

    TCronEntriesScheduleProcessingJob(EProcessCronEntriesStep step, TLockableHandle<TMasterGraph> graph, TLockableHandle<TWorkerPool> pool)
        : Step(step)
        , Graph(graph)
        , Pool(pool)
    {
    }

    void Process(void*) override {
        TInstant now = TInstant::Now();

        // See CLUSTERMASTER-76 to find out why we need another queue here
        AsyncCronProcessingJobs().Add(MakeHolder<TCronEntriesProcessJob>(now, Step == PCES_MAIN, Graph, Pool, false));
        switch (Step) { // see Step field comment to understand why we need this
        case PCES_MAIN: {
            AsyncJobs().Add(MakeHolder<TCronEntriesScheduleProcessingJob>(PCES_ADDITIONAL_FIRST, Graph, Pool), now + ADDITIONAL_STEP_DELAY);
            break;
        }
        case PCES_ADDITIONAL_FIRST: {
            AsyncJobs().Add(MakeHolder<TCronEntriesScheduleProcessingJob>(PCES_ADDITIONAL_SECOND, Graph, Pool), now + ADDITIONAL_STEP_DELAY);
            break;
        }
        case PCES_ADDITIONAL_SECOND: {
            TInstant nextRunOn = TInstant::Seconds((now.Minutes() + 1) * 60); // first second of next minute
            AsyncJobs().Add(MakeHolder<TCronEntriesScheduleProcessingJob>(PCES_MAIN, Graph, Pool), nextRunOn);
            break;
        }
        default:
            Y_FAIL("Unknown step");
        }
    }
};

struct TCronEntriesScheduleProcessingJobPerSecond: TAsyncJobs::IJob {
    TLockableHandle<TMasterGraph> Graph;
    TLockableHandle<TWorkerPool> Pool;

    TCronEntriesScheduleProcessingJobPerSecond(TLockableHandle<TMasterGraph> graph, TLockableHandle<TWorkerPool> pool)
        : Graph(graph)
        , Pool(pool)
    {
    }

    void Process(void*) override {
        TInstant now = TInstant::Now();

        AsyncCronProcessingJobs().Add(MakeHolder<TCronEntriesProcessJob>(now, true, Graph, Pool, true));
        AsyncJobsPerSecond().Add(MakeHolder<TCronEntriesScheduleProcessingJobPerSecond>(Graph, Pool)
                                    , now + ADDITIONAL_STEP_DELAY_FOR_SECONDS);
    }
};

}

int master(int argc, const char** argv) {
    LogLevel::Level() = GetLogLevelFromEnvOrDefault(ELogLevel::Info);
    TLogLevel<LogSubsystem::http>::Level() = GetLogLevelFromEnvOrDefault(ELogLevel::Info);

    NGetoptPb::TGetoptPbSettings settings;
    settings.ConfPathShort = 'g';
    NGetoptPb::TGetoptPb getoptPb(settings);
    NClusterMaster::TMasterOptions masterOptionsPb;
    getoptPb.AddOptions(masterOptionsPb);
    getoptPb.GetOpts().AddLongOption('V', "version", "print svn version and exit").NoArgument().Handler(&PrintSvnVersionAndExit0);
    getoptPb.GetOpts().AddHelpOption('?');

    TString errorMsg;
    bool ret = getoptPb.ParseArgs(argc, argv, masterOptionsPb, errorMsg);
    if (ret) {
        Cerr << "Using settings:\n"
             << "====================\n";
        getoptPb.DumpMsg(masterOptionsPb, Cerr);
        Cerr << "====================\n";
    } else {
        Cerr << errorMsg;
    }

    MasterOptions.Fill(masterOptionsPb);
    try {
        MasterOptions.Check();
    } catch (const TMasterOptions::TInvalid& e) {
        Cerr << "Invalid options: " << e.what() << Endl;
        getoptPb.GetOpts().PrintUsage(argv[0] ? argv[0] : "master");
        exit(EXIT_FAILURE);
    }

    if (MasterOptions.DebugLogOutput)
        LogLevel::Level() = Debug;

    if (!MasterOptions.YtProxy.empty()) {
        NYT::NApi::IClientPtr client = NYT::NApi::CreateClient(
            MasterOptions.YtProxy,
            MasterOptions.YtUser,
            ReadYtToken(MasterOptions.YtTokenPath)
        );
        MasterOptions.YtStorage.ConstructInPlace(
            MasterOptions.YtVarDir,
            "master_state",
            client,
            MasterOptions.BackoffPeriodMin,
            MasterOptions.BackoffPeriodMax
        );
    }

    // get own hostname
    {
        auto hostname = HostName();
        auto hostnameSb = TStringBuf(hostname);
        hostnameSb.ChopSuffix(".search.yandex.net");
        MasterEnv.SelfHostname = hostnameSb;
    }

#ifndef _win_

    // set resolver retry
    _res.retry = 3;

    InitializeDaemonGeneric(MasterOptions.LogFilePath, MasterOptions.PidFilePath, MasterOptions.DoNotDaemonize);

    struct sigaction sa;
    Zero(sa);
    sa.sa_handler = sigcleanup;
    SigFillSet(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    if (MasterOptions.DoNotDaemonize)
        sigaction(SIGHUP, &sa, nullptr);

    // block all signals in the main thread to have them blocked in all newly
    // created threads
    THolder<TSignalBlockingGuard> bg;
    bg.Reset(new TSignalBlockingGuard);

    // load configs
    LOG("Loading lists");

    TMasterListManager listManager;

    listManager.LoadHostcfgs(MasterOptions.HostCfgPaths);

    if (!MasterOptions.CustomWorkersListPath.empty())
        listManager.LoadHostlist(MasterOptions.CustomWorkersListPath, MasterOptions.VarDirPath);

    TPollerEvents pollerEvents;

    // construct pool
    TLockableHandle<TWorkerPool> workerPool(new TWorkerPool(&listManager, &pollerEvents));

    // create graph
    TLockableHandle<TMasterGraph> targetGraph(new TMasterGraph(&listManager));

    LOG("Starting asynchronous jobs thread pool");
    AsyncJobs().Start();
    AsyncMailJobs().Start();
    AsyncSmsJobs().Start();
    AsyncJugglerJobs().Start();
    AsyncTelegramJobs().Start();
    AsyncJNSChannelJobs().Start();
    AsyncCronProcessingJobs().Start();
    if (MasterOptions.PerSecondTargetsEnabled)
        AsyncJobsPerSecond().Start();

    // start HTTP servers
    LOG("Starting HTTP servers");

    InitNetworkSubSystem();

    TMasterHttpServer serv(workerPool, targetGraph, THttpServerOptions(MasterOptions.HTTPPort), false);

    serv.RestartRequestThreads(4, 100);
    serv.Start();

    THolder<TMasterHttpServer> roServ;
    if (MasterOptions.ROHTTPPort) {
        LOG("Starting read-only HTTP server");
        roServ.Reset(new TMasterHttpServer(workerPool, targetGraph, THttpServerOptions(MasterOptions.ROHTTPPort), true));
        roServ->RestartRequestThreads(4, 100);
        roServ->Start();
    }

    // unblock all signals after all necessary threads are created
    bg.Destroy();

    LOG("Starting main loop");
    SetCurrentThreadName("main thread");

    TInstant lastPoke;
    TInstant lastStatsUpdate;
    TInstant lastFailureEmailsDispatch;

    AsyncJobs().Add(THolder(new NAsyncJobs::TMaintainCountersJob()));
    AsyncJobs().Add(THolder(new NAsyncJobs::TCronEntriesScheduleProcessingJob(NAsyncJobs::PCES_MAIN, targetGraph, workerPool)),
            TInstant::Seconds((TInstant::Now().Minutes() + 1) * 60));
    if (MasterOptions.PerSecondTargetsEnabled)
        AsyncJobsPerSecond().Add(THolder(new NAsyncJobs::TCronEntriesScheduleProcessingJobPerSecond(targetGraph, workerPool)),
                                 TInstant::MilliSeconds((TInstant::Now().Seconds() + 1) * 1000));

    // mainloop
    while (!MasterEnv.NeedExit) {
        PROFILE_ACQUIRE(ID_WHOLE_MAIN_LOOP_TIME)

        if (MasterEnv.NeedReloadScript) {
            PROFILE_ACQUIRE(ID_RELOAD_SCRIPT_TIME)

            LOG("Loading script");

            MasterEnv.NeedReloadScript = false;

            MasterEnv.ClearErrorNotification();

            try {
                listManager.Reset();

                listManager.LoadHostcfgs(MasterOptions.HostCfgPaths);

                if (!MasterOptions.CustomWorkersListPath.empty())
                    listManager.LoadHostlist(MasterOptions.CustomWorkersListPath, MasterOptions.VarDirPath);

                {
                    TLockableHandle<TMasterGraph>::TReloadScriptWriteLock graph(targetGraph);
                    TLockableHandle<TWorkerPool>::TReloadScriptWriteLock pool(workerPool);

                    graph->ClearPendingPokeUpdatesAndFailureNotificationsDispatch();

                    THolder<IOutputStream> configOutput;
                    if (MasterOptions.VarDirPath) {
                        configOutput.Reset(new TOFStream(TFsPath(MasterOptions.VarDirPath).Child("script.dump").GetPath()));
                    }
                    graph->LoadConfig(TMasterConfigSource(TFsPath(MasterOptions.ScriptPath)), configOutput.Get());
                    /* sync variables in all stores */
                    graph->SaveVariables();
                    graph->ConfigureWorkers(pool);

                    if (MasterOptions.DumpGraphOnReload) {
                        if(!MasterOptions.VarDirPath) {
                            ERRORLOG("Cannot dump graph: var directory isn't specified");
                        } else {
                            TFsPath dumpTxt = TFsPath(MasterOptions.VarDirPath).Child("dump.master.txt");
                            TFsPath dumpTxtTmp = dumpTxt.GetPath() + ".tmp";

                            LOG("Dumping graph state to " << dumpTxt);

                            dumpTxt.Parent().MkDirs();

                            {
                                TFixedBufferFileOutput os(dumpTxtTmp);
                                TPrinter printer(os);
                                graph->DumpState(printer);
                                os.Flush();
                            }

                            dumpTxtTmp.RenameTo(dumpTxt);
                        }
                    }


                    if (!MasterOptions.DisableGraphviz)
                        AsyncJobs().Add(THolder(new NAsyncJobs::TGenerateGraphImageJob(targetGraph)));
                }

                LOG("Script loaded");

            } catch (...) {
                const auto currentExceptionMessage = CurrentExceptionMessage();
                ERRORLOG("Script loading error: " << currentExceptionMessage);
                Cerr << currentExceptionMessage;
                if (MasterOptions.Panic) {
                    throw;
                }
                MasterEnv.SetErrorNotification(TString("Script loading error: ") + currentExceptionMessage);
            }

            PROFILE_RELEASE(ID_RELOAD_SCRIPT_TIME)
        }

        TInstant now = TInstant::Now();

        PROFILE_ACQUIRE(ID_PROCESS_CONNECTIONS_TIME)
        TLockableHandle<TWorkerPool>::TWriteLock(workerPool)->ProcessConnections(now);
        PROFILE_RELEASE(ID_PROCESS_CONNECTIONS_TIME)

        PROFILE_ACQUIRE(ID_NETWORK_TIME)
        if (pollerEvents.Receive(TDuration::Seconds(MasterOptions.NetworkHeartbeat).ToDeadLine(now), now)) {
            TLockableHandle<TMasterGraph>::TWriteLock graph(targetGraph);
            TLockableHandle<TWorkerPool>::TWriteLock pool(workerPool);

            PROFILE_ACQUIRE(ID_PROCESS_EVENTS_TIME)
            pool->ProcessEvents(graph);
            PROFILE_RELEASE(ID_PROCESS_EVENTS_TIME)
        }
        PROFILE_RELEASE(ID_NETWORK_TIME)

        if (now - lastPoke > TDuration::Seconds(MasterOptions.PokeHeartbeat)) {
            TLockableHandle<TMasterGraph>::TWriteLock graph(targetGraph);
            TLockableHandle<TWorkerPool>::TWriteLock pool(workerPool);

            PROFILE_ACQUIRE(ID_ASYNC_POKES_TIME)
            graph->ProcessPokes(pool);
            PROFILE_RELEASE(ID_ASYNC_POKES_TIME)

            lastPoke = now;
        }

        if (now - lastStatsUpdate > TDuration::Seconds(15)) {
            TLockableHandle<TMasterGraph>::TWriteLock graph(targetGraph);
            TLockableHandle<TWorkerPool>::TWriteLock pool(workerPool);

            graph->UpdateTargetsStats();

            lastStatsUpdate = now;
        }

        if (now - lastFailureEmailsDispatch > TDuration::Minutes(1)) {
            TLockableHandle<TMasterGraph>::TWriteLock graph(targetGraph);
            TLockableHandle<TWorkerPool>::TWriteLock pool(workerPool);

            PROFILE_ACQUIRE(ID_FAILURE_EMAILS_DISPATCH)
            graph->DispatchFailureNotifications();
            PROFILE_RELEASE(ID_FAILURE_EMAILS_DISPATCH)
            lastFailureEmailsDispatch = now;
        }

        GetMasterProfiler().Counters->AddOne(TMasterProfiler::ID_MAIN_LOOP_ITERATIONS);

        PROFILE_RELEASE(ID_WHOLE_MAIN_LOOP_TIME)
    }

#endif // !_win_

    LOG("Stopping async jobs threads..");
    AsyncJobs().Stop();
    AsyncMailJobs().Stop();
    AsyncSmsJobs().Stop();
    AsyncJugglerJobs().Stop();
    AsyncTelegramJobs().Stop();
    AsyncJNSChannelJobs().Stop();
    AsyncJobsPerSecond().Stop();

    LOG("Stopping http server..");
    serv.Stop();
    if (roServ.Get()) {
        roServ->Stop();
    }
    MasterOptions.YtStorage.Clear(); // destroy yt client before NYT::Shutdown()

    LOG("Normal exit");
    NYT::Shutdown();

    return 0;
}

int MasterMain(int argc, const char* argv[]) {
    return master(argc, argv);
}
