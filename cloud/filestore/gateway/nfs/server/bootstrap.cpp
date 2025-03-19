#include "bootstrap.h"

#include "options.h"

#include <cloud/filestore/gateway/nfs/libs/service/factory.h>

#include <cloud/filestore/libs/client/probes.h>

#include <cloud/storage/core/libs/common/scheduler.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>

#include <library/cpp/lwtrace/mon/mon_lwtrace.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

TBootstrap::TBootstrap(TOptionsPtr options)
    : Options(std::move(options))
{}

TBootstrap::~TBootstrap()
{}

void TBootstrap::Init()
{
    Timer = CreateWallClockTimer();
    Scheduler = CreateScheduler();

    TLogSettings logSettings;
    logSettings.UseLocalTimestamps = true;

    if (Options->VerboseLevel) {
        auto level = GetLogLevel(Options->VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());

        logSettings.FiltrationLevel = *level;
    }

    Logging = CreateLoggingService("console", logSettings);

    Log = Logging->CreateLog("NFS");

    if (Options->MonitoringPort) {
        Monitoring = CreateMonitoringService(
            Options->MonitoringPort,
            Options->MonitoringAddress,
            Options->MonitoringThreads);
    } else {
        Monitoring = CreateMonitoringServiceStub();
    }

    InitLWTrace();

    ServiceFactory = std::make_shared<TFileStoreServiceFactory>(
        Timer,
        Scheduler,
        Logging);

    yfs_service_factory_init(ServiceFactory.get());
}

void TBootstrap::InitLWTrace()
{
    auto& probes = NLwTraceMonPage::ProbeRegistry();
    probes.AddProbesList(LWTRACE_GET_PROBES(FILESTORE_CLIENT_PROVIDER));
}

void TBootstrap::Start()
{
    if (Scheduler) {
        Scheduler->Start();
    }

    if (Logging) {
        Logging->Start();
    }

    if (Monitoring) {
        Monitoring->Start();
    }
}

void TBootstrap::Stop()
{
    if (Monitoring) {
        Monitoring->Stop();
    }

    if (Logging) {
        Logging->Stop();
    }

    if (Scheduler) {
        Scheduler->Stop();
    }
}

TOptionsPtr TBootstrap::GetOptions() const
{
    return Options;
}

}   // namespace NCloud::NFileStore::NGateway
