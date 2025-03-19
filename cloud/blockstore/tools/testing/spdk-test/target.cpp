#include "target.h"

#include "device.h"
#include "options.h"
#include "runnable.h"

#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/spdk/env.h>
#include <cloud/blockstore/libs/spdk/target.h>

#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TTestTarget final
    : public IRunnable
{
private:
    TAtomic ShouldStop = 0;
    TAtomic ExitCode = 0;

    TMutex WaitMutex;
    TCondVar WaitCondVar;

    TOptionsPtr Options;

    ILoggingServicePtr Logging;
    TLog Log;

    NSpdk::ISpdkEnvPtr SpdkEnv;
    NSpdk::ISpdkTargetPtr SpdkTarget;

public:
    TTestTarget(TOptionsPtr options)
        : Options(std::move(options))
    {}

    int Run() override;
    void Stop(int exitCode) override;

private:
    void InitLogging();
    void Init();
    void Term();
    void WaitForShutdown();
};

////////////////////////////////////////////////////////////////////////////////

int TTestTarget::Run()
{
    InitLogging();

    STORAGE_INFO("Initializing...");
    try {
        Init();
        WaitForShutdown();
    } catch (...) {
        STORAGE_ERROR(
            "Error during initialization: " << CurrentExceptionMessage());
    }

    STORAGE_INFO("Terminating...");
    try {
        Term();
    } catch (...) {
        STORAGE_ERROR(
            "Error during shutdown: " << CurrentExceptionMessage());
    }

    return AtomicGet(ExitCode);
}

void TTestTarget::InitLogging()
{
    TLogSettings settings;

    if (Options->VerboseLevel) {
        auto level = GetLogLevel(Options->VerboseLevel);
        Y_ENSURE(level, "unknown log level: " << Options->VerboseLevel.Quote());
        settings.FiltrationLevel = *level;
    }

    Logging = CreateLoggingService("console", settings);
    Logging->Start();

    Log = Logging->CreateLog("BLOCKSTORE_TEST");

    NSpdk::InitLogging(Logging->CreateLog("SPDK"));
}

void TTestTarget::WaitForShutdown()
{
    with_lock (WaitMutex) {
        while (AtomicGet(ShouldStop) == 0) {
            WaitCondVar.WaitT(WaitMutex, WaitTimeout);
        }
    }
}

void TTestTarget::Stop(int exitCode)
{
    AtomicSet(ExitCode, exitCode);
    AtomicSet(ShouldStop, 1);

    WaitCondVar.Signal();
}

void TTestTarget::Init()
{
    NProto::TSpdkEnvConfig config;
    if (Options->CpuMask) {
        config.SetCpuMask(Options->CpuMask);
    }

    SpdkEnv = NSpdk::CreateEnv(std::make_shared<NSpdk::TSpdkEnvConfig>(config));
    SpdkEnv->Start();

    auto devices = RegisterDevices(SpdkEnv, *Options);
    Y_ENSURE(devices);

    for (const auto& deviceName: devices) {
        auto stats = SpdkEnv->QueryDeviceStats(deviceName)
            .GetValue(WaitTimeout);

        Cout
            << "Device: " << deviceName << Endl
            << "BlockSize: " << stats.BlockSize << Endl
            << "BlocksCount: " << stats.BlocksCount << Endl;
    }

    SpdkTarget = CreateTarget(SpdkEnv, *Options, devices);
    Y_ENSURE(SpdkTarget);

    SpdkTarget->Start();
}

void TTestTarget::Term()
{
    if (SpdkTarget) {
        SpdkTarget->Stop();
    }

    if (SpdkEnv) {
        SpdkEnv->Stop();
    }

    if (Logging) {
        Logging->Stop();
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IRunnablePtr CreateTestTarget(TOptionsPtr options)
{
    return std::make_shared<TTestTarget>(std::move(options));
}

}   // namespace NCloud::NBlockStore
