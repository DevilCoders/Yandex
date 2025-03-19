#include "command.h"

#include <cloud/filestore/libs/client/config.h>
#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/diagnostics/request_stats.h>
#include <cloud/filestore/libs/fuse/config.h>
#include <cloud/filestore/libs/fuse/driver.h>
#include <cloud/filestore/libs/fuse/fs.h>

namespace NCloud::NFileStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TMountCommand final
    : public TFileStoreCommand
{
private:
    TString MountPath;
    bool MountReadOnly = false;

    NFuse::IFileSystemDriverPtr FileSystemDriver;

public:
    TMountCommand()
    {
        Opts.AddLongOption("mount-path")
            .Required()
            .RequiredArgument("STR")
            .StoreResult(&MountPath);

        Opts.AddLongOption("mount-readonly")
            .NoArgument()
            .SetFlag(&MountReadOnly);
    }

    void Init() override
    {
        TFileStoreCommand::Init();

        NProto::TSessionConfig sessionConfig;
        sessionConfig.SetFileSystemId(FileSystemId);
        sessionConfig.SetClientId(ClientId);
        auto session = NClient::CreateSession(
            Logging,
            Timer,
            Scheduler,
            Client,
            std::make_shared<TSessionConfig>(sessionConfig));

        NProto::TFuseConfig proto;
        proto.SetFileSystemId(FileSystemId);
        proto.SetClientId(ClientId);
        proto.SetMountPath(MountPath);
        proto.SetReadOnly(MountReadOnly);
        if (LogSettings.FiltrationLevel > TLOG_DEBUG) {
            proto.SetDebug(true);
        }

        auto config = std::make_shared<NFuse::TFuseConfig>(proto);
        FileSystemDriver = NFuse::CreateFileSystemDriver(
            config,
            Logging,
            CreateRequestStatsRegistryStub(),
            session,
            NFuse::CreateFuseFileSystemFactory(
                Logging,
                Scheduler,
                Timer));
    }

    void Start() override
    {
        TFileStoreCommand::Start();

        if (FileSystemDriver) {
            auto error = FileSystemDriver->StartAsync().GetValueSync();
            if (FAILED(error.GetCode())) {
                STORAGE_ERROR("failed to start driver: " << FormatError(error))
                ythrow TServiceError(error);
            }
        }
    }

    void Stop() override
    {
        if (FileSystemDriver) {
            FileSystemDriver->StopAsync().GetValueSync();
        }

        TFileStoreCommand::Stop();
    }

    bool Execute() override
    {
        // wait until stopped by user
        return false;
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TCommandPtr NewMountCommand()
{
    return std::make_shared<TMountCommand>();
}

}   // namespace NCloud::NFileStore::NClient
