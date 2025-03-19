#pragma once

#include "context.h"

#include <kernel/daemon/base_controller.h>

#include <library/cpp/regex/pcre/regexp.h>
#include <util/stream/file.h>

namespace NRTProc {
    struct TSlotInfo;
}

using IServer = NController::IServer;
using TExtendedServer = NController::TExtendedServer;

namespace NRTProc {

    class TController: public NController::TController {
    public:
        class TExtendedClient: public TClient {
        public:
            TExtendedClient(TController& owner)
                : TClient(owner)
                , Owner(owner)
            {
            }

            virtual void GetMetrics(IOutputStream& out) const override;
            virtual void ProcessServerStatus() override;

        protected:
            TController& Owner;
        };

        class TExtendedCommandProcessor: public TCommandProcessor {
        public:
            using TCommandProcessor::TCommandProcessor;

            virtual void Init(NController::TController& owner, NController::TController::TClient* requester, bool async) override {
                Owner = VerifyDynamicCast<TController*>(&owner);
                TCommandProcessor::Init(owner, requester, async);
            }

        protected:
            TController* Owner = nullptr;
        };

    protected:
        virtual bool StrictDMUsing() const {
            return false;
        }

    public:
        TController(TServerConfigConstructorParams& params, const IServerDescriptor& descriptor);

        bool DownloadConfigsFromDM(const NController::TDownloadContext& ctx);

        // THttpServer::ICallBack
        virtual TClientRequest* CreateClient() override {
            return new TExtendedClient(*this);
        }

        // TController
        virtual bool LoadConfigs(const bool force, const bool lastChance) override;

        bool SetConfigFields(const TCgiParameters& cgiParams, NJson::TJsonValue& result);
        bool GetConfigFields(const TCgiParameters& cgiParams, NJson::TJsonValue& result);
        bool CanSetConfig(bool& isExists);

        void FillConfigsHashes(NJson::TJsonValue& result, TString configPath = TString(), const TRegExMatch* filter = nullptr) const;

        TFsPath GetFullPath(const TString& path, const TString& root = Default<TString>()) {
            if (NoFileOperations)
                ythrow yexception() << "file operations locked";
            TFsPath fsPath(path);
            if (fsPath.IsAbsolute())
                return fsPath.Fix().GetPath();
            if (!root)
                return (TFsPath(ConfigParams.GetDaemonConfig()->GetController().ConfigsRoot) / fsPath).Fix();
            return (TFsPath(root) / fsPath).Fix();
        }

        void WriteFile(const TString& filename, TStringBuf data, const TString& root = Default<TString>()) {
            TFsPath path(GetFullPath(filename, root));
            {
                TGuard<TMutex> g(MutexMakeDirs);
                path.Parent().MkDirs();
            }
            TFixedBufferFileOutput fo(path.GetPath());
            fo.Write(data.data(), data.size());
        }

        void GetAsyncCommandInfo(const TString& taskId, TAsyncTaskExecutor::TTask& requester) {
            TAsyncExecuter::TTask::TPtr task = AsyncExecuter.GetTask(taskId);
            if (!task)
                requester.GetReply()->InsertValue("result", NJson::JSON_MAP).InsertValue("task_status", ToString(TAsyncTaskExecutor::TTask::stsNotFound));
            else
                task->FillInfo(requester.GetReply()->InsertValue("result", NJson::JSON_MAP));
        }

        bool GetNoFileOperations() const {
            return NoFileOperations;
        }

        void SetNoFileOperations(bool value) {
            NoFileOperations = value;
        }

        bool GetMustBeAlive() const {
            return MustBeAlive;
        }

    protected:
        TMutex MutexMakeDirs;
        bool NoFileOperations;
        bool MustBeAlive;
    };

    class TControllerExternal: public TController {
    protected:
        virtual bool StrictDMUsing() const {
            return true;
        }

    public:
        using TController::TController;
    };
}

#undef  DEFINE_CONTROLLER_COMMAND
#define DEFINE_CONTROLLER_COMMAND(server, name) \
class T ## name ## server ## Command : public NRTProc::TController::TExtendedCommandProcessor {\
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
