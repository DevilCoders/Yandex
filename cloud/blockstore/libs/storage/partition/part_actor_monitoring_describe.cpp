#include "part_actor.h"

#include <cloud/blockstore/libs/storage/core/monitoring_utils.h>
#include <cloud/blockstore/libs/storage/core/probes.h>

#include <library/cpp/monlib/service/pages/templates.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/string.h>
#include <util/stream/str.h>

namespace NCloud::NBlockStore::NStorage::NPartition {

using namespace NKikimr;
using namespace NKikimr::NTabletFlatExecutor;

LWTRACE_USING(BLOCKSTORE_STORAGE_PROVIDER);

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDescribeRangeVisitor final
    : public IFreshBlocksIndexVisitor
    , public IBlocksIndexVisitor
{
private:
    TTxPartition::TDescribeRange& Args;

public:
    TDescribeRangeVisitor(TTxPartition::TDescribeRange& args)
        : Args(args)
    {}

    bool Visit(const TFreshBlock& block) override
    {
        Args.MarkBlock(
            block.Meta.BlockIndex,
            block.Meta.CommitId,
            {},
            block.Content ? 0 : InvalidBlobOffset
        );

        return true;
    }

    bool Visit(
        ui32 blockIndex,
        ui64 commitId,
        const TPartialBlobId& blobId,
        ui16 blobOffset) override
    {
        Args.MarkBlock(blockIndex, commitId, blobId, blobOffset);
        return true;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TDescribeBlobVisitor final
    : public IBlocksIndexVisitor
{
private:
    TTxPartition::TDescribeBlob& Args;

public:
    TDescribeBlobVisitor(TTxPartition::TDescribeBlob& args)
        : Args(args)
    {}

    bool Visit(
        ui32 blockIndex,
        ui64 commitId,
        const TPartialBlobId& blobId,
        ui16 blobOffset) override
    {
        Y_UNUSED(blobId);

        Args.MarkBlock(blockIndex, commitId, blobOffset);
        return true;
    }
};

}   // namespace


////////////////////////////////////////////////////////////////////////////////

bool TPartitionActor::PrepareDescribeRange(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TDescribeRange& args)
{
    Y_UNUSED(ctx);

    TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    bool ready = true;

    TDescribeRangeVisitor visitor(args);
    if (!args.BlockFilter || args.BlockFilter.find('f') != TString::npos) {
        State->FindFreshBlocks(visitor, args.BlockRange);
    }
    if (!args.BlockFilter || args.BlockFilter.find('m') != TString::npos) {
        ready &= db.FindMixedBlocks(
            visitor,
            args.BlockRange,
            true    // precharge
        );
    }
    if (!args.BlockFilter || args.BlockFilter.find('M') != TString::npos) {
        ready &= db.FindMergedBlocks(
            visitor,
            args.BlockRange,
            true,   // precharge
            State->GetMaxBlocksInBlob()
        );
    }

    return ready;
}

void TPartitionActor::ExecuteDescribeRange(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TDescribeRange& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TPartitionActor::CompleteDescribeRange(
    const TActorContext& ctx,
    TTxPartition::TDescribeRange& args)
{
    using namespace NMonitoringUtils;

    auto comparer = [] (
        const TTxPartition::TDescribeRange::TBlockMark& l,
        const TTxPartition::TDescribeRange::TBlockMark& r)
    {
        if (l.BlockIndex == r.BlockIndex) {
            if (l.CommitId == r.CommitId) {
                return l.BlobOffset < r.BlobOffset;
            } else {
                // last entries goes first
                return l.CommitId > r.CommitId;
            }
        } else {
            return l.BlockIndex < r.BlockIndex;
        }
    };

    Sort(args.BlockMarks, comparer);

    TStringStream out;
    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);
    DumpDescribeHeader(out, *Info());

    HTML(out) {
        const auto& cm = State->GetCompactionMap();
        const auto groupStart =
            cm.GetGroupStart(args.BlockRange.Start, State->GetBlockSize());

        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                TABLER() {
                    TABLED() { out << "GroupStart"; }
                    TABLED() {
                        out << groupStart;
                    }
                }
            }
        }

        auto outputStat = [&] (const auto& rangeStat, ui32 blockIndex) {
            TABLE_CLASS("table table-condensed") {
                TABLEBODY() {
                    TABLER() {
                        TABLED() { out << "BlockIndex"; }
                        TABLED() {
                            out << blockIndex;
                        }
                    }
                    TABLER() {
                        TABLED() { out << "BlobCount"; }
                        TABLED() { out << rangeStat.BlobCount; }
                    }
                    TABLER() {
                        TABLED() { out << "BlockCount"; }
                        TABLED() { out << rangeStat.BlockCount; }
                    }
                    TABLER() {
                        TABLED() { out << "UsedBlockCount"; }
                        TABLED() { out << rangeStat.UsedBlockCount; }
                    }
                    TABLER() {
                        TABLED() { out << "ReadRequestCount"; }
                        TABLED() { out << rangeStat.ReadRequestCount; }
                    }
                    TABLER() {
                        TABLED() { out << "ReadRequestBlobCount"; }
                        TABLED() { out << rangeStat.ReadRequestBlobCount; }
                    }
                    TABLER() {
                        TABLED() { out << "ReadRequestBlockCount"; }
                        TABLED() { out << rangeStat.ReadRequestBlockCount; }
                    }
                    TABLER() {
                        TABLED() { out << "Compacted"; }
                        TABLED() { out << rangeStat.Compacted; }
                    }
                    TABLER() {
                        TABLED() { out << "Score"; }
                        TABLED() { out << rangeStat.Score; }
                    }
                }
            }
        };

        H3() { out << "RangeStat (current)"; }
        outputStat(cm.Get(args.BlockRange.Start), args.BlockRange.Start);
        H3() { out << "RangeStat (top by garbage)"; }
        const auto& topByGarbage = cm.GetTopByGarbageBlockCount();
        outputStat(topByGarbage.Stat, topByGarbage.BlockIndex);

        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "# Block"; }
                    TABLED() { out << "CommitId"; }
                    TABLED() { out << "BlobId"; }
                    TABLED() { out << "Offset"; }
                }
            }
            TABLEBODY() {
                auto dump = [&] (const TTxPartition::TDescribeRange::TBlockMark& mark) {
                    TABLER() {
                        TABLED_CLASS("view") { DumpBlockIndex(out, *Info(), mark.BlockIndex, mark.CommitId); }
                        TABLED() { DumpCommitId(out, mark.CommitId); }
                        TABLED_CLASS("view") { DumpBlobId(out, *Info(), mark.BlobId); }
                        TABLED() { DumpBlobOffset(out, mark.BlobOffset); }
                    }
                };

                size_t count = 0;
                for (const auto& mark: args.BlockMarks) {
                    dump(mark);
                    if (++count == State->GetMaxBlocksInBlob()) {
                        break;
                    }
                }
            }
        }
    }

    GenerateBlobviewJS(out);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(args.RequestInfo->TraceId));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

bool TPartitionActor::PrepareDescribeBlob(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TDescribeBlob& args)
{
    Y_UNUSED(ctx);

    TRequestScope timer(*args.RequestInfo);
    TPartitionDatabase db(tx.DB);

    TDescribeBlobVisitor visitor(args);
    return db.FindBlocksInBlobsIndex(visitor, args.BlobId);
}

void TPartitionActor::ExecuteDescribeBlob(
    const TActorContext& ctx,
    TTransactionContext& tx,
    TTxPartition::TDescribeBlob& args)
{
    Y_UNUSED(ctx);
    Y_UNUSED(tx);
    Y_UNUSED(args);
}

void TPartitionActor::CompleteDescribeBlob(
    const TActorContext& ctx,
    TTxPartition::TDescribeBlob& args)
{
    using namespace NMonitoringUtils;

    TStringStream out;
    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);
    DumpDescribeHeader(out, *Info());

    HTML(out) {
        TABLE_SORTABLE() {
            TABLEHEAD() {
                TABLER() {
                    TABLED() { out << "# Block"; }
                    TABLED() { out << "Offset"; }
                }
            }
            TABLEBODY() {
                auto dump = [&] (const TTxPartition::TDescribeBlob::TBlockMark& mark) {
                    TABLER() {
                        TABLED_CLASS("view") { DumpBlockIndex(out, *Info(), mark.BlockIndex, mark.CommitId); }
                        TABLED() { DumpBlobOffset(out, mark.BlobOffset); }
                    }
                };

                size_t count = 0;
                for (const auto& mark: args.BlockMarks) {
                    dump(mark);
                    if (++count == State->GetMaxBlocksInBlob()) {
                        break;
                    }
                }
            }
        }
    }
    GenerateBlobviewJS(out);

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &args.RequestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(args.RequestInfo->TraceId));

    NCloud::Reply(ctx, *args.RequestInfo, std::move(response));
}

////////////////////////////////////////////////////////////////////////////////

void TPartitionActor::HandleHttpInfo_Describe(
    const TActorContext& ctx,
    TRequestInfoPtr requestInfo,
    const TCgiParameters& params)
{
    if (const auto& range = params.Get("range")) {
        TBlockRange32 blockRange;
        if (TBlockRange32::TryParse(range, blockRange)) {
            ExecuteTx<TDescribeRange>(
                ctx,
                std::move(requestInfo),
                blockRange,
                params.Get("blockfilter"));
        } else {
            auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(
                "invalid range specified: " + range.Quote());

            BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

            LWPROBE(
                ResponseSent_Partition,
                "HttpInfo",
                GetRequestId(requestInfo->TraceId));

            NCloud::Reply(ctx, *requestInfo, std::move(response));
        }
        return;
    }

    if (const auto& blob = params.Get("blob")) {
        TLogoBlobID blobId;
        TString errorExplanation;
        if (TLogoBlobID::Parse(blobId, blob, errorExplanation)) {
            ExecuteTx<TDescribeBlob>(
                ctx,
                std::move(requestInfo),
                MakePartialBlobId(blobId));
        } else {
            auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(
                "invalid blob specified: " + blob.Quote() +
                "(" + errorExplanation + ")");

            BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

            LWPROBE(
                ResponseSent_Partition,
                "HttpInfo",
                GetRequestId(requestInfo->TraceId));

            NCloud::Reply(ctx, *requestInfo, std::move(response));
        }
        return;
    }

    using namespace NMonitoringUtils;

    TStringStream out;
    DumpDefaultHeader(out, *Info(), SelfId().NodeId(), *DiagnosticsConfig);
    DumpDescribeHeader(out, *Info());

    auto response = std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str());

    BLOCKSTORE_TRACE_SENT(ctx, &requestInfo->TraceId, this, response);

    LWPROBE(
        ResponseSent_Partition,
        "HttpInfo",
        GetRequestId(requestInfo->TraceId));

    NCloud::Reply(ctx, *requestInfo, std::move(response));
}

}   // namespace NCloud::NBlockStore::NStorage::NPartition
