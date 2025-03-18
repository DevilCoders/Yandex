#include <library/cpp/malloc/calloc/calloc_profile_scan/process_state.h>
#include <library/cpp/malloc/calloc/calloc_profile_scan/stack_decoder.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/malloc/calloc/alloc_header.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/unordered_map.h>

#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/system/info.h>

#include <sys/ptrace.h>
#include <sys/wait.h>

struct TCallocStats {
    ui64 Count = 0;
    ui64 Size = 0;
};

Y_DECLARE_PODTYPE(TCallocStats);

using TCallocProfile = NMms::TUnorderedMap<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>, TCallocStats>;

EProcessState GetProcessState(const pid_t pid) {
    const TString processStatPath = TStringBuilder() << "/proc/" << pid << "/stat";
    const TString processStat = TFileInput(processStatPath).ReadAll();
    constexpr size_t STAT_INDEX_STATE = 2;
    Y_ENSURE(processStat);
    TVector<TStringBuf> processStatValues;
    StringSplitter(processStat).Split(' ').Collect(&processStatValues);
    Y_ENSURE(processStatValues.size() > STAT_INDEX_STATE);
    return FromString<EProcessState>(processStatValues[STAT_INDEX_STATE]);
}

void WaitForStatus(const pid_t pid, const EProcessState processState) {
    while (GetProcessState(pid) != processState) {
        Sleep(TDuration::MilliSeconds(10));
    }
}

class TProcessPause {
public:
    explicit TProcessPause(pid_t pid)
        : Pid(pid)
    {
        SuspendProcess(Pid);
    }

    ~TProcessPause() {
        try {
            ResumeProcess(Pid);
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
        }
    }

private:
    void SuspendProcess(pid_t pid) {
        Y_ENSURE_EX(kill(pid, SIGSTOP) == 0, TSystemError() << "SIGSTOP failed");
        WaitForStatus(pid, EProcessState::STOPPED);
    }

    void ResumeProcess(pid_t pid) {
        Y_ENSURE_EX(kill(pid, SIGCONT) == 0, TSystemError() << "SIGCONT failed");
    }

    pid_t Pid;
};

struct TMemoryRegion {
    ui64 Offset;
    ui64 Size;
};

TVector<TMemoryRegion> EnumAnonymousRegions(pid_t pid) {
    const TString mapsFilePath = TStringBuilder() << "/proc/" << pid << "/maps";
    TVector<TMemoryRegion> result;
    TFileInput fi(mapsFilePath);
    TString line;
    TVector<TStringBuf> cols;
    while (fi.ReadLine(line)) {
        StringSplitter(line).Split(' ').SkipEmpty().Collect(&cols);
        if (cols.size() >= 6) {
            // named mapping
            continue;
        }
        const TStringBuf permissions = cols[1];
        if (!permissions.StartsWith(TStringBuf("r"))) {
            // no read access
            continue;
        }
        TMemoryRegion memoryRegion;
        const TStringBuf addressRange = cols[0];
        TStringBuf addrBegin, addrEnd;
        Split(addressRange, '-', addrBegin, addrEnd);
        memoryRegion.Offset = IntFromString<ui64, 16>(addrBegin);
        memoryRegion.Size = IntFromString<ui64, 16>(addrEnd) - memoryRegion.Offset;
        result.push_back(std::move(memoryRegion));
    }
    return result;
}

using TAllocHeadersCallback = std::function<void(void* dataPtr, const TStringBuf stackTrace)>;
constexpr size_t MAX_ALLOC_HEADER_SIZE = NCalloc::STACK_TRACE_BUFFER_SIZE + 1 + sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE) + sizeof(NCalloc::NPrivate::TAllocHeader);

void FindAllocHeaders(TFile& file, const ui64 offset, const ui64 size, const TAllocHeadersCallback& callback) {
    TTempBuf buffer(NSystemInfo::GetPageSize() * 1000);
    const ui64 readLimit = offset + size;
    ui64 readPos = offset;
    while (readPos < readLimit) {
        const ui64 readSize = Min(buffer.Size(), readLimit - readPos);
        Y_ENSURE(file.Pread(buffer.Data(), readSize, readPos) == readSize);
        size_t findPos = 0;
        size_t maxFindPos = 0;
        while (findPos < readSize) {
            findPos = TStringBuf(buffer.Data(), readSize).find(TStringBuf(NCalloc::CALLOC_PROFILER_SIGNATURE, sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE)), findPos);
            if (findPos == TStringBuf::npos) {
                break;
            }
            if (findPos) {
                const ui8 stackTraceSize = buffer.Data()[findPos - 1];
                if (findPos >= 1 + stackTraceSize) {
                    const TStringBuf stackTrace(buffer.Data() + findPos - 1 - stackTraceSize, stackTraceSize);
                    if (findPos + sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE) + sizeof(NCalloc::NPrivate::TAllocHeader) <= readSize) {
                        void* dataPtr = buffer.Data() + findPos + sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE) + sizeof(NCalloc::NPrivate::TAllocHeader);
                        callback(dataPtr, stackTrace);
                        maxFindPos = findPos + sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE) + sizeof(NCalloc::NPrivate::TAllocHeader);
                    }
                }
            }
            findPos += sizeof(NCalloc::CALLOC_PROFILER_SIGNATURE) + sizeof(NCalloc::NPrivate::TAllocHeader);
        }
        if (readSize != buffer.Size()) {
            break;
        }
        readPos = readPos + Max(maxFindPos, buffer.Size() - (MAX_ALLOC_HEADER_SIZE - 1));
    }
}

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    pid_t processPid;
    TString outputFilePath;
    opts.AddLongOption("pid", "Process pid").Required().StoreResult(&processPid);
    opts.AddLongOption("output", "Output file path").Required().StoreResult(&outputFilePath);
    NLastGetopt::TOptsParseResult(&opts, argc, argv);

    using TUniqueStacks = THashMap<TString, TCallocStats>;
    TUniqueStacks uniqueStacks;

    const bool isTTY = isatty(fileno(stdout));

    TProcessPause processPause(processPid);
    const TString processMemoryFilePath = TStringBuilder() << "/proc/" << processPid << "/mem";
    TFile processMemoryFile(processMemoryFilePath, OpenExisting | RdOnly);
    for (const TMemoryRegion& anonMemoryRegion : EnumAnonymousRegions(processPid)) {
        FindAllocHeaders(
            processMemoryFile,
            anonMemoryRegion.Offset,
            anonMemoryRegion.Size,
            [&uniqueStacks](void* dataPtr, const TStringBuf stackTrace) {
                TCallocStats& callocStats = uniqueStacks[stackTrace];
                callocStats.Count += 1;
                callocStats.Size += NCalloc::NPrivate::TAllocHeader::GetAllocSize(dataPtr);
            }
        );
        if (isTTY) {
            Cout << '.' << Flush;
        }
    }
    if (isTTY) {
        Cout << '\n';
    }

    TCallocProfile callocProfile;
    for (const auto& [stackTrace, callocStats] : uniqueStacks) {
        const ui8* stackBuffer = (const ui8*)stackTrace.data();
        const ui8* endOfStackBuffer = stackBuffer + stackTrace.size();
        i64 prevRip = NCalloc::DIFF_ENCODING_START;
        TStringBuilder stackText;
        while (stackBuffer != endOfStackBuffer) {
            const ui64 zigZaggedRipDiff = VarIntDecode(&stackBuffer, endOfStackBuffer - stackBuffer);
            if (!zigZaggedRipDiff) {
                break;
            }
            const i64 ripDiff = ZigZagDecode(zigZaggedRipDiff - 1);
            const i64 rip = prevRip + ripDiff;
            prevRip = rip;
            stackText << Hex(rip) << '\n';
        }
        callocProfile[stackText] = callocStats;
    }
    TFileOutput out(outputFilePath);
    NMms::Write(out, callocProfile);

    return 0;
}
