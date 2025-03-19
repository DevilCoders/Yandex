#include <cloud/filestore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/filestore/libs/storage/tablet/model/range.h>
#include <cloud/filestore/tools/analytics/libs/event-log/dump.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>
#include <library/cpp/getopt/small/last_getopt.h>

#include <util/generic/size_literals.h>
#include <util/generic/vector.h>

namespace {

using namespace NCloud::NFileStore;
using namespace NCloud::NFileStore::NStorage;

////////////////////////////////////////////////////////////////////////////////

struct TNodeIdWithRange
{
    ui64 NodeId;
    TByteRange Range;

    TNodeIdWithRange(ui64 nodeId, TByteRange range)
        : NodeId(nodeId)
        , Range(range)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    TString EvlogDumperParamsStr;
    TString FileSystemId;
    ui64 DefaultNodeId;
    ui32 BlockSize;
    TVector<TString> ByteRangeStrs;
    TVector<TNodeIdWithRange> Ranges;
    TVector<const char*> EvlogDumperArgv;

    TOptions(int argc, const char** argv)
    {
        using namespace NLastGetopt;

        TOpts opts;
        opts.AddHelpOption();

        opts.AddLongOption("evlog-dumper-params", "evlog dumper param string")
            .Required()
            .RequiredArgument("STR")
            .StoreResult(&EvlogDumperParamsStr);

        opts.AddLongOption("fs-id", "fs-id filter")
            .RequiredArgument("STR")
            .StoreResult(&FileSystemId);

        opts.AddLongOption(
                "default-node-id",
                "default node id, used if NODE_ID is not specified for ranges")
            .RequiredArgument("NODE_ID")
            .DefaultValue(0)
            .StoreResult(&DefaultNodeId);

        opts.AddLongOption("block-size", "fs block size")
            .RequiredArgument("INT")
            .DefaultValue(4_KB)
            .StoreResult(&BlockSize);

        opts.AddLongOption("byte-range", "byte ranges to find")
            .RequiredArgument("[NODE_ID,]START,COUNT")
            .AppendTo(&ByteRangeStrs);

        TOptsParseResultException(&opts, argc, argv);

        EvlogDumperArgv.push_back("fake");

        TStringBuf sit(EvlogDumperParamsStr);
        TStringBuf arg;
        while (sit.NextTok(' ', arg)) {
            if (sit.Size()) {
                const auto idx = EvlogDumperParamsStr.Size() - sit.Size() - 1;
                EvlogDumperParamsStr[idx] = 0;
            }
            EvlogDumperArgv.push_back(arg.Data());
        }

        for (const auto& s: ByteRangeStrs) {
            TStringBuf a, b, c;
            TStringBuf it(s);
            it.NextTok(',', a);
            it.NextTok(',', b);
            it.NextTok(',', c);

            TByteRange range = c ? TByteRange(
                FromString<ui64>(b),
                FromString<ui64>(c),
                BlockSize
            ) : TByteRange(
                FromString<ui64>(a),
                FromString<ui64>(b),
                BlockSize
            );

            Ranges.push_back({c ? FromString<ui64>(a) : DefaultNodeId, range});
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

class TEventProcessor final
    : public TProtobufEventProcessor
{
private:
    const TOptions& Options;

public:
    TEventProcessor(const TOptions& options)
        : Options(options)
    {
    }

protected:
    void DoProcessEvent(const TEvent* ev, IOutputStream* out) override
    {
        auto* message =
            dynamic_cast<const NProto::TProfileLogRecord*>(ev->GetProto());
        if (message && (message->GetFileSystemId() == Options.FileSystemId
                || !Options.FileSystemId))
        {
            auto order = GetItemOrder(*message);

            for (const auto i: order) {
                ProcessRequest(*message, i, out);
            }
        }
    }

private:
    void ProcessRequest(
        const NProto::TProfileLogRecord& record,
        int i,
        IOutputStream* out)
    {
        const auto& r = record.GetRequests(i);

        for (const auto& nr: Options.Ranges) {
            bool found = false;

            for (const auto& range: r.GetRanges()) {
                if (range.GetNodeId() != nr.NodeId) {
                    continue;
                }

                auto reqRange = TByteRange(
                    range.GetOffset(),
                    range.GetBytes(),
                    Options.BlockSize
                );

                if (reqRange.Overlaps(nr.Range)) {
                    found = true;
                    break;
                }
            }

            if (found) {
                DumpRequest(record, i, out);
                break;
            }
        }
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int main(int argc, const char** argv)
{
    TOptions options(argc, argv);
    TEventProcessor processor(options);

    return IterateEventLog(
        NEvClass::Factory(),
        &processor,
        options.EvlogDumperArgv.size(),
        options.EvlogDumperArgv.begin()
    );
}
