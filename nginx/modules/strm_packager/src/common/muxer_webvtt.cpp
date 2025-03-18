#include <nginx/modules/strm_packager/src/base/config.h>
#include <nginx/modules/strm_packager/src/common/muxer_webvtt.h>

#include <util/string/printf.h>

namespace NStrm::NPackager {

    TMuxerWebvtt::TMuxerWebvtt(TRequestWorker& request)
        : Request(request)
    {
    }

    TMuxerFuture TMuxerWebvtt::Make(TRequestWorker& request) {
        return NThreading::MakeFuture<IMuxer*>(request.GetPoolUtil<TMuxerWebvtt>().New(request));
    }

    TRepFuture<TSendData> TMuxerWebvtt::GetData() {
        Y_ENSURE(DataPromise.Initialized());
        return DataPromise.GetFuture();
    }

    void TMuxerWebvtt::SetMediaData(TRepFuture<TMediaData> data) {
        Y_ENSURE(!DataPromise.Initialized());
        DataPromise = TRepPromise<TSendData>::Make();

        constexpr TStringBuf begin = "WEBVTT\n\n";
        DataPromise.PutData(TSendData{
            .Blob = TSimpleBlob((ui8 const*)begin.Data(), begin.Size()),
            .Flush = false});

        RequireData(data, Request).AddCallback(Request.MakeFatalOnException(std::bind(&TMuxerWebvtt::MediaDataCallback, this, std::placeholders::_1)));
    }

    TSimpleBlob TMuxerWebvtt::MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const {
        (void)tracksInfo;
        ythrow THttpError(NGX_HTTP_NOT_FOUND, TLOG_INFO);
    };

    TString TMuxerWebvtt::GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const {
        return IMuxer::GetContentType(tracksInfo) + "/vtt; charset=utf-8";
    }

    void TMuxerWebvtt::MediaDataCallback(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Empty()) {
            DataPromise.Finish();
            return;
        }

        const auto printTs = [](TString& out, Ti64TimeP ts) {
            const i64 ms = ts.MilliSeconds();
            const i64 s = ms / 1000;
            const i64 m = s / 60;
            const i64 h = m / 60;
            fcat(out, "%02ld:%02ld:%02ld.%03ld", h, m % 60, s % 60, ms % 1000);
        };

        const TMediaData& data = dataRef.Data();
        Y_ENSURE(data.TracksSamples.size() == 1);
        Y_ENSURE(std::holds_alternative<TTrackInfo::TSubtitleParams>(data.TracksInfo[0]->Params));

        const TVector<TVector<TSampleData>> tracks = RemoveOverlappingDts(data.TracksSamples);

        if (tracks[0].empty()) {
            return;
        }

        TString buffer;

        for (const TSampleData& sd : tracks[0]) {
            Y_ENSURE(sd.DataParams.Format == TSampleData::TDataParams::EFormat::SubtitleRawText);

            if (sd.DataSize == 0) {
                continue;
            }

            printTs(buffer, sd.Dts);
            fcat(buffer, " --> ");
            printTs(buffer, sd.Dts + sd.CoarseDuration);
            buffer += "\n";
            buffer += TStringBuf((char const*)sd.Data, sd.DataSize);
            buffer += "\n\n";
        }

        if (buffer.size() == 0) {
            return;
        }

        DataPromise.PutData(TSendData{Request.HangDataInPool(std::move(buffer)), false});
    }

}
