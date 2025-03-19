#pragma once

#include "private.h"

#include <cloud/filestore/gateway/nfs/libs/service/public.h>

#include <cloud/storage/core/libs/common/public.h>
#include <cloud/storage/core/libs/diagnostics/public.h>

#include <library/cpp/logger/log.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

class TBootstrap
{
private:
    TOptionsPtr Options;

    ILoggingServicePtr BootstrapLogging;
    TLog Log;

    ITimerPtr Timer;
    ISchedulerPtr Scheduler;
    ILoggingServicePtr Logging;
    IMonitoringServicePtr Monitoring;

    TFileStoreServiceFactoryPtr ServiceFactory;

public:
    TBootstrap(TOptionsPtr options);
    ~TBootstrap();

    void Init();

    void Start();
    void Stop();

    TOptionsPtr GetOptions() const;

private:
    void InitLWTrace();
};

}   // namespace NCloud::NFileStore::NGateway
