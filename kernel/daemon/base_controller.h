#pragma once

#include "base_http_client.h"
#include "context.h"
#include "environment_manager.h"
#include "server.h"

#include <kernel/daemon/async/async_task_executor.h>
#include <kernel/daemon/common/common.h>
#include <kernel/daemon/common/guarded_ptr.h>
#include <kernel/daemon/config/config_constructor.h>
#include <kernel/daemon/protos/status.pb.h>

#include <library/cpp/mediator/messenger.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/misc/httpreqdata.h>

#include <util/system/event.h>

class TBomb;

namespace NController {

    class TController
        : public THttpServer
        , public THttpServer::ICallBack
        , public IMessageProcessor {
    public:
        using TGuardServer = TGuardedPtr<TExtendedServer, TRWMutex, TReadGuard>;
        using TGuardConfig = TGuardedPtr<const IServerConfig, TRWMutex, TReadGuard>;
        using TGuardLog = TGuardedPtr<TLog, TRWMutex, TReadGuard>;

        class TClient: public TBaseHttpClient {
        public:
            TClient(TController& owner);

            virtual bool Reply(void* ThreadSpecificResource) override;
            virtual void ProcessTass(IOutputStream& out) const override;

            inline const TServerRequestData& GetRD() const {
                return RD;
            }
            inline TBlob& PostBuffer() {
                return Buf;
            }

        private:
            void LogRequest();

        protected:
            TController& Owner;
        };

        class TCommandProcessor: public TAsyncTaskExecutor::TTask {
        public:
            typedef THolder<TCommandProcessor> TPtr;
            typedef NObjectFactory::TObjectFactory<TCommandProcessor, TString> TFactory;

        public:
            TCommandProcessor()
                : TTask("cc_" + Now().ToString() + "_" + ToString(static_cast<const void*>(this)))
            {
            }

            virtual ~TCommandProcessor() override {
            }

            void Write(const TString& key, const NJson::TJsonValue& value) {
                GetReply()->InsertValue(key, value);
            }

            virtual void Init(TController& owner, TController::TClient* requester, bool async);
            virtual void Process(void* ts) override;

        protected:
            virtual TString GetName() const = 0;
            virtual void DoProcess() = 0;

            TGuardServer GetServer(bool mustBe = true);
            ui32 ParseRigidStopLevel(const TCgiParameters& params, ui32 fallback = 0) const;

            const TCgiParameters& GetCgi() const {
                return Async ? Cgi : Requester->GetRD().CgiParam;
            }

            TStringBuf GetPostBuffer() {
                return Async ? TStringBuf(Buf.Data(), Buf.Size()) : TStringBuf(Requester->PostBuffer().AsCharPtr(), Requester->PostBuffer().Size());
            }

            TController* Owner = nullptr;
            bool Async = false;

        private:
            TController::TClient* Requester = nullptr;
            TCgiParameters Cgi;
            TBuffer Buf;
        };

        typedef NObjectFactory::TObjectFactory<TController::TCommandProcessor, TString> TCommandFactory;

        class IServerDescriptor {
        public:
            virtual ~IServerDescriptor() {
            }
            virtual IServerConfig* CreateConfig(const TServerConfigConstructorParams& constParams) const = 0;
            virtual IServer* CreateServer(const IServerConfig& config) const = 0;
            virtual TCollectServerInfo* CreateInfoCollector() const = 0;
            virtual TController::TCommandProcessor* CreateCommandProcessor(const TString& command) const = 0;
            virtual TString GetConfigString(TController& owner) const = 0;
            virtual bool CanSetConfig(TController::TGuardServer& gs) const = 0;
        };

    private:
        void DestroyServerImpl(const TDestroyServerContext& ctx) noexcept;
        bool InitConfiguration(const TRestartContext& ctx) noexcept;
        bool StartServerImpl(const TRestartContext& ctx) noexcept;
        void RereadBaseConfig();

    protected:
        virtual bool LoadConfigs(bool force, const bool lastChance);
        virtual void StartItsWatcher() {};
        virtual void StopItsWatcher() {};

    public:
        TController(TServerConfigConstructorParams& params, const IServerDescriptor& descriptor);
        virtual ~TController() override;

        // THttpServer::ICallBack
        virtual TClientRequest* CreateClient() override {
            return new TClient(*this);
        }

        //IMessageProcessor
        virtual bool Process(IMessage* message) override;
        virtual TString Name() const override {
            return "Controller";
        }

        void ClearConfigurations();
        void ClearServerDataStatus();
        bool RestartServer(const TRestartContext& ctx);
        void DestroyServer(const TDestroyServerContext& ctx);
        void Run();
        void Stop(ui32 rigidStopLevel);

        TGuardServer GetServer() {
            return TGuardServer(Server, ServerMutex);
        }

        TGuardConfig GetConfig() const {
            return TGuardConfig(Config, ConfigMutex);
        }

        TGuardedDaemonConfig GetDaemonConfig() const {
            return ConfigParams.GetDaemonConfig();
        }

        TGuardLog GetLog() const {
            return TGuardLog(Log, LogMutex);
        }

        TString GetConfigTextUnsafe() const {
            return ConfigParams.GetTextUnsafe();
        }

        void WaitStopped() {
            Stopped.Wait();
        }

        NController::TServerStatusByController GetStatus() const {
            TReadGuard g(StatusMutex);
            return Status;
        }

        bool IsRestored() const {
            if (!!EnvironmentManager) {
                return EnvironmentManager->IsRestored();
            } else {
                return false;
            }
        }

        NController::TSlotStatus GetSlotStatus() const {
            if (!!EnvironmentManager) {
                return EnvironmentManager->GetStatus();
            } else {
                return NController::OK;
            }
        }

        void SetStatus(NController::TServerStatusByController status) {
            TWriteGuard g(StatusMutex);
            Status = status;
        }

        TServerInfo GetServerInfo(bool controllerOnly);

        void ExecuteAsyncCommand(TAsyncTaskExecutor::TTask::TPtr worker, NJson::TJsonValue& result) {
            AsyncExecuter.AddTask(worker, result);
        }

        virtual void CheckConfig(IServerConfig& /*config*/, bool /*onStart*/) const {
        }

    protected:
        using TAsyncExecuter = TAsyncTaskExecutor;

    protected:
        TServerConfigConstructorParams& ConfigParams;
        THolder<TEnvironmentManager> EnvironmentManager;
        TAtomicSharedPtr<IServerConfig> Config;
        TAtomicSharedPtr<TExtendedServer> Server;
        TAtomicSharedPtr<TLog> Log;
        TMutex ControllerMutex;
        TRWMutex ServerMutex;
        TRWMutex ConfigMutex;
        TRWMutex StatusMutex;
        TRWMutex LogMutex;
        NController::TServerStatusByController Status;
        TManualEvent Stopped;
        TInstant ControllerStart;
        TInstant ServerStart;
        TAsyncExecuter AsyncExecuter;
        const IServerDescriptor& Descriptor;
        bool FirstStart = true;
        THolder<TBomb> LiveTimeBomb;
        TMutex RestartMutex;
    };
}


#if defined(DEFINE_CONTROLLER_COMMAND)
#    error DEFINE_CONTROLLER_COMMAND has been already defined
#endif

#define DEFINE_CONTROLLER_COMMAND(server, name) \
class T ## name ## server ## Command : public NController::TController::TCommandProcessor {\
public:\
    T ## name ## server ## Command(server*)\
    {}\
protected:\
    virtual TString GetName() const { return #name ; }\
    virtual void DoProcess();\
};\
    TServerDescriptor<server>::TCommandFactory::TRegistrator<T ## name ## server ## Command> Registrator ## name ## server(#name);\
    void T ## name ## server ## Command::DoProcess() {

#define END_CONTROLLER_COMMAND }
