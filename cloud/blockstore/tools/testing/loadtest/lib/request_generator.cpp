#include "request_generator.h"

#include "range_allocator.h"
#include "range_map.h"

#include <cloud/blockstore/libs/diagnostics/events/profile_events.ev.pb.h>
#include <cloud/blockstore/libs/service/request_helpers.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <library/cpp/eventlog/dumper/evlogdump.h>
#include <library/cpp/eventlog/eventlog.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/random/random.h>
#include <util/string/builder.h>

namespace NCloud::NBlockStore::NLoadTest {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TArtificialRequestGenerator final
    : public IRequestGenerator
{
private:
    TLog Log;
    NProto::TRangeTest RangeTest;
    TRangeMap BlocksRange;
    TRangeAllocator RangeAllocator;

    TVector<std::pair<ui64, EBlockStoreRequest>> Rates;
    ui64 TotalRate = 0;

public:
    TArtificialRequestGenerator(
            ILoggingServicePtr logging,
            NProto::TRangeTest range)
        : RangeTest(std::move(range))
        , BlocksRange(TBlockRange64(RangeTest.GetStart(), RangeTest.GetEnd()))
        , RangeAllocator(RangeTest)
    {
        Log = logging->CreateLog(Describe());

        SetupRequestWeights();
    }

    bool Next(TRequest* request) override;
    void Complete(TBlockRange64 blockRange) override;
    TString Describe() const override;
    size_t Size() const override;

private:
    TMaybe<TBlockRange64> AllocateRange();
    void SetupRequestWeights();
    EBlockStoreRequest ChooseRequest() const;
};

////////////////////////////////////////////////////////////////////////////////

TString TArtificialRequestGenerator::Describe() const
{
    return TStringBuilder()
        << "Range[" << RangeTest.GetStart()
        << ',' << RangeTest.GetEnd() << ']';
}

size_t TArtificialRequestGenerator::Size() const
{
    return RangeTest.GetRequestsCount();
}

bool TArtificialRequestGenerator::Next(TRequest* request)
{
    auto blockRange = AllocateRange();
    if (!blockRange.Defined()) {
        STORAGE_WARN(
            "No free blocks found: (%lu) have = %s",
            BlocksRange.Size(),
            BlocksRange.DumpRanges().data());
        return false;
    }

    STORAGE_TRACE(
        "BlockAllocated: (%lu) allocated = %s left = %s",
        BlocksRange.Size(),
        DescribeRange(*blockRange).data(),
        BlocksRange.DumpRanges().data());

    request->RequestType = ChooseRequest();
    request->BlockRange = *blockRange;

    return true;
}

void TArtificialRequestGenerator::Complete(TBlockRange64 blockRange)
{
    STORAGE_TRACE(
        "Block Deallocated: (%lu) %s \n%s",
        BlocksRange.Size(),
        DescribeRange(blockRange).data(),
        BlocksRange.DumpRanges().data());

    BlocksRange.PutBlock(blockRange);
}

void TArtificialRequestGenerator::SetupRequestWeights()
{
    if (RangeTest.GetWriteRate()) {
        TotalRate += RangeTest.GetWriteRate();
        Rates.emplace_back(TotalRate, EBlockStoreRequest::WriteBlocks);
    }

    if (RangeTest.GetReadRate()) {
        TotalRate += RangeTest.GetReadRate();
        Rates.emplace_back(TotalRate, EBlockStoreRequest::ReadBlocks);
    }

    if (RangeTest.GetZeroRate()) {
        TotalRate += RangeTest.GetZeroRate();
        Rates.emplace_back(TotalRate, EBlockStoreRequest::ZeroBlocks);
    }
}

EBlockStoreRequest TArtificialRequestGenerator::ChooseRequest() const
{
    auto it = LowerBound(
        Rates.begin(),
        Rates.end(),
        RandomNumber(TotalRate),
        [] (const auto& a, const auto& b) { return a.first < b; });

    auto offset = std::distance(Rates.begin(), it);
    return Rates[offset].second;
}

TMaybe<TBlockRange64> TArtificialRequestGenerator::AllocateRange()
{
    return BlocksRange.GetBlock(
        RangeAllocator.AllocateRange(),
        RangeTest.GetLoadType() == NProto::LOAD_TYPE_SEQUENTIAL);
}

////////////////////////////////////////////////////////////////////////////////

class TRealRequestGenerator final
    : public IRequestGenerator
{
private:
    TLog Log;

    TString ProfileLogPath;
    TString DiskId;
    bool FullSpeed;

    struct TRequestWithVersion
    {
        ui64 Version = 0;
        NProto::TProfileLogRequestInfo Request;

        bool operator<(const TRequestWithVersion& rhs) const
        {
            return Version == rhs.Version
                ? Request.GetTimestampMcs() < rhs.Request.GetTimestampMcs()
                : Version < rhs.Version;
        }
    };

    TVector<TRequestWithVersion> Requests;
    mutable TInstant StartTs;
    mutable TInstant FirstRequestTs;
    mutable ui64 CurrentLogVersion = 0;
    ui32 CurrentRequestIdx = 0;

    struct TCompareByEnd
    {
        using is_transparent = void;

        bool operator()(const auto& lhs, const auto& rhs) const
        {
            return GetEnd(lhs) < GetEnd(rhs);
        }

        static ui64 GetEnd(const TBlockRange64& blockRange)
        {
            return blockRange.End;
        }

        static ui64 GetEnd(ui64 End)
        {
            return End;
        }
    };

    TSet<TBlockRange64, TCompareByEnd> InFlight;

public:
    TRealRequestGenerator(
            ILoggingServicePtr logging,
            TString profileLogPath,
            TString diskId,
            bool fullSpeed)
        : ProfileLogPath(std::move(profileLogPath))
        , DiskId(std::move(diskId))
        , FullSpeed(fullSpeed)
    {
        Log = logging->CreateLog(Describe());

        ReadRequests();
    }

    bool Next(TRequest* request) override;
    TInstant Peek() override;
    void Complete(TBlockRange64 blockRange) override;
    TString Describe() const override;
    size_t Size() const override;

private:
    static EBlockStoreRequest RequestType(ui32 intType)
    {
        return static_cast<EBlockStoreRequest>(intType);
    }

    TInstant Timestamp(const TRequestWithVersion& r) const;
    void ReadRequests();
};

////////////////////////////////////////////////////////////////////////////////

TString TRealRequestGenerator::Describe() const
{
    return TStringBuilder()
        << "LogReplay[" << ProfileLogPath << "]";
}

size_t TRealRequestGenerator::Size() const
{
    return Requests.size();
}

bool TRealRequestGenerator::Next(TRequest* request)
{
    if (CurrentRequestIdx == Requests.size()) {
        return false;
    }

    const auto& r = Requests[CurrentRequestIdx];

    if (!FullSpeed) {
        auto now = Now();
        if (now < Timestamp(r)) {
            return false;
        }
    }

    request->RequestType = RequestType(r.Request.GetRequestType());
    if (r.Request.GetRanges().empty()) {
        request->BlockRange = TBlockRange64::WithLength(
            r.Request.GetBlockIndex(),
            r.Request.GetBlockCount());
    } else {
        request->BlockRange = TBlockRange64::WithLength(
            r.Request.GetRanges(0).GetBlockIndex(),
            r.Request.GetRanges(0).GetBlockCount());
    }

    if (FullSpeed) {
        auto it = InFlight.lower_bound(request->BlockRange.Start);
        if (it != InFlight.end() && it->Start <= request->BlockRange.End) {
            return false;
        }
    }

    InFlight.insert(request->BlockRange);

    ++CurrentRequestIdx;
    return true;
}

TInstant TRealRequestGenerator::Peek()
{
    return !FullSpeed && CurrentRequestIdx < Requests.size()
        ? Timestamp(Requests[CurrentRequestIdx])
        : TInstant::Max();
}

void TRealRequestGenerator::Complete(TBlockRange64 blockRange)
{
    InFlight.erase(blockRange);
}

TInstant TRealRequestGenerator::Timestamp(
    const TRequestWithVersion& r) const
{
    auto requestTs = TInstant::MicroSeconds(r.Request.GetTimestampMcs());

    if (CurrentLogVersion != r.Version || StartTs == TInstant::Zero()) {
        CurrentLogVersion = r.Version;
        StartTs = Now();
        FirstRequestTs = requestTs;
    }

    return StartTs + (requestTs - FirstRequestTs);
}

void TRealRequestGenerator::ReadRequests()
{
    struct TEventProcessor
        : TProtobufEventProcessor
    {
        TRealRequestGenerator& Parent;

        TEventProcessor(TRealRequestGenerator& parent)
            : Parent(parent)
        {
        }

        void DoProcessEvent(const TEvent* ev, IOutputStream* out) override
        {
            Y_UNUSED(out);

            auto message =
                dynamic_cast<const NProto::TProfileLogRecord*>(ev->GetProto());

            if (message && message->GetDiskId() == Parent.DiskId) {
                const auto s = Parent.Requests.size();

                for (const auto& r: message->GetRequests()) {
                    if (IsReadWriteRequest(RequestType(r.GetRequestType()))) {
                        Parent.Requests.push_back({ev->Timestamp, r});
                    }
                }

                Sort(Parent.Requests.begin() + s, Parent.Requests.end());
            }
        }
    };

    TEventProcessor eventProcessor(*this);

    const char* argv[] = {"foo", ProfileLogPath.c_str()};
    auto code = IterateEventLog(NEvClass::Factory(), &eventProcessor, 2, argv);
    if (code) {
        ythrow yexception()
            << "profile log iteration has failed, code " << code;
    }

    if (Requests.empty()) {
        ythrow yexception() << "profile log is empty";
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IRequestGeneratorPtr CreateArtificialRequestGenerator(
    ILoggingServicePtr loggingService,
    NProto::TRangeTest range)
{
    return std::make_shared<TArtificialRequestGenerator>(
        std::move(loggingService),
        std::move(range));
}

IRequestGeneratorPtr CreateRealRequestGenerator(
    ILoggingServicePtr loggingService,
    TString profileLogPath,
    TString diskId,
    bool fullSpeed)
{
    return std::make_shared<TRealRequestGenerator>(
        std::move(loggingService),
        std::move(profileLogPath),
        std::move(diskId),
        fullSpeed);
}

}   // namespace NCloud::NBlockStore::NLoadTest
