#include <nginx/modules/strm_packager/src/common/source_webvtt_file.h>

#include <util/string/cast.h>
#include <util/string/split.h>

namespace NStrm::NPackager {
    TSourceFuture TSourceWebvttFile::Make(
        TRequestWorker& request,
        const TString& uri,
        const TString& args,
        const TIntervalP cropInterval,
        const TMaybe<Ti64TimeP> offset) {
        if (cropInterval.End <= cropInterval.Begin) {
            return NThreading::MakeFuture<ISource*>(request.GetPoolUtil<TSourceWebvttFile>().New(
                request,
                TStringBuf(),
                cropInterval,
                offset.GetOrElse(Ti64TimeP(0))));
        }

        NThreading::TFuture<TBuffer> future = request.CreateSubrequest(
            TSubrequestParams{
                .Uri = uri,
                .Args = args,
            },
            NGX_HTTP_OK);

        return future.Apply([future, &request, cropInterval, offset](const NThreading::TFuture<TBuffer>&) mutable {
            future.TryRethrow();

            const TSimpleBlob data = request.HangDataInPool(future.ExtractValue());

            return (ISource*)request.GetPoolUtil<TSourceWebvttFile>().New(
                request,
                TStringBuf((char const*)data.data(), data.size()),
                cropInterval,
                offset.GetOrElse(Ti64TimeP(0)));
        });
    }

    TSourceWebvttFile::TSourceWebvttFile(
        TRequestWorker& request,
        const TStringBuf fileData,
        const TIntervalP cropInterval,
        const Ti64TimeP offset)
        : ISource(request)
        , Interval{.Begin = cropInterval.Begin + offset, .End = cropInterval.End + offset}
        , Info{Request.GetPoolUtil<TTrackInfo>().New(TTrackInfo{
              .Params = TTrackInfo::TSubtitleParams{
                  .Type = TTrackInfo::TSubtitleParams::EType::RawText,
              },
          })}
    {
        Y_ENSURE(cropInterval.End >= cropInterval.Begin);

        Data = TRepPromise<TMediaData>::Make();

        TMediaData data = {
            .Interval = Interval,
            .TracksInfo = Info,
        };

        data.TracksSamples.resize(1);
        TVector<TSampleData>& samples = data.TracksSamples[0];

        TVector<TStringBuf> lines, timingsParts, tsParts;
        Split(fileData, "\n\r", lines);

        const auto parseTimestamp = [](const TStringBuf& timestamp, TVector<TStringBuf>& tsParts) {
            Split(timestamp, ":.", tsParts);
            Y_ENSURE(tsParts.size() == 3 || tsParts.size() == 4);

            auto it = tsParts.rbegin();

            const i64 ms = FromString<i64>(*it);
            ++it;
            const i64 s = FromString<i64>(*it);
            ++it;
            const i64 m = FromString<i64>(*it);
            ++it;
            const i64 h = (it != tsParts.rend()) ? FromString<i64>(*it) : 0;

            return (Ti64TimeP)Ti64TimeMs(ms + 1000 * (s + 60 * (m + 60 * h)));
        };

        const NThreading::TFuture<void> readyFuture = NThreading::MakeFuture();

        constexpr char emptyData[] = "";

        TSampleData* sample = nullptr;
        for (const TStringBuf& line : lines) {
            if (line.find("-->") == TStringBuf::npos) {
                if (!sample) {
                    continue;
                }

                sample->Data = (sample->Data != (ui8 const*)emptyData) ? sample->Data : (ui8 const*)line.data();
                sample->DataSize = line.end() - (char const*)sample->Data;
            } else {
                Split(line, " \t", timingsParts);
                Y_ENSURE(timingsParts.size() >= 3);
                Y_ENSURE(timingsParts[1] == "-->");

                const Ti64TimeP begin = parseTimestamp(timingsParts[0], tsParts);

                if (begin < cropInterval.Begin || begin >= cropInterval.End) {
                    sample = nullptr;
                    continue;
                }

                const Ti64TimeP end = parseTimestamp(timingsParts[2], tsParts);

                samples.push_back(TSampleData{
                    .Dts = begin + offset,
                    .Cto = Ti32TimeP(0),
                    .CoarseDuration = Ti32TimeP(end - begin),
                    .Flags = TSampleData::KeyframeFlags,
                    .DataSize = 0,
                    .DataParams = {
                        .Format = TSampleData::TDataParams::EFormat::SubtitleRawText,
                    },
                    .Data = (ui8 const*)emptyData,
                    .DataFuture = readyFuture,
                });
                sample = &samples.back();
            }
        }

        Data.PutData(std::move(data));
        Data.Finish();
    }

    TIntervalP TSourceWebvttFile::FullInterval() const {
        return Interval;
    }

    TVector<TTrackInfo const*> TSourceWebvttFile::GetTracksInfo() const {
        return Info;
    }

    TRepFuture<TMediaData> TSourceWebvttFile::GetMedia() {
        return Data.GetFuture();
    }

}
