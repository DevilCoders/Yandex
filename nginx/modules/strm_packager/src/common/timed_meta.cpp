#include <nginx/modules/strm_packager/src/common/timed_meta.h>

#include <nginx/modules/strm_packager/src/base/workers.h>

#include <util/generic/deque.h>
#include <util/generic/overloaded.h>
#include <util/stream/printf.h>

namespace NStrm::NPackager {
    class TTimedMetaMaker {
    public:
        TTimedMetaMaker(
            TRequestWorker& request,
            const TIntervalP fullInterval,
            const TRepFuture<TMediaData>& media,
            const TRepFuture<TMetaData>& meta);

        TRepFuture<TMediaData> GetFuture();

    private:
        void AcceptMeta(const TRepFuture<TMetaData>::TDataRef& dataRef);
        void AcceptMedia(const TRepFuture<TMediaData>::TDataRef& dataRef);
        void TrySend();

        static TBuffer MakeId3Data(const TIntervalP fullInterval, const TMetaData::TAdData& ad);

    private:
        TRequestWorker& Request;
        const TIntervalP FullInterval;
        TRepPromise<TMediaData> Promise;

        TTrackInfo const* const TimedMetaTrackInfo;

        bool Error;
        bool MetaFinished;
        bool MediaFinished;

        TMaybe<TMetaData::TAdData> AdData;
        Ti64TimeP MetaEnd;
        TDeque<TMediaData> Media;
        TMaybe<TSimpleBlob> LastId3Data;
    };

    TRepFuture<TMediaData> AddTimedMetaTrack(
        TRequestWorker& request,
        const TIntervalP fullInterval,
        const TRepFuture<TMediaData>& media,
        const TRepFuture<TMetaData>& meta) {
        return request.GetPoolUtil<TTimedMetaMaker>().New(request, fullInterval, media, meta)->GetFuture();
    }

    TTimedMetaMaker::TTimedMetaMaker(
        TRequestWorker& request,
        const TIntervalP fullInterval,
        const TRepFuture<TMediaData>& media,
        const TRepFuture<TMetaData>& meta)
        : Request(request)
        , FullInterval(fullInterval)
        , Promise(TRepPromise<TMediaData>::Make())
        , TimedMetaTrackInfo(request.GetPoolUtil<TTrackInfo>().New(TTrackInfo{
              .Params = TTrackInfo::TTimedMetaId3Params(),
          }))
        , Error(false)
        , MetaFinished(false)
        , MediaFinished(false)
        , MetaEnd(fullInterval.Begin)
    {
        meta.AddCallback(request.MakeFatalOnException(std::bind(&TTimedMetaMaker::AcceptMeta, this, std::placeholders::_1)));
        media.AddCallback(request.MakeFatalOnException(std::bind(&TTimedMetaMaker::AcceptMedia, this, std::placeholders::_1)));
    }

    TRepFuture<TMediaData> TTimedMetaMaker::GetFuture() {
        return Promise.GetFuture();
    }

    void TTimedMetaMaker::AcceptMeta(const TRepFuture<TMetaData>::TDataRef& dataRef) {
        if (Error) {
            return;
        }

        if (dataRef.Exception()) {
            Error = true;
            Promise.FinishWithException(dataRef.Exception());
            return;
        }

        if (dataRef.Empty()) {
            MetaFinished = true;
        } else {
            const TMetaData& data = dataRef.Data();
            Y_ENSURE(data.Interval.Begin == MetaEnd);
            Y_ENSURE(data.Interval.End <= FullInterval.End);

            MetaEnd = data.Interval.End;

            std::visit(
                TOverloaded{
                    [](const TMetaData::TSourceData&) {
                        // nothing
                    },
                    [this](const TMetaData::TAdData& adData) {
                        AdData = adData;
                    }},
                data.Data);
        }

        TrySend();
    }

    void TTimedMetaMaker::AcceptMedia(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (Error) {
            return;
        }

        if (dataRef.Exception()) {
            Error = true;
            Promise.FinishWithException(dataRef.Exception());
            return;
        }

        if (dataRef.Empty()) {
            MediaFinished = true;
        } else {
            Media.push_back(dataRef.Data());
            TMediaData& data = Media.back();

            data.TracksInfo.push_back(TimedMetaTrackInfo);
            data.TracksSamples.emplace_back();
        }

        TrySend();
    }

    void TTimedMetaMaker::TrySend() {
        while (!Media.empty() && MetaEnd >= Media.front().Interval.End) {
            TMediaData data = std::move(Media.front());
            Media.pop_front();

            if (AdData.Defined()) {
                TBuffer id3Data = MakeId3Data(FullInterval, *AdData);

                // do not send again the same meta
                if (LastId3Data.Empty() || !std::equal(LastId3Data->begin(), LastId3Data->end(), id3Data.Begin(), id3Data.End())) {
                    TSimpleBlob blob = Request.HangDataInPool(std::move(id3Data));
                    LastId3Data = blob;

                    data.TracksSamples.back().emplace_back(TSampleData{
                        .Dts = data.Interval.Begin,
                        .Cto = Ti32TimeP(0),
                        .CoarseDuration = Ti32TimeP(1),
                        .Flags = TSampleData::KeyframeFlags,
                        .DataSize = (ui32)blob.size(),
                        .DataParams = TSampleData::TDataParams{
                            .Format = TSampleData::TDataParams::EFormat::TimedMetaId3,
                        },
                        .Data = blob.data(),
                        .DataFuture = NThreading::MakeFuture(),
                    });
                }
            }

            Promise.PutData(std::move(data));
        }

        if (MetaFinished && MediaFinished) {
            Y_ENSURE(Media.empty());
            Promise.Finish();
        }
    }

    TBuffer TTimedMetaMaker::MakeId3Data(const TIntervalP fullInterval, const TMetaData::TAdData& ad) {
        NMP4Muxer::TBufferWriter writer;

        // ID3 signature, version, and flags
        const ui8 bytesPack1[] = {'I', 'D', '3', 4, 0, 0};
        writer.WorkIO<sizeof(bytesPack1)>(bytesPack1);

        const ui64 size1pos = writer.GetPosition();
        writer.SetPosition(size1pos + 4); // skip 4 bytes for size
        const ui64 size1begin = size1pos + 4;

        writer.WorkIO<4>("TEXT");

        const ui64 size2pos = writer.GetPosition();
        writer.SetPosition(size2pos + 4); // skip 4 bytes for size
        const ui64 size2begin = size2pos + 4 + 2;

        // 2 bytes flags, and byte for encoding (3 mean utf-8)
        const ui8 bytesPack2[] = {0, 0, 3};
        writer.WorkIO<sizeof(bytesPack2)>(bytesPack2);

        NMP4Muxer::TOutputStreamAdapter stream(writer);

        Printf(stream, "{\"cue_id\":\"%s\"", ad.Id.Data());
        Printf(stream, ",\"id\":\"%s\"", ad.Id.Data());
        Printf(stream, ",\"ad_type\":\"%s\"", ToString(ad.Type).Data());

        if (ad.ImpId.Defined()) {
            Printf(stream, ",\"ad_imp_id\":\"%s\"", ad.ImpId->Data());
        }

        Printf(stream, ",\"offset\":%" PRId64, (ad.BlockBegin - fullInterval.Begin).MilliSeconds());

        if (ad.BlockEnd.Defined()) {
            Printf(stream, ",\"cutoff\":%" PRId64, (*ad.BlockEnd - fullInterval.Begin).MilliSeconds());
        }

        // TODO: get vast for s2s type
        Printf(stream, ",\"vast\":\"<VAST></VAST>\"");

        const bool gzip = false; // looks like this flag is about VAST

        if (gzip) {
            Printf(stream, ",\"gzip\":true}");
        } else {
            Printf(stream, "}");
        }

        const ui64 endPos = writer.GetPosition();

        const auto writeSize = [&writer](const ui32 size) {
            WriteUI8(writer, (size >> 21) & 0x7f);
            WriteUI8(writer, (size >> 14) & 0x7f);
            WriteUI8(writer, (size >> 7) & 0x7f);
            WriteUI8(writer, (size >> 0) & 0x7f);
        };

        writer.SetPosition(size2pos);
        writeSize(endPos - size2begin);

        writer.SetPosition(size1pos);
        writeSize(endPos - size1begin);

        return std::move(writer.Buffer());
    }

}
