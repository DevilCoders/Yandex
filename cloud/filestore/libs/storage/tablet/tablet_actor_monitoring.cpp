#include "tablet_actor.h"

#include <cloud/storage/core/libs/common/format.h>

#include <library/cpp/monlib/service/pages/templates.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/str.h>
#include <util/string/join.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

namespace {

////////////////////////////////////////////////////////////////////////////////

void DumpOperationState(
    const TString& opName,
    const TOperationState& state,
    IOutputStream& out)
{
    out << opName
        << "State: " << static_cast<ui32>(state.GetOperationState())
        << ", Ts: " << state.GetStateChanged()
        << ", Completed: " << state.GetCompleted()
        << ", Failed: " << state.GetFailed()
        << ", Backoff: " << state.GetBackoffTimeout().ToString();
}

////////////////////////////////////////////////////////////////////////////////

void DumpCompactionMap(
    const TVector<TCompactionRangeInfo>& ranges,
    IOutputStream& out)
{
    HTML(out) {
        TABLE_SORTABLE_CLASS("table table-bordered") {
            TABLEHEAD() {
                TABLER() {
                    TABLEH() { out << "Range"; }
                    TABLEH() { out << "Blobs"; }
                    TABLEH() { out << "Deletions"; }
                }
            }

            for (const auto& range: ranges) {
                TABLER() {
                    // TODO: link to describe range page that would show the
                    // blocks belonging to this range
                    TABLED() { out << range.RangeId; }
                    TABLED() { out << range.Stats.BlobsCount; }
                    TABLED() { out << range.Stats.DeletionsCount; }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void DumpProfillingAllocatorStats(IOutputStream& out)
{
    HTML(out) {
        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                ui64 allBytes = 0;
                for (ui32 i = 0; i < static_cast<ui32>(EAllocatorTag::Max); ++i) {
                    EAllocatorTag tag = static_cast<EAllocatorTag>(i);
                    const ui64 bytes = GetAllocatorByTag(tag)->GetBytesAllocated();
                    TABLER() {
                        TABLED() { out << tag; }
                        TABLED() { out << FormatByteSize(bytes); }
                    }
                    allBytes += bytes;
                }
                TABLER() {
                    TABLED() { out << "Summary"; }
                    TABLED() { out << FormatByteSize(allBytes); }
                }
            }
        }
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

void TIndexTabletActor::HandleHttpInfo(
    const NMon::TEvRemoteHttpInfo::TPtr& ev,
    const TActorContext& ctx)
{
    auto* msg = ev->Get();
    const auto& params = msg->Cgi();

    LOG_DEBUG(ctx, TFileStoreComponents::TABLET,
        "[%lu] HTTP request: %s",
        TabletID(),
        msg->Query.Quote().data());

    TStringStream out;

    HTML(out) {
        H3() { out << "Info"; }
        DIV() { out << "FileSystemId: " << GetFileSystemId(); }
        DIV() { out << "BlockSize: " << GetBlockSize(); }
        DIV() { out << "Blocks: " << GetBlocksCount() << " (" <<
            FormatByteSize(GetBlocksCount() * GetBlockSize()) << ")";
        }

        H3() { out << "State"; }
        DIV() { out << "CurrentCommitId: " << GetCurrentCommitId(); }
        DIV() { DumpOperationState("Flush", FlushState, out); }
        DIV() { DumpOperationState("BlobIndexOp", BlobIndexOpState, out); }
        DIV() { DumpOperationState("CollectGarbage", CollectGarbageState, out); }

        H3() { out << "Stats"; }
        PRE() { DumpStats(out); }

        const ui32 topSize = FromStringWithDefault(params.Get("top-size"), 1);
        H3() { out << "CompactionMap - ByCompactionScore"; }
        DumpCompactionMap(GetTopRangesByCompactionScore(topSize), out);
        H3() { out << "CompactionMap - ByCleanupScore"; }
        DumpCompactionMap(GetTopRangesByCleanupScore(topSize), out);

        H3() { out << "Profilling allocator stats"; }
        DumpProfillingAllocatorStats(out);
    }

    NCloud::Reply(
        ctx,
        *ev,
        std::make_unique<NMon::TEvRemoteHttpInfoRes>(out.Str()));
}

}   // namespace NCloud::NFileStore::NStorage
