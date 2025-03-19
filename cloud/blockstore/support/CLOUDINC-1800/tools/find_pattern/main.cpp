#include <cloud/storage/core/libs/common/format.h>

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/threading/blocking_queue/blocking_queue.h>

#include <util/generic/scope.h>
#include <util/generic/size_literals.h>
#include <util/generic/vector.h>
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

    bool Verbose = false;

    unsigned int IoDepth = 4;
    unsigned int BlockSize = 4_KB;

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

        opts.AddLongOption("io-depth")
            .RequiredArgument("NUM")
            .DefaultValue(IoDepth)
            .StoreResult(&IoDepth);

        opts.AddLongOption("block-size")
            .RequiredArgument("NUM")
            .DefaultValue(BlockSize)
            .StoreResult(&BlockSize);

        opts.AddLongOption('v', "verbose")
            .NoArgument()
            .StoreTrue(&Verbose);

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

class TApp
{
    using TThread = THolder<IThreadFactory::IThread>;

    struct TReadTask {
        ui32 SeqNo = 0;
        char* Buffer = nullptr;
        ui32 ByteCount = 0;
    };

    struct TRange
    {
        ui64 StartIndex = 0;
        ui64 BlockCount = 0;
    };

    struct TPrintTask {
        ui32 SeqNo = 0;
        TVector<TRange> DirtyBlocks;
    };

private:
    const TOptions& Options;

    TFile Dev;

    TVector<char*> Buffers;
    static constexpr const ui64 BufferSize = 4_MB;

    NThreading::TBlockingQueue<char*> BufferQueue;
    NThreading::TBlockingQueue<TReadTask> ReadQueue;
    NThreading::TBlockingQueue<TPrintTask> PrintQueue;

    TVector<TThread> Readers;
    TThread Printer;

public:
    explicit TApp(const TOptions& options)
        : Options(options)
        , Dev(Options.DevPath, OpenExisting | RdOnly | DirectAligned)
        , BufferQueue(Options.IoDepth)
        , ReadQueue(Options.IoDepth)
        , PrintQueue(1000)
    {
        Buffers.reserve(2 * Options.IoDepth); // readers + writers
        for (ui32 i = 0; i != Options.IoDepth; ++i) {
            Buffers.push_back(static_cast<char*>(
                std::aligned_alloc(Options.BlockSize, BufferSize)
            ));
        }
    }

    ~TApp()
    {
        BufferQueue.Stop();
        ReadQueue.Stop();
        PrintQueue.Stop();

        if (Printer) {
            Printer->Join();
        }

        for (auto& r: Readers) {
            r->Join();
        }

        for (void* b: Buffers) {
            std::free(b);
        }
    }

    int Run()
    {
        Readers.reserve(Options.IoDepth);

        for (ui32 i = 0; i != Options.IoDepth; ++i) {
            Readers.push_back(SystemThreadFactory()->Run([this] {
                ReaderThread();
            }));
        }

        Printer = SystemThreadFactory()->Run([this] { PrinterThread(); });

        const ui64 totalSize = GetDeviceSize(Options.DevPath);
        ui64 byteCount = totalSize;

        i64 offset = 0;
        for (char* b: Buffers) {
            BufferQueue.Push(b);
        }

        ui32 progress = 0;
        ui32 index = 0;
        while (byteCount) {
            auto b = BufferQueue.Pop();
            Y_VERIFY(b);

            const ui32 len = static_cast<ui32>(std::min(byteCount, BufferSize));
            ReadQueue.Push({ index++, *b, len });

            byteCount -= len;
            offset += len;

            ui32 p = offset * 100 / totalSize;
            if (p > progress) {
                progress = p;
                Cerr << progress << " %" << Endl;
            }
        }

        // wait for Readers

        for (size_t i = 0; i != Readers.size(); ++i) {
            ReadQueue.Push({});
        }

        for (auto& r: Readers) {
            r->Join();
        }
        Readers.clear();

        // wait for Printer
        PrintQueue.Push({ static_cast<ui32>(-1), {} });
        Printer->Join();
        Printer = nullptr;

        return 0;
    }

private:
    void ReaderThread()
    {
        TVector<TRange> dirtyBlocks;

        while (auto task = ReadQueue.Pop()) {
            auto [index, buffer, len] = *task;

            if (!len) {
                break;
            }

            const ui64 offset = index * BufferSize;
            Dev.Pload(buffer, len, offset);

            FindPattern(buffer, len, dirtyBlocks);

            BufferQueue.Push(buffer);

            for (auto& db: dirtyBlocks) {
                db.StartIndex += offset / Options.BlockSize;
            }

            PrintQueue.Push({ index, std::move(dirtyBlocks) });
            dirtyBlocks.clear();
        }
    }

    void FindPattern(char* buffer, ui64 len, TVector<TRange>& dirtyBlocks)
    {
        ui32 startIndex = static_cast<ui32>(-1);
        ui32 count = 0;

        ui32 offset = 0;

        while (len) {
            TStringBuf block{buffer + offset, Options.BlockSize};
            if (block.find(Options.Pattern) != block.npos) {
                ++count;
                if (startIndex == static_cast<ui32>(-1)) {
                    startIndex = offset / Options.BlockSize;
                }
            } else {
                if (startIndex != static_cast<ui32>(-1)) {
                    dirtyBlocks.push_back(TRange{startIndex, count});
                    count = 0;
                    startIndex = static_cast<ui32>(-1);
                }
            }

            len -= Options.BlockSize;
            offset += Options.BlockSize;
        }

        if (startIndex != static_cast<ui32>(-1)) {
            dirtyBlocks.push_back(TRange{startIndex, count});
        }
    }

    void PrinterThread()
    {
        TVector<TPrintTask> postponed;

        ui64 totalDirtyBlocks = 0;
        ui32 nextSeqNo = 0;

        TRange dirty = {};

        auto print = [] (auto& d) {
            Cout << d.StartIndex << " - " << (d.StartIndex + d.BlockCount - 1)
                << " : " << d.BlockCount << '\n';
            Cout.Flush();
        };

        while (auto task = PrintQueue.Pop()) {
            if (task->SeqNo == static_cast<ui32>(-1)) {
                break;
            }

            postponed.push_back(std::move(*task));

            if (postponed.back().SeqNo != nextSeqNo) {
                continue;
            }

            SortBy(postponed, [](auto& task) { return task.SeqNo; });

            auto it = postponed.begin();
            for (; it != postponed.end(); ++it) {
                if (it->SeqNo != nextSeqNo) {
                    break;
                }

                ++nextSeqNo;

                if (it->DirtyBlocks.empty()) {
                    continue;
                }

                for (auto [index, len]: it->DirtyBlocks) {
                    totalDirtyBlocks += len;

                    if (!dirty.BlockCount) {
                        dirty = {index, len};
                        continue;
                    }

                    if (dirty.StartIndex + dirty.BlockCount != index) {
                        print(dirty);
                        dirty = {index, len};
                        continue;
                    }
                    dirty.BlockCount += len;
                }
            }

            postponed.erase(postponed.begin(), it);
        }

        if (dirty.BlockCount) {
            print(dirty);
        }

        Y_VERIFY(postponed.empty());

        Cout << totalDirtyBlocks << '\n';
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
