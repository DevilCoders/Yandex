#pragma once

#include "messages.h"
#include "server_description.h"
#include "signal_handler.h"

#include <kernel/daemon/config/options.h>

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/system/daemon.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

void CreatePidFile(const TString& pidFile);

template <class TServer, class Controller = typename TServer::TController, class ServerDescriptor = TServerDescriptor<TServer>>
class TDaemon : TNonCopyable {
private:
    using TControllerClass = Controller;
    using TServerDescriptorClass = ServerDescriptor;
    using TSelf = TDaemon<TServer, TControllerClass>;
    Y_DECLARE_SINGLETON_FRIEND()

private:
    THolder<TControllerClass> Server;
    THolder<NUtil::TSigHandlerThread> SigHandlerThread;
    TMutex Mutex;
    bool Stopped = false;

private:
    void StopServer(int signal) {
        TGuard<TMutex> guard(Mutex);
        if (!Stopped) {
            Stopped = true;
            INFO_LOG << "Signal caught, stopping server" << Endl;
            Server->Stop(((signal == SIGINT) || (signal == SIGTERM)) ? Max<ui32>() : 0);
        } else {
            INFO_LOG << "Server stop already is in progress, ignoring signal" << Endl;
        }
    }

private:
    static void AbortOnSignal(int sig) {
        FormatBackTrace(&SINGLETON_GENERIC_LOG_CHECKED(TGlobalLog, TRTYLogPreprocessor, TLOG_CRIT, "CRITICAL_INFO"));
        signal(sig, SIG_DFL);
        Y_FAIL("Aborting on signal %d", sig);
    }

    static void AbortOnException() {
        FormatBackTrace(&SINGLETON_GENERIC_LOG_CHECKED(TGlobalLog, TRTYLogPreprocessor, TLOG_CRIT, "CRITICAL_INFO"));
        Y_FAIL("Aborting on exception %s", CurrentExceptionMessage().c_str());
    }

    static void StopServerStatic(int signal) {
        Singleton<TDaemon<TServer, TControllerClass> >()->StopServer(signal);
    }

    static void LogDisconnection(int /*signal*/) {
        WARNING_LOG << "SIGPIPE handled. Client disconnected." << Endl;
    }

    static void ReopenLog(int) {
        DEBUG_LOG << "start loggers reopen...";
        Singleton<TDaemon<TServer, TControllerClass> >()->Server->GetConfig()->GetDaemonConfig().ReopenLog();
        TLogBackend::ReopenAllBackends();
        TMessageReopenLogs messReopen;
        SendGlobalMessage(messReopen);
        DEBUG_LOG << "loggers reopen finished";
    }

public:
    class IAdditionOptionsParser {
    public:
        virtual TString GetUsage() const {
            return TString();
        };
        virtual TString GetAdditionOptionsString() = 0;
        virtual bool ParseOption(int optlet, const TString& arg, TConfigPatcher& preprocessor) = 0;
        virtual void OnBeforeParsing(TConfigPatcher& /*preprocessor*/) {}
        virtual ~IAdditionOptionsParser() {}
    };

    int Run(int argc, char* argv[], bool printConfig = false) {
        InitGlobalLog2Console(TLOG_DEBUG);
        try {
            TDaemonOptions options;
            options.Parse(argc, argv);
            options.GetPreprocessor().SetStrict(false);
            TString configText = options.RunPreprocessor();
            options.GetPreprocessor().SetStrict(true);

            TDaemonConfig daemonConfig(configText.data(), true);
            INFO_LOG << "Daemon config parsed" << Endl;
            if (!options.GetValidateOnly() && daemonConfig.StartAsDaemon()) {
                NDaemonMaker::MakeMeDaemon(NDaemonMaker::closeStdIoOnly, NDaemonMaker::openNone, NDaemonMaker::chdirNone);
                daemonConfig.ReopenLog();
                CreatePidFile(daemonConfig.GetPidFileName());
            }
            TServerConfigConstructorParams configParams(options, configText);
            if (printConfig) {
                INFO_LOG << "Original config: " << Endl << configText << Endl << Endl;
            }

            TServerDescriptorClass sd;
            Server = MakeHolder<TControllerClass>(configParams, sd);

            if (options.GetValidateOnly()) {
                TAtomicSharedPtr<IServerConfig> config(sd.CreateConfig(configParams));
                Server->CheckConfig(*config, true);
                Server.Destroy();
                return EXIT_SUCCESS;
            }
            {
                signal(SIGSEGV, AbortOnSignal);
                std::set_terminate(AbortOnException);
            }
            // start signal handling on a dedicated thread
            {
                TVector<NUtil::TSigHandler> sigHandlers;
                sigHandlers.push_back(NUtil::TSigHandler(SIGINT,  StopServerStatic));
                sigHandlers.push_back(NUtil::TSigHandler(SIGTERM, StopServerStatic));
                sigHandlers.push_back(NUtil::TSigHandler(SIGUSR1, StopServerStatic));
                sigHandlers.push_back(NUtil::TSigHandler(SIGHUP, ReopenLog));
                sigHandlers.push_back(NUtil::TSigHandler(SIGPIPE, LogDisconnection));
                SigHandlerThread.Reset(new NUtil::TSigHandlerThread(sigHandlers));
            }
            SigHandlerThread->DelegateSignals();
            INFO_LOG << "Starting server" << Endl;
            Server->Run();
            Server->WaitStopped();
            // Stop signal handling
            SigHandlerThread->Stop();
            SigHandlerThread.Destroy();
            TGuard<TMutex> guard(Mutex);
            Server.Destroy();
            NMessenger::TGlobalMediator::CheckList();
            NMessenger::TGlobalMediator::CheckEmpty();
            INFO_LOG << "Server stopped" << Endl;
            return EXIT_SUCCESS;
        } catch (...) {
            FATAL_LOG << "Daemon running failed: " << CurrentExceptionMessage() << Endl;
            return EXIT_FAILURE;
        }
    }
};
