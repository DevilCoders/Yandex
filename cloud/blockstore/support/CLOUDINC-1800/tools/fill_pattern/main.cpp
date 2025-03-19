#include <cloud/storage/core/libs/common/format.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/threading/blocking_queue/blocking_queue.h>

#include <util/generic/scope.h>
#include <util/generic/size_literals.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/string/strip.h>
#include <util/system/file.h>
#include <util/system/shellcommand.h>
#include <util/thread/factory.h>

#include <memory>

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString DevPath = "/dev/nbd0";
    TString Pattern = "CLOUD\n";

    TString Generator = "strip";

    bool Verbose = false;
    bool DryRun = false;

    unsigned int IoDepth = 4;

    void Parse(int argc, char** argv)
    {
        using namespace NLastGetopt;

        TOpts opts;
        opts.AddHelpOption();

        opts.AddLongOption("path")
            .Required()
            .RequiredArgument("PATH")
            .StoreResult(&DevPath);

        opts.AddLongOption("pattern")
            .RequiredArgument("STR")
            .DefaultValue(Pattern)
            .StoreResult(&Pattern);

        opts.AddLongOption("generator")
            .RequiredArgument("STR")
            .DefaultValue(Generator)
            .StoreResult(&Generator);

        opts.AddLongOption("io-depth")
            .RequiredArgument("NUM")
            .DefaultValue(IoDepth)
            .StoreResult(&IoDepth);

        opts.AddLongOption('v', "verbose")
            .NoArgument()
            .StoreTrue(&Verbose);

        opts.AddLongOption("dry-run")
            .NoArgument()
            .StoreTrue(&DryRun);

        TOptsParseResultException res(&opts, argc, argv);
    }
};

////////////////////////////////////////////////////////////////////////////////

ui64 GetDeviceSize(const TString& devPath)
{
    TShellCommand cmd("blockdev", { "--getsize64", devPath });

    auto output = cmd.Run().Wait().GetOutput();

    auto ec = cmd.GetExitCode();

    Y_ENSURE(!ec.Empty());
    Y_ENSURE(ec == 0, "blockdev: " << *ec);

    return FromString<ui64>(Strip(output));
}

////////////////////////////////////////////////////////////////////////////////

struct TRange
{
    ui64 Offset = 0;
    ui64 ByteCount = 0;
};

struct IRangeGenerator
{
    virtual ~IRangeGenerator() = default;
    virtual TRange Emit() = 0;
};

class TStripRangeGenerator
    : public IRangeGenerator
{
private:
    const ui32 BufferSize;
    const ui64 ChunkSize = 93_GB;

    TVector<TRange> Chunks;

    size_t Idx = 0;

public:
    TStripRangeGenerator(ui32 bufferSize, ui64 totalByteCount)
        : BufferSize(bufferSize)
    {
        Y_ENSURE(totalByteCount % ChunkSize == 0);

        Chunks.reserve(totalByteCount / ChunkSize);

        ui64 offset = 0;
        while (offset != totalByteCount) {
            Chunks.push_back({offset, ChunkSize});

            offset += ChunkSize;
        }
    }

    TRange Emit() override
    {
        if (Chunks.empty()) {
            return {};
        }

        if (Idx >= Chunks.size()) {
            Idx = 0;
        }

        auto& chunk = Chunks[Idx];

        const ui64 offset = chunk.Offset;
        const ui64 len = Min<ui64>(chunk.ByteCount, BufferSize);

        chunk.Offset += len;
        chunk.ByteCount -= len;

        if (!chunk.ByteCount) {
            if (Idx != Chunks.size() - 1) {
                Chunks[Idx] = Chunks.back();
            }
            Chunks.pop_back();
        } else {
            ++Idx;
        }

        return {offset, len};
    }
};

class TLinearRangeGenerator
    : public IRangeGenerator
{
private:
    const ui32 BufferSize;
    const ui64 TotalByteCount;

    ui64 Offset = 0;

public:
    TLinearRangeGenerator(
            ui32 bufferSize,
            ui64 totalByteCount)
        : BufferSize(bufferSize)
        , TotalByteCount(totalByteCount)
    {}

    TRange Emit() override
    {
        if (Offset == TotalByteCount) {
            return {};
        }

        const ui64 len = Min<ui64>(TotalByteCount - Offset, BufferSize);
        const ui64 offset = Offset;

        Offset += len;

        return {offset, len};
    }
};

////////////////////////////////////////////////////////////////////////////////

class TApp
{
    using TThread = THolder<IThreadFactory::IThread>;

private:
    const TOptions& Options;

    TFile Dev;

    char* Buffer;
    static constexpr const ui64 BufferSize = 4_MB;

    NThreading::TBlockingQueue<TRange> WriteQueue;
    TVector<TThread> Writers;

public:
    explicit TApp(const TOptions& options)
        : Options(options)
        , Dev(Options.DevPath, OpenExisting | RdWr | DirectAligned | Sync)
        , Buffer(static_cast<char*>(std::aligned_alloc(4_KB, BufferSize)))
        , WriteQueue(Options.IoDepth)
    {
        ui64 offset = 0;
        while (offset != BufferSize) {
            ui64 len = Min<ui64>(Options.Pattern.size(), BufferSize - offset);
            memcpy(Buffer + offset, Options.Pattern.data(), len);
            offset += len;
        }
    }

    ~TApp()
    {
        WriteQueue.Stop();

        for (auto& w: Writers) {
            w->Join();
        }

        std::free(Buffer);
    }

    int Run()
    {
        Writers.reserve(Options.IoDepth);
        for (ui32 i = 0; i != Options.IoDepth; ++i) {
            Writers.push_back(SystemThreadFactory()->Run([this] {
                WriterThread();
            }));
        }

        ui64 bytesWritten = 0;
        const ui64 totalSize = GetDeviceSize(Options.DevPath);

        auto gen = CreateGenerator(totalSize);
        for (;;) {
            auto [offset, len] = gen->Emit();

            if (!len) {
                break;
            }

            bytesWritten += len;

            WriteQueue.Push({ offset, len });
        }

        // wait for Writers

        for (size_t i = 0; i != Writers.size(); ++i) {
            WriteQueue.Push({});
        }

        for (auto& r: Writers) {
            r->Join();
        }
        Writers.clear();

        Y_ENSURE(bytesWritten == totalSize);

        return 0;
    }

private:
    std::unique_ptr<IRangeGenerator> CreateGenerator(ui64 totalSize) const
    {
        if (Options.Generator == "strip") {
            return std::make_unique<TStripRangeGenerator>(BufferSize, totalSize);
        }

        if (Options.Generator == "linear") {
            return std::make_unique<TLinearRangeGenerator>(
                BufferSize,
                totalSize);
        }

        ythrow yexception() << "unknown generator: " << Options.Generator;
    }

    void WriterThread()
    {
        while (auto task = WriteQueue.Pop()) {
            auto r = *task;

            if (!r.ByteCount) {
                break;
            }

            if (!Options.DryRun) {
                Dev.Pwrite(Buffer, r.ByteCount, r.Offset);
            }
        }
    }
};

} // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
    try {
        TOptions options;
        options.Parse(argc, argv);

        TApp app(options);

        return app.Run();

    } catch(...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
