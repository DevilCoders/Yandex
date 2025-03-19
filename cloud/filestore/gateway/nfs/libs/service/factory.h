#pragma once

#include "public.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

class TFileStoreServiceFactory final
    : public yfs_service_factory
{
private:
    const ITimerPtr Timer;
    const ISchedulerPtr Scheduler;
    const ILoggingServicePtr Logging;

    TLog Log;

public:
    TFileStoreServiceFactory(
        ITimerPtr timer,
        ISchedulerPtr scheduler,
        ILoggingServicePtr logging);

    //
    // Factory methods
    //

    int Create(
        const TString& configPath,
        const TString& filesystemId,
        const TString& clientId,
        yfs_service** svc);

    int Destroy(yfs_service* svc);

private:
    static void Init(yfs_service_factory* sf);

    template <typename Method, typename... Args>
    static int Call(Method&& m, yfs_service_factory* svc, Args&&... args) noexcept;
};

}   // namespace NCloud::NFileStore::NGateway
