#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/tests_data.h>
#include <util/system/shellcommand.h>
#include <util/stream/file.h>
#include <library/cpp/logger/global/global.h>
#include <kernel/daemon/common/time_guard.h>
#include <kernel/daemon/common_modules/parent_existence/parent_existence_module.h>
#include <search/fetcher/fetcher.h>
#include <util/system/fs.h>
#include "daemon/config.h"
#include <util/system/getpid.h>
#include <library/cpp/json/json_reader.h>
#include <util/thread/pool.h>
#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/svnversion/svnversion.h>

class IProcessWatcher {
public:

    virtual ~IProcessWatcher() {

    }
    virtual void Start() = 0;
    virtual void Stop() = 0;
};

class TSimpleProcessWatcher: public IProcessWatcher {
protected:
    TAtomic StopFlag = 0;
    THolder<TShellCommand> ShellCommand;
    TThreadPool Runner;
private:
    TString Command;
    TRWMutex ShellCommandMutex;
    TAtomic StopsCounter = 0;
    TDuration Discretization = TDuration::Seconds(1);

    class TWatcher: public IObjectInQueue {
    private:
        TSimpleProcessWatcher* Owner;
    public:

        TWatcher(TSimpleProcessWatcher* owner)
            : Owner(owner)
        {

        }

        void Process(void*) override {
            if (!AtomicGet(Owner->StopFlag)) {
                {
                    TWriteGuard rg(Owner->ShellCommandMutex);
                    Owner->ShellCommand.Reset(new TShellCommand(Owner->Command));
                }
                INFO_LOG << "Run started: " << Owner->Command << Endl;
                TReadGuard rg(Owner->ShellCommandMutex);
                CHECK_WITH_LOG(Owner->ShellCommand);
                if (!AtomicGet(Owner->StopFlag)) {
                    Owner->ShellCommand->Run();
                    AtomicIncrement(Owner->StopsCounter);
                    INFO_LOG << "Run finished: " << Owner->Command << Endl;
                    if (!AtomicGet(Owner->StopFlag)) {
                        Owner->Runner.SafeAddAndOwn(MakeHolder<TWatcher>(Owner));
                    }
                }
            }
        }
    };

public:

    TSimpleProcessWatcher& SetDiscretization(const TDuration& dur) {
        Discretization = dur;
        return *this;
    }

    ui32 GetCounter() const {
        return AtomicGet(StopsCounter);
    }

    TProcessId GetPid() {
        TProcessId parentPid = 0;
        while (parentPid == 0) {
            TReadGuard rg(ShellCommandMutex);
            CHECK_WITH_LOG(ShellCommand);
            parentPid =  ShellCommand->GetPid();
        }
        return parentPid;
    }

    ~TSimpleProcessWatcher() {
    }

    TSimpleProcessWatcher(const TString& cmd)
        : Command(cmd)
    {
        Runner.Start(1);
    }

    void Start() override {
        TTimeGuard tg("Watcher start");
        TWriteGuard rg(ShellCommandMutex);
        CHECK_WITH_LOG(!ShellCommand);
        ShellCommand.Reset(new TShellCommand(Command));
        Runner.SafeAddAndOwn(MakeHolder<TWatcher>(this));
    }

    void Stop() override {
        TTimeGuard tg("Watcher stop");
        AtomicSet(StopFlag, 1);
        ShellCommand->Terminate();
        Runner.Stop();
        ShellCommand.Destroy();

    }

};

class TControllerProcessWatcher: public TSimpleProcessWatcher {
private:
    ui16 Port;

public:
    TControllerProcessWatcher(const ui16 port, const TString& command)
        : TSimpleProcessWatcher(command)
        , Port(port)
    {
    }

    bool SendCommand(const TString& command, NJson::TJsonValue& result, const TDuration timeout) const {
        TEventLogPtr evLog = nullptr;
        THttpFetcher fetcher(evLog, "11");
        fetcher.AddRequest("127.0.0.1", Port, command.c_str(), timeout);
        fetcher.Run(THttpFetcher::TAbortOnSuccess());
        if (!fetcher.GetRequestResult(0)) {
            return false;
        }
        const TString jsonStr(fetcher.GetRequestResult(0)->Content().data, fetcher.GetRequestResult(0)->Content().size);
        NJson::TJsonValue json;
        CHECK_WITH_LOG(NJson::ReadJsonFastTree(jsonStr, &json));
        result = json;
        return true;
    }

    TString GetStatus(const TDuration timeout = TDuration::Seconds(10)) const {
        NJson::TJsonValue json;
        CHECK_WITH_LOG(SendCommand("/?command=get_info_server", json, timeout));
        NJson::TJsonValue* controllerStatus = json.GetValueByPath("result.controller_status");
        NJson::TJsonValue* serverStatusGlobal = json.GetValueByPath("result.server_status_global.state");
        NJson::TJsonValue* serverStatusInfo = json.GetValueByPath("result.server_status_global.info");
        CHECK_WITH_LOG(controllerStatus && serverStatusGlobal && serverStatusInfo);
        return controllerStatus->GetStringRobust() + "-" + serverStatusGlobal->GetStringRobust() + "-" + serverStatusInfo->GetStringRobust();
    }

    bool WaitStatus(const TString& status, const TDuration timeout = TDuration::Seconds(100), bool controllerStatus = true, const ui32 count = 1) const {
        TInstant start = Now();
        ui32 currentCount = 0;
        TString currentStatus;
        TString path = controllerStatus ? "result.controller_status" : "result.server_status_global.state";
        while (Now() - start < timeout && currentCount < count) {
            if (GetInfoServerField(path, currentStatus, Now() - start) && currentStatus == status) {
                currentCount++;
            }
            Sleep(TDuration::Seconds(1));
        }
        return (currentCount == count);
    }

    void Restart() {
        TTimeGuard tg("Watcher restart");

        TEventLogPtr evLog = nullptr;
        THttpFetcher fetcher(evLog, "11");
        fetcher.AddRequest("127.0.0.1", Port, "/?command=restart&reread_config=true", TDuration::Seconds(10));
        fetcher.Run(THttpFetcher::TAbortOnSuccess());
    }

    void Shutdown() {
        TTimeGuard tg("Watcher shutdown");

        TEventLogPtr evLog = nullptr;
        THttpFetcher fetcher(evLog, "11");
        fetcher.AddRequest("127.0.0.1", Port, "/?command=shutdown", TDuration::Seconds(10));
        fetcher.Run(THttpFetcher::TAbortOnSuccess());
    }

    void Interrupt() {
        TTimeGuard tg("Watcher interrupt");
        AtomicSet(StopFlag, 1);
        TProcessId processPid = ShellCommand->GetPid();
        ShellCommand->Terminate();
        TInstant start = Now();
        while (Now() - start < TDuration::Seconds(20) && NParentExistenceChecker::TParentExistenceChecker::CheckExistence(processPid)) {
            Sleep(TDuration::MilliSeconds(50));
        }
        CHECK_WITH_LOG(Now() - start < TDuration::Seconds(20));
        Runner.Stop();
        ShellCommand.Destroy();
    }

    void Stop() override {
        TTimeGuard tg("Watcher stop");
        AtomicSet(StopFlag, 1);

        TEventLogPtr evLog = nullptr;
        THttpFetcher fetcher(evLog, "11");
        fetcher.AddRequest("127.0.0.1", Port, "/?command=shutdown", TDuration::Seconds(10));
        fetcher.Run(THttpFetcher::TAbortOnSuccess());

        Runner.Stop();
        ShellCommand.Destroy();
    }

private:

    bool GetInfoServerField(const TString& path, TString& result, const TDuration timeout) const {
        NJson::TJsonValue json;
        if (SendCommand("/?command=get_info_server", json, timeout)) {
            NJson::TJsonValue* controllerStatus = json.GetValueByPath(path);
            if (controllerStatus) {
                result = controllerStatus->GetStringRobust();
                return true;
            }
        }
        return false;
    }
};

namespace {
    TString GetDaemonConfig(const ui16 port, const bool enabled) {
        TStringStream ss;
        ss << "<DaemonConfig>" << Endl;
        ss << "LoggerType: 111" << Endl;
        ss << "LogLevel: 7" << Endl;
        ss << "<Controller>" << Endl;
        ss << "ConfigsControl: true" << Endl;
        ss << "Host:127.0.0.1" << Endl;
        ss << "Port:" << port << Endl;
        ss << "Threads:" << 1 << Endl;
        ss << "StartServer:" << enabled << Endl;
        ss << "</Controller>" << Endl;
        ss << "</DaemonConfig>" << Endl;
        return ss.Str();
    }

    TString MakeParentConfig(const TProcessId parentPid = GetPID(), const TDuration interval = TDuration::Seconds(10)) {
        TStringStream ss;
        ss << "<ModulesConfig>" << Endl;
        ss << "<ParentExistenceChecker>" << Endl;
        ss << "Enabled:" << true << Endl;
        ss << "ParentPid: " << parentPid << Endl;
        ss << "CheckInterval:" << interval.Seconds() << Endl;
        ss << "</ParentExistenceChecker>" << Endl;
        ss << "</ModulesConfig>" << Endl;
        return ss.Str();
    }

    void MakeConfigImpl(const ui16 port, const TString& correctness, const bool enabled) {
        TFileOutput fo("config.simple");
        fo << GetDaemonConfig(port, enabled) << Endl;
        fo << "<Server>" << Endl;
        fo << "State: " << correctness << Endl;
        fo << "</Server>" << Endl;
        fo << MakeParentConfig() << Endl;
    }

    void MakeIncorrectConfig(const ui16 port, const bool enabled = true) {
        MakeConfigImpl(port, "FAIL", enabled);
    }

    void MakeBadIndexConfig(const ui16 port, const bool enabled = true) {
        MakeConfigImpl(port, "FAILED_INDEX", enabled);
    }

    void MakeCorrectConfig(const ui16 port) {
        MakeConfigImpl(port, "CORRECT", true);
    }
}

const TString CorrectStatusString = "Active-OK-";

Y_UNIT_TEST_SUITE(Daemon) {
    Y_UNIT_TEST(ControllerRestoreConfig) {
        const TString current = GetWorkPath() + "/test" + ToString(Now().MicroSeconds() + rand()) + "/";
        const TString configs = current + "/configs/";
        const TString state = current + "/state/";
        TFsPath(current).MkDirs();
        TFsPath(configs).MkDirs();
        TFsPath(state).MkDirs();
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        TConfig globalConfig(GetPID());
        {
            TDaemonConfig& config = globalConfig.MutableDaemonConfig();
            config.SetLoggerType(current + "/log");
            config.SetLogLevel(7);
            config.SetStdErr(current + "/log_err");
            config.SetStdOut(current + "/log_out");
            config.MutableController().SetHost("127.0.0.1");
            config.MutableController().SetPort(port);
            config.MutableController().SetThreads(10);
            config.MutableController().ConfigsControl = true;
            config.MutableController().StateRoot = state;
            config.MutableController().ConfigsRoot = configs;
        }
        {
            globalConfig.SetState("OK");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }

        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester " + configs + "/config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0) << watcher.GetCounter();

        {
            globalConfig.SetState("FAIL");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == "Active-RestoredConfig-Configs restored") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();
        watcher.Shutdown();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 2) << watcher.GetCounter();

        {
            globalConfig.SetState("INCORRECT_FAIL");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == "Active-RestoredConfig-Configs restored") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 3) << watcher.GetCounter();
        watcher.Shutdown();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 4) << watcher.GetCounter();

        {
            globalConfig.SetState("FAIL_EXCEPTION");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == "Active-RestoredConfig-Configs restored") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 5) << watcher.GetCounter();
        watcher.Shutdown();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 6) << watcher.GetCounter();

        {
            globalConfig.SetState("${BrokenLua}");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == "Active-RestoredConfig-Configs restored") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 7) << watcher.GetCounter();
        watcher.Shutdown();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 8) << watcher.GetCounter();
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 9);
    }

    Y_UNIT_TEST(UnknownRestartsCounter) {
        const TString current = GetWorkPath() + "/test" + ToString(Now().MicroSeconds() + rand()) + "/";
        const TString configs = current + "/configs/";
        const TString state = current + "/state/";
        TFsPath(current).MkDirs();
        TFsPath(configs).MkDirs();
        TFsPath(state).MkDirs();
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        TConfig globalConfig(GetPID());
        {
            TDaemonConfig& config = globalConfig.MutableDaemonConfig();
            config.SetLoggerType(current + "/log");
            config.SetLogLevel(7);
            config.SetStdErr(current + "/log_err");
            config.SetStdOut(current + "/log_out");
            config.MutableController().SetHost("127.0.0.1");
            config.MutableController().SetPort(port);
            config.MutableController().SetThreads(10);
            config.MutableController().ConfigsControl = true;
            config.MutableController().StateRoot = state;
            config.MutableController().ConfigsRoot = configs;
        }
        {
            globalConfig.SetState("FAILED_SERVER");
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
        }

        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester " + configs + "/config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable", TDuration::Seconds(100), true));
        watcher.Stop();
    }

    Y_UNIT_TEST(ControllerStartIncorrectLastSuccess) {
        const TString current = GetWorkPath() + "/test" + ToString(Now().MicroSeconds() + rand()) + "/";
        const TString configs = current + "/configs/";
        const TString state = current + "/state/";
        TFsPath(current).MkDirs();
        TFsPath(configs).MkDirs();
        TFsPath(state).MkDirs();
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        {
            TConfig globalConfig(GetPID());
            globalConfig.SetState("FAIL");
            TDaemonConfig& config = globalConfig.MutableDaemonConfig();
            config.SetLoggerType(current + "/log");
            config.SetLogLevel(7);
            config.MutableController().SetHost("127.0.0.1");
            config.MutableController().SetPort(port);
            config.MutableController().SetThreads(10);
            config.MutableController().ConfigsControl = true;
            config.MutableController().StateRoot = state;
            config.MutableController().ConfigsRoot = configs;
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
            {
                TFsPath path(state + "/" + ToString(port) + "/configurations/aaa/configs");
                path.MkDirs();
                TFileOutput foSuccess(path / "config.simple");
                foSuccess << globalConfig.ToString() << Endl;
            }
            {
                TFsPath path(state + "/" + ToString(port) + "/configurations/GLOBAL");
                path.MkDirs();
                TFileOutput foSuccess(path / "status");
                foSuccess << "SuccessConfiguration: \"aaa\"";
            }
        }

        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester " + configs + "/config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedConfig-Incorrect state") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 2) << watcher.GetCounter();
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedConfig-Incorrect state") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 3) << watcher.GetCounter();
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 4);
    }

    Y_UNIT_TEST(ClearUselessConfigurations) {
        const TString current = GetWorkPath() + "/test" + ToString(Now().MicroSeconds() + rand()) + "/";
        const TString configs = current + "/configs/";
        const TString state = current + "/state/";
        TFsPath(current).MkDirs();
        TFsPath(configs).MkDirs();
        TFsPath(state).MkDirs();
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        {
            TConfig globalConfig(GetPID());
            globalConfig.SetState("OK");
            TDaemonConfig& config = globalConfig.MutableDaemonConfig();
            config.SetLoggerType(current + "/log");
            config.SetLogLevel(7);
            config.MutableController().SetHost("127.0.0.1");
            config.MutableController().SetPort(port);
            config.MutableController().SetThreads(10);
            config.MutableController().ConfigsControl = true;
            config.MutableController().StateRoot = state;
            config.MutableController().ConfigsRoot = configs;
            TFileOutput fo(configs + "/config.simple");
            fo << globalConfig.ToString();
            for (ui32 i = 0; i < 20; ++i) {
                TFsPath path(state + "/" + ToString(port) + "/configurations/" + ToString(i) + "-1/configs");
                path.MkDirs();
                TFileOutput foSuccess(path / "config.simple");
                foSuccess << globalConfig.ToString() << Endl;
            }
            for (ui32 i = 0; i < 5; ++i) {
                TFsPath path(state + "/" + ToString(port) + "/configurations/" + ToString(i) + "-3/configs");
                path.MkDirs();
                TFileOutput foSuccess(path / "config.simple");
                foSuccess << globalConfig.ToString() << Endl;
            }
            for (ui32 i = 0; i < 6; ++i) {
                TFsPath path(state + "/" + ToString(port) + "/configurations/" + ToString(i) + "-" + ToString(GetProgramSvnRevision()) + "/configs");
                path.MkDirs();
                TFileOutput foSuccess(path / "config.simple");
                foSuccess << globalConfig.ToString() << Endl;
            }
        }

        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester " + configs + "/config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("OK", TDuration::Seconds(100), false));
        TFsPath path(state + "/" + ToString(port) + "/configurations");
        TVector<TFsPath> configurations;
        path.List(configurations);
        CHECK_WITH_LOG(configurations.size() == 13);
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 1);
    }

    Y_UNIT_TEST(ControllerStartIncorrectServerInactive) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeIncorrectConfig(port, false);
        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("Stopped"));
        CHECK_WITH_LOG(watcher.GetStatus() == "Stopped-OK-") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0) << watcher.GetCounter();
        watcher.Restart();
        CHECK_WITH_LOG(watcher.GetStatus() == "Stopped-OK-") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0);
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 1);
    }

    Y_UNIT_TEST(ControllerStartCorrect) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeCorrectConfig(port);
        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0) << watcher.GetCounter();
        watcher.Restart();
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0);
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 1);
    }

    Y_UNIT_TEST(ControllerStartIncorrect) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeIncorrectConfig(port);
        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedConfig-Incorrect state") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedConfig-Incorrect state") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 2);
        MakeCorrectConfig(port);
        watcher.Restart();
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 2);
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 3);
    }

    Y_UNIT_TEST(ParentExistence) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;

        MakeCorrectConfig(manager.GetPort());
        TSimpleProcessWatcher mainWatcher(binariesPath + "/daemon/daemon_tester config.simple");
        mainWatcher.Start();

        const ui16 port = manager.GetPort();

        TConfig globalConfig(mainWatcher.GetPid(), TDuration::Seconds(1));
        globalConfig.SetState("OK");
        TDaemonConfig& config = globalConfig.MutableDaemonConfig();
        config.SetLoggerType("11");
        config.SetLogLevel(7);
        config.MutableController().SetHost("127.0.0.1");
        config.MutableController().SetPort(port);
        config.MutableController().SetThreads(1);
        config.MutableController().ConfigsControl = true;
        config.MutableController().MaxRestartAttemptions = 0;

        {
            TFileOutput fo("child_config.simple");
            fo << globalConfig.ToString();
        }
        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester child_config.simple");


        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("OK", TDuration::Seconds(100), false));
        CHECK_WITH_LOG(watcher.GetCounter() == 0) << watcher.GetCounter() << Endl;
        mainWatcher.Stop();
        CHECK_WITH_LOG(watcher.WaitStatus("UnknownError", TDuration::Seconds(100), false, 5));
        CHECK_WITH_LOG(watcher.GetCounter() == 2) << watcher.GetCounter() << Endl;
        watcher.Interrupt();
    }

    Y_UNIT_TEST(StartIncorrect) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeIncorrectConfig(port);
        TSimpleProcessWatcher watcher(binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();
        sleep(10);
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 2);
    }

    Y_UNIT_TEST(ClearConfigurations) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeIncorrectConfig(port);
        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedConfig-Incorrect state") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();
        NJson::TJsonValue json;
        CHECK_WITH_LOG(watcher.SendCommand("/?command=clear_configurations", json, TDuration::Seconds(10)));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-OK-") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();
        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 2);
    }

    Y_UNIT_TEST(FailedIndex) {
        const TString binariesPath = BinaryPath("kernel/daemon/ut");
        TPortManager manager;
        const ui16 port = manager.GetPort();
        MakeCorrectConfig(port);

        TControllerProcessWatcher watcher(port, binariesPath + "/daemon/daemon_tester config.simple");
        watcher.Start();

        CHECK_WITH_LOG(watcher.WaitStatus("Active"));
        CHECK_WITH_LOG(watcher.GetStatus() == CorrectStatusString) << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 0) << watcher.GetCounter();

        MakeBadIndexConfig(port);
        watcher.Restart();
        CHECK_WITH_LOG(watcher.WaitStatus("NotRunnable"));
        CHECK_WITH_LOG(watcher.GetStatus() == "NotRunnable-FailedIndex-Incorrect index") << watcher.GetStatus();
        CHECK_WITH_LOG(watcher.GetCounter() == 1) << watcher.GetCounter();

        watcher.Stop();
        CHECK_WITH_LOG(watcher.GetCounter() == 2) << watcher.GetCounter();
    }
}
