#pragma once

#include "private.h"

#include <cloud/filestore/config/server.pb.h>
#include <cloud/filestore/libs/daemon/bootstrap.h>
#include <cloud/filestore/libs/diagnostics/public.h>
#include <cloud/filestore/libs/server/public.h>
#include <cloud/filestore/libs/service/public.h>
#include <cloud/filestore/libs/storage/core/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/diagnostics/public.h>
#include <cloud/storage/core/libs/kikimr/public.h>

#include <library/cpp/actors/util/should_continue.h>
#include <library/cpp/logger/log.h>

namespace NCloud::NFileStore::NServer {

////////////////////////////////////////////////////////////////////////////////

class TFileStoreServerBootstrap final
    : public NDaemon::TBootstrap
{
private:
    TOptionsPtr Options;
    TServerConfigPtr ServerConfig;

    NProto::TServerAppConfig AppConfig;
    IFileStoreServicePtr Service;
    ITaskQueuePtr ThreadPool;

public:
    TFileStoreServerBootstrap(TOptionsPtr options);
    ~TFileStoreServerBootstrap();

private:
    void StartComponents() override;
    void StopComponents() override;
    void InitLWTrace() override;
    IServerPtr CreateServer() override;
    TVector<IIncompleteRequestProviderPtr> GetIncompleteRequestProviders() override;

private:
    void InitConfig();

    void InitKikimrService();
    void InitLocalService();
    void InitNullService();
};

}   // namespace NCloud::NFileStore::NServer
