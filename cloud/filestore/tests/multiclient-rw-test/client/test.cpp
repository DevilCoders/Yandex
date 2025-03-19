#include "test.h"

#include "options.h"

#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/digest/crc32c/crc32c.h>

#include <util/datetime/base.h>
#include <util/generic/size_literals.h>
#include <util/random/random.h>
#include <util/system/file.h>
#include <util/system/info.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TBlockData
{
    ui64 RequestNumber;
    ui64 RequestTimestamp;
    ui64 RandomNumber;
    ui64 CheckSum;
};

////////////////////////////////////////////////////////////////////////////////

class TTest final
   : public ITest
{
private:
    TOptionsPtr Options;

    ILoggingServicePtr Logging;
    TLog Log;

    TAtomic ShouldStop = 0;

    char* Buffer;

public:
    TTest(TOptionsPtr options)
        : Options(std::move(options))
    {
        Buffer = static_cast<char*>(
            std::aligned_alloc(
                NSystemInfo::GetPageSize(),
                Options->RequestSize * 1_MB)
        );
        InitLogger();
    }

    ~TTest()
    {
        std::free(Buffer);
    }

    int Run() override;
    void Stop() override;

    int RunReadClient();
    int RunWriteClient();

private:
    void InitLogger();
    bool InterruptedSleep(TDuration time);
};

////////////////////////////////////////////////////////////////////////////////

void TTest::InitLogger()
{
    TLogSettings settings;
    settings.UseLocalTimestamps = true;
    Logging = CreateLoggingService("console", settings);
    Logging->Start();
    Log = Logging->CreateLog("CLIENT");
}

int TTest::Run()
{
    if (InterruptedSleep(TDuration::Minutes(Options->SleepBeforeStart))) {
        return 0;
    }

    switch (Options->Type) {
        case EClientType::Write:
            return RunWriteClient();
        case EClientType::Read:
            return RunReadClient();
    }
}

bool TTest::InterruptedSleep(TDuration time)
{
    const auto start = Now();
    while (Now() - start < time) {
        if (AtomicGet(ShouldStop)) {
            return true;
        }
    }
    return false;
}

int TTest::RunWriteClient()
{
    EOpenMode flags = EOpenModeFlag::WrOnly | EOpenModeFlag::OpenAlways | EOpenModeFlag::DirectAligned;
    TFile file(Options->FilePath, flags);

    while (!AtomicGet(ShouldStop)) {
        file.Flock(LOCK_EX);

        file.Resize(1_GB);

        const ui64 requestNum = Options->FileSize * 1_GB / (Options->RequestSize * 1_MB);
        for (ui32 i = 0; i < requestNum; ++i) {
            if (AtomicGet(ShouldStop)) {
                file.Flock(LOCK_UN);
                return 0;
            }

            TBlockData data {
                .RequestNumber = i,
                .RequestTimestamp = Now().MicroSeconds(),
                .RandomNumber = RandomNumber<ui64>(),
                .CheckSum = 0
            };
            data.CheckSum = Crc32c(&data, sizeof(data));
            memcpy(Buffer, &data, sizeof(data));

            try {
                file.Pwrite(Buffer, Options->RequestSize * 1_MB, i * Options->RequestSize * 1_MB);
            } catch (...) {
                STORAGE_ERROR("Can't write to file: " << CurrentExceptionMessage());
                file.Flock(LOCK_UN);
                Stop();
                return 1;
            }
        }
        file.Flock(LOCK_UN);

        if (InterruptedSleep(TDuration::Minutes(Options->SleepBetweenWrites))) {
            return 0;
        }
    }

    return 0;
}

int TTest::RunReadClient()
{
    EOpenMode flags = EOpenModeFlag::RdOnly | EOpenModeFlag::OpenAlways | EOpenModeFlag::DirectAligned;
    TFile file(Options->FilePath, flags);

    while (!AtomicGet(ShouldStop)) {
        file.Flock(LOCK_SH);

        const ui64 requestNum = Options->FileSize * 1_GB / (Options->RequestSize * 1_MB);
        for (ui32 i = 0; i < requestNum; ++i) {
            if (AtomicGet(ShouldStop)) {
                break;
            }

            try {
                file.Pread(Buffer, Options->RequestSize * 1_MB, i * Options->RequestSize * 1_MB);
            } catch (...) {
                STORAGE_ERROR("Can't read from file: " << CurrentExceptionMessage());
                file.Flock(LOCK_UN);
                Stop();
                return 1;
            }

            TBlockData data;
            memcpy(&data, Buffer, sizeof(data));
            ui32 actual = data.CheckSum;
            data.CheckSum = 0;
            ui32 expected = Crc32c(&data, sizeof(data));

            if (actual != expected) {
                STORAGE_ERROR("Wrong checksum in request number " << i
                    << " expected " << expected
                    << " actual " << actual
                    << " timestamp " << TInstant::MicroSeconds(data.RequestTimestamp));
                file.Flock(LOCK_UN);
                Stop();
                return 1;
            }
        }

        file.Flock(LOCK_UN);
    }

    return 0;
}

void TTest::Stop()
{
    AtomicSet(ShouldStop, 1);
    if (Logging) {
        Logging->Stop();
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

ITestPtr CreateTest(TOptionsPtr options)
{
    return std::make_shared<TTest>(std::move(options));
}

}   // namespace NCloud::NFileStore
