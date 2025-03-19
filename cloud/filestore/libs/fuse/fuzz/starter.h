#pragma once

#include <cloud/filestore/libs/fuse/log.h>
#include <cloud/filestore/libs/service/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/vhost-client/vhost-client.h>

namespace NCloud::NFileStore::NFuse {

class TStarter
{
private:
    ILoggingServicePtr Logging;
    ITimerPtr Timer;
    ISchedulerPtr Scheduler;

    IFileStoreServicePtr Service;
    IFileSystemDriverPtr Driver;

    TString SocketPath;
    NVHost::TClient Client;

public:
    static TStarter* GetStarter();

    void Start();
    void Stop();

    int Run(const ui8* data, size_t size);

private:
    TStarter();
};

} // namespace NCloud::NFileStore::NFuse
