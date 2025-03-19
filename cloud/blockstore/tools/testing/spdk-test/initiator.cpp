#include "initiator.h"

#include "device.h"
#include "options.h"
#include "runnable.h"

#include <cloud/blockstore/libs/spdk/alloc.h>
#include <cloud/blockstore/libs/spdk/config.h>
#include <cloud/blockstore/libs/spdk/device.h>
#include <cloud/blockstore/libs/spdk/env.h>

#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/weighted_percentile.h>

#include <util/datetime/base.h>
#include <util/generic/intrlist.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/condvar.h>
#include <util/system/mutex.h>

namespace NCloud::NBlockStore {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

constexpr TDuration ReportInterval = TDuration::Seconds(5);

////////////////////////////////////////////////////////////////////////////////

bool ValidateRequestSize(ui32 requestSize, const NSpdk::TDeviceStats& stats)
{
    if (!requestSize) {
        return true;
    }

    return requestSize % stats.BlockSize == 0
        && requestSize <= stats.BlockSize * stats.BlocksCount;
}

////////////////////////////////////////////////////////////////////////////////

struct TRequest : TIntrusiveSListItem<TRequest>
{
    NSpdk::TSpdkBuffer Buffer;

    ui32 StartOffset;
    ui32 BlocksCount;

    TInstant SendTs;
};

////////////////////////////////////////////////////////////////////////////////

class TRequestSender
{
public:
    struct TOptions
    {
        ui32 BlockSize;
        ui64 BlocksCount;

        ui32 MaxIoDepth;
        ui32 MinBlocksCount;
        ui32 MaxBlocksCount;
        ui32 WriteRate;
    };

private:
    const NSpdk::ISpdkDevicePtr Device;
    const TOptions Options;

    TVector<TRequest> Requests;
    TIntrusiveSList<TRequest> CompletedRequests;
    size_t CompletedRequestsCount = 0;

    TMutex Mutex;
    TCondVar CondVar;

public:
    TRequestSender(NSpdk::ISpdkDevicePtr device, const TOptions& options)
        : Device(std::move(device))
        , Options(options)
    {
        Requests.resize(Options.MaxIoDepth);

        for (auto& request: Requests) {
            request.Buffer = NSpdk::AllocateUninitialized(
                Options.BlockSize * Options.MaxBlocksCount);

            CompletedRequests.PushFront(&request);
            ++CompletedRequestsCount;
        }
    }

    void SendRequest(TRequest* request)
    {
        request->BlocksCount = Options.MinBlocksCount;
        if (Options.MinBlocksCount < Options.MaxBlocksCount) {
            request->BlocksCount += RandomNumber(
                Options.MaxBlocksCount - Options.MinBlocksCount);
        }

        request->StartOffset = RandomNumber(
            Options.BlocksCount - request->BlocksCount);

        request->SendTs = TInstant::Now();

        if (RandomNumber(100ul) < Options.WriteRate) {
            WriteBlocks(request);
        } else {
            ReadBlocks(request);
        }
    }

    TRequest* WaitForRequest(TDuration timeout)
    {
        with_lock (Mutex) {
            if (CompletedRequests) {
                auto* request = CompletedRequests.Front();

                CompletedRequests.PopFront();
                --CompletedRequestsCount;

                return request;
            }

            CondVar.WaitT(Mutex, timeout);
            return nullptr;
        }
    }

    bool WaitForCompletion(TDuration timeout)
    {
        with_lock (Mutex) {
            if (CompletedRequestsCount == Requests.size()) {
                return true;
            }

            CondVar.WaitT(Mutex, timeout);
            return false;
        }
    }

private:
    void WriteBlocks(TRequest* request)
    {
        auto future = Device->Write(
            request->Buffer.get(),
            request->StartOffset * Options.BlockSize,
            request->BlocksCount * Options.BlockSize);

        future.Subscribe([=] (auto f) mutable {
            ReportProgress(request, ExtractResponse(f));
        });
    }

    void ReadBlocks(TRequest* request)
    {
        auto future = Device->Read(
            request->Buffer.get(),
            request->StartOffset * Options.BlockSize,
            request->BlocksCount * Options.BlockSize);

        future.Subscribe([=] (auto f) mutable {
            ReportProgress(request, ExtractResponse(f));
        });
    }

    void ReportProgress(TRequest* request, const NProto::TError& error)
    {
        // TODO
        Y_UNUSED(error);

        with_lock (Mutex) {
            CompletedRequests.PushFront(request);
            ++CompletedRequestsCount;

            CondVar.Signal();
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TTestInitiator final
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
    NSpdk::ISpdkDevicePtr SpdkDevice;
    NSpdk::TDeviceStats Stats;
    TString DeviceName;

    TAtomic UpdateIOStatsInProgress = false;

    TInstant LastReportTs;
    TInstant IoStatsTS;
    TInstant LastIoStatsTS;

    NSpdk::TDeviceIoStats IoStats = {};
    NSpdk::TDeviceIoStats LastIoStats = {};

    ui64 BytesReadPerSecond = 0;
    ui64 BytesWrittenPerSecond = 0;
    ui64 NumReadOpsPerSecond = 0;
    ui64 NumWriteOpsPerSecond = 0;

public:
    TTestInitiator(TOptionsPtr options)
        : Options(std::move(options))
    {}

    int Run() override;
    void Stop(int exitCode) override;

private:
    void InitLogging();
    void Init();
    void Shoot();
    void WaitForShutdown();
    void Term();

    void ReportProgress();
    bool UpdateIOStats();
};

////////////////////////////////////////////////////////////////////////////////

int TTestInitiator::Run()
{
    InitLogging();

    STORAGE_INFO("Initializing...");
    try {
        Init();
        Shoot();
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

void TTestInitiator::InitLogging()
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

void TTestInitiator::WaitForShutdown()
{
    with_lock (WaitMutex) {
        while (AtomicGet(ShouldStop) == 0) {
            WaitCondVar.WaitT(WaitMutex, WaitTimeout);
        }
    }
}

void TTestInitiator::Stop(int exitCode)
{
    AtomicSet(ExitCode, exitCode);
    AtomicSet(ShouldStop, 1);

    WaitCondVar.Signal();
}

void TTestInitiator::Init()
{
    NProto::TSpdkEnvConfig config;
    if (Options->CpuMask) {
        config.SetCpuMask(Options->CpuMask);
    }

    SpdkEnv = NSpdk::CreateEnv(std::make_shared<NSpdk::TSpdkEnvConfig>(config));
    SpdkEnv->Start();

    auto devices = RegisterDevices(SpdkEnv, *Options);
    Y_ENSURE(devices);

    auto deviceName = devices.front();

    auto stats = SpdkEnv->QueryDeviceStats(deviceName)
        .GetValue(WaitTimeout);

    Cout
        << "Device: " << deviceName << Endl
        << "BlockSize: " << stats.BlockSize << Endl
        << "BlocksCount: " << stats.BlocksCount << Endl;

    if (!ValidateRequestSize(Options->MinRequestSize, stats)) {
        ythrow yexception() << "Incorrect min-request-size";
    }

    if (!ValidateRequestSize(Options->MaxRequestSize, stats)) {
        ythrow yexception() << "Incorrect max-request-size";
    }

    if (!Options->MinRequestSize) {
        Options->MinRequestSize = stats.BlockSize;
    }

    if (!Options->MaxRequestSize) {
        Options->MaxRequestSize = stats.BlockSize;
    }

    SpdkDevice = SpdkEnv->OpenDevice(deviceName, true)
        .GetValue(WaitTimeout);

    SpdkEnv->SetRateLimits(deviceName, {
        Options->IopsLimit,
        Options->BandwidthLimit,
        Options->ReadBandwidthLimit,
        Options->WriteBandwidthLimit
    }).GetValue(WaitTimeout);

    if (Options->CollectHistogram) {
        SpdkEnv->EnableHistogram(deviceName, true).GetValue(WaitTimeout);
    }

    Stats = stats;
    DeviceName = deviceName;
}

void TTestInitiator::Shoot()
{
    TRequestSender::TOptions opts = {
        .BlockSize = Stats.BlockSize,
        .BlocksCount = Stats.BlocksCount,
        .MaxIoDepth = Options->MaxIoDepth,
        .MinBlocksCount = Options->MinRequestSize / Stats.BlockSize,
        .MaxBlocksCount = Options->MaxRequestSize / Stats.BlockSize,
        .WriteRate = Options->WriteRate,
    };

    TRequestSender sender(SpdkDevice, opts);

    const auto startTime = Now();
    const auto deadline = Options->TestDuration
        ? Options->TestDuration.ToDeadLine(startTime)
        : TInstant::Max();

    LastReportTs = startTime;
    UpdateIOStats();

    while (AtomicGet(ShouldStop) == 0) {
        if (auto* request = sender.WaitForRequest(WaitTimeout)) {
            sender.SendRequest(request);
        }

        if (deadline < Now()) {
            Stop(0);
        }

        ReportProgress();
    }

    while (!sender.WaitForCompletion(WaitTimeout)) {
        ReportProgress();
    }

    const auto stats = SpdkEnv->GetDeviceIoStats(DeviceName)
        .GetValue(WaitTimeout);

    const auto seconds = (Now() - startTime).Seconds();
    const auto ops = stats.NumReadOps + stats.NumWriteOps;
    const auto bytes = stats.BytesRead + stats.BytesWritten;

    Cout << "===  Total  ===\n"
        << "Reads        : " << stats.NumReadOps << "\n"
        << "Writes       : " << stats.NumWriteOps << "\n"
        << "Bytes read   : " << stats.BytesRead << "\n"
        << "Bytes written: " << stats.BytesWritten << "\n\n";

    Cout << "=== Average ===\n"
        << "IOPS  : " << (ops / seconds) << "\n"
        << "BW(r) : " << FormatByteSize(stats.BytesRead / seconds) << "/s\n"
        << "BW(w) : " << FormatByteSize(stats.BytesWritten / seconds) << "/s\n"
        << "BW(rw): " << FormatByteSize(bytes / seconds) << "/s\n\n";

    if (Options->CollectHistogram) {
        const TVector<TPercentileDesc> percentiles {
            { 0.01,   "    1" },
            { 0.10,   "   10" },
            { 0.25,   "   25" },
            { 0.50,   "   50" },
            { 0.75,   "   75" },
            { 0.90,   "   90" },
            { 0.95,   "   95" },
            { 0.98,   "   98" },
            { 0.99,   "   99" },
            { 0.995,  " 99.5" },
            { 0.999,  " 99.9" },
            { 0.9999, "99.99" },
            { 1.0000, "100.0" },
        };

        const auto buckets = SpdkEnv->GetHistogramBuckets(DeviceName)
            .GetValue(WaitTimeout);

        const auto values = CalculateWeightedPercentiles(buckets, percentiles);

        Cout << "=== Latency (mcs) ===\n";
        for (size_t i = 0; i != percentiles.size(); ++i) {
            Cout << percentiles[i].second << " : " << values[i] << "\n";
        }
    }
}

void TTestInitiator::ReportProgress()
{
    const auto now = Now();
    if (now - LastReportTs > ReportInterval) {
        if (UpdateIOStats()) {
            STORAGE_INFO(
                "IOPS: " << (NumWriteOpsPerSecond + NumReadOpsPerSecond) << " "
                "BW(r): " << FormatByteSize(BytesReadPerSecond) << "/s "
                "BW(w): " << FormatByteSize(BytesWrittenPerSecond) << "/s "
                "BW(total): " << FormatByteSize(
                    BytesReadPerSecond + BytesWrittenPerSecond) << "/s"
            );
        }

        LastReportTs = now;
    }
}

bool TTestInitiator::UpdateIOStats()
{
    const bool hasStats = !!LastIoStatsTS;

    if (AtomicGet(UpdateIOStatsInProgress)) {
        return hasStats;
    }

    if (LastIoStatsTS) {
        auto seconds = (IoStatsTS - LastIoStatsTS).Seconds();

        BytesReadPerSecond = (IoStats.BytesRead - LastIoStats.BytesRead) / seconds;
        BytesWrittenPerSecond = (IoStats.BytesWritten - LastIoStats.BytesWritten) / seconds;
        NumReadOpsPerSecond = (IoStats.NumReadOps - LastIoStats.NumReadOps) / seconds;
        NumWriteOpsPerSecond = (IoStats.NumWriteOps - LastIoStats.NumWriteOps) / seconds;
    }

    LastIoStatsTS = IoStatsTS;
    LastIoStats = IoStats;

    UpdateIOStatsInProgress = true;
    SpdkEnv->GetDeviceIoStats(DeviceName).Subscribe([=] (auto future) {
        IoStats = future.GetValue();
        IoStatsTS = Now();

        AtomicSet(UpdateIOStatsInProgress, false);
    });

    return hasStats;
}

void TTestInitiator::Term()
{
    if (SpdkDevice) {
        SpdkDevice->Stop();
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

IRunnablePtr CreateTestInitiator(TOptionsPtr options)
{
    return std::make_shared<TTestInitiator>(std::move(options));
}

}   // namespace NCloud::NBlockStore
