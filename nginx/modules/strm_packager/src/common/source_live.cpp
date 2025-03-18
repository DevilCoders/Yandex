#include <nginx/modules/strm_packager/src/common/source_live.h>

#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/base/config.h>

#include <strm/media/transcoder/mp4muxer/boxes_container.h>
#include <strm/media/transcoder/mp4muxer/movie.h>

namespace NStrm::NPackager {
    // static
    TSourceFuture TSourceLive::Make(
        TRequestWorker& request,
        const TConfig& /*config*/,
        const TString& uri,
        const TString& tracks,
        const ui64 chunkIndex,
        const TMaybe<ui64> partIndex,
        const Ti64TimeP chunkDuration,
        const Ti64TimeP chunkFragmentDuration)
    {
        return request.GetPoolUtil<TSourceLive>().New(
                                                     request,
                                                     uri,
                                                     tracks,
                                                     chunkIndex,
                                                     partIndex,
                                                     chunkDuration,
                                                     chunkFragmentDuration)
            ->ReadyPromise.GetFuture();
    }

    TSourceLive::TSourceLive(
        TRequestWorker& request,
        const TString& uri,
        const TString& tracks,
        const ui64 chunkIndex,
        const TMaybe<ui64> partIndex,
        const Ti64TimeP chunkDuration,
        const Ti64TimeP chunkFragmentDuration)
        : ISource(request)
        , Interval(MakeInterval(chunkIndex, partIndex, chunkDuration, chunkFragmentDuration))
        , FragmentDuration(chunkFragmentDuration)
        , Ready(false)
        , ReadyPromise(NewSourcePromise())
        , FinishedWithException(false)
        , FragmentReserveBufferSize(TFragment::MinBoxSize)
        , SentEnd(Interval.Begin)
        , CreationTime(TInstant::Now())
        , FirstFragmentSent(false)
    {
        const TIntervalP chunkInterval = MakeInterval(chunkIndex, {}, chunkDuration, chunkFragmentDuration);

        TStringBuilder srUri;
        srUri << uri << "/chunk/init-" << chunkInterval.End.ConvertToTimescale(1000);
        if (partIndex.Defined()) {
            srUri << "-" << *partIndex;
        }
        srUri << "/" << tracks;

        request.CreateSubrequest(
            TSubrequestParams{
                .Uri = srUri,
                .Background = partIndex.Empty(),
            },
            this);

        request.GetMetrics().LiveCreateLiveSourceDelay = CreationTime.MilliSeconds() - request.CreationTime.MilliSeconds();
    };

    TIntervalP TSourceLive::FullInterval() const {
        Y_ENSURE(Ready);
        return Interval;
    }

    TVector<TTrackInfo const*> TSourceLive::GetTracksInfo() const {
        Y_ENSURE(Ready);
        return TracksInfo;
    }

    TRepFuture<TMediaData> TSourceLive::GetMedia() {
        Y_ENSURE(Ready);
        return MediaPromise.GetFuture();
    }

    void TSourceLive::AcceptHeaders(const THeadersOut& headers) try {
        Y_ENSURE_EX(NGX_HTTP_OK == headers.Status(), THttpError(ProxyBadUpstreamHttpCode(headers.Status()), TLOG_WARNING) << "TSourceLive subrequest http code: " << headers.Status());
    } catch (const TFatalExceptionContainer&) {
        FinishedWithException = true;
        throw;
    } catch (...) {
        FinishedWithException = true;
        ReadyPromise.SetException(std::current_exception());
    }

    void TSourceLive::AcceptData(char const* const begin, char const* const end) try {
        if (FinishedWithException) {
            return;
        }

        char const* pos = begin;

        while (pos != end) {
            if (CurrentFragment.Empty()) {
                CurrentFragment.ConstructInPlace(/*withMoov = */ !Ready, FragmentReserveBufferSize);
            }
            pos = CurrentFragment->Append(pos, end);

            if (!CurrentFragment->Ready()) {
                Y_ENSURE(pos == end);
                break;
            }

            FragmentReserveBufferSize = Max<ui64>(FragmentReserveBufferSize, CurrentFragment->Data.Buffer().Size() * 1.1);

            NMP4Muxer::TBufferReader reader(std::move(CurrentFragment->Data.Buffer()));
            CurrentFragment.Clear();

            Movie.Read(reader, /*allowEmptyTrackMoovData = */ false);

            if (!Ready) {
                TracksInfo.resize(Movie.Tracks.size());
                TracksDataParams.resize(Movie.Tracks.size());

                for (size_t ti = 0; ti < Movie.Tracks.size(); ++ti) {
                    TTrackInfo* info = Request.GetPoolUtil<TTrackInfo>().New();

                    info->Language = Movie.Tracks[ti]->MoovData->Language;
                    info->Name = Movie.Tracks[ti]->MoovData->Name;
                    info->Params = Movie.Tracks[ti]->MoovData->Params;

                    TracksInfo[ti] = info;

                    TracksDataParams[ti] = Mp4TrackParams2TrackDataParams(info->Params);
                }

                MediaPromise = TRepPromise<TMediaData>::Make();

                Ready = true;
                ReadyPromise.SetValue(this);
            }

            const TSimpleBlob dataBlob = Request.HangDataInPool(std::move(reader.Buffer()));

            TMediaData media;
            media.Interval.Begin = SentEnd;
            media.Interval.End = SentEnd;
            media.TracksInfo = TracksInfo;
            media.TracksSamples.resize(Movie.Tracks.size());

            for (size_t ti = 0; ti < Movie.Tracks.size(); ++ti) {
                const NMP4Muxer::TTrack& track = *Movie.Tracks[ti];
                const ui32 timescale = track.MoovData->Timescale;

                media.TracksSamples[ti].resize(track.Samples.size());

                for (size_t si = 0; si < track.Samples.size(); ++si) {
                    const auto& src = track.Samples[si];
                    TSampleData& dst = media.TracksSamples[ti][si];

                    const i64 dts = src.Dts;
                    const i64 pts = dts + src.CompositionTimeOffset;

                    const Ti64TimeP pDts = Ti64TimeP(dts, timescale);
                    const Ti64TimeP pPts = Ti64TimeP(pts, timescale);
                    const Ti32TimeP pDuration = Ti32TimeP(Ti64TimeP(dts + src.Duration, timescale) - pDts);

                    dst.Dts = pDts;
                    dst.Cto = Ti32TimeP(pPts - pDts);
                    dst.CoarseDuration = pDuration;
                    dst.Flags = src.Flags;
                    dst.DataSize = src.DataSize;
                    dst.Data = dataBlob.begin() + src.DataBegin;
                    dst.DataParams = TracksDataParams[ti];
                    dst.DataFuture = NThreading::MakeFuture();

                    Y_ENSURE(dst.Data + dst.DataSize <= dataBlob.end());

                    media.Interval.End = Max(media.Interval.End, pPts + Ti64TimeP(1));
                }
            }

            Movie.DropAllSamples();

            media.Interval.End = DivCeil(media.Interval.End.Value, FragmentDuration.Value) * FragmentDuration;

            Y_ENSURE(media.Interval.End > media.Interval.Begin);

            SentEnd = media.Interval.End;

            const i64 begin = media.Interval.Begin.MilliSeconds();
            const i64 end = media.Interval.End.MilliSeconds();
            const i64 currentTimestamp = TInstant::Now().MilliSeconds();
            if (!FirstFragmentSent) {
                FirstFragmentSent = true;

                Request.GetMetrics().LiveFirstFragmentDelay = currentTimestamp - CreationTime.MilliSeconds();
                Request.GetMetrics().LiveFirstFragmentDuration = end - begin;
                Request.GetMetrics().LiveFirstFragmentLatency = currentTimestamp - begin;
            }

            Request.GetMetrics().LiveLastFragmentDelay = currentTimestamp - CreationTime.MilliSeconds();
            Request.GetMetrics().LiveLastFragmentDuration = end - begin;
            Request.GetMetrics().LiveLastFragmentLatency = currentTimestamp - begin;

            MediaPromise.PutData(std::move(media));
        }
    } catch (const TFatalExceptionContainer&) {
        FinishedWithException = true;
        throw;
    } catch (...) {
        FinishedWithException = true;
        const std::exception_ptr ex = std::current_exception();

        if (Ready) {
            MediaPromise.FinishWithException(ex);
        } else {
            ReadyPromise.SetException(ex);
        }
    }

    void TSourceLive::SubrequestFinished(const TFinishStatus status) try {
        if (FinishedWithException) {
            return;
        }
        CheckFinished(status);

        MediaPromise.Finish();
    } catch (const TFatalExceptionContainer&) {
        FinishedWithException = true;
        throw;
    } catch (...) {
        FinishedWithException = true;
        const std::exception_ptr ex = std::current_exception();

        if (Ready) {
            MediaPromise.FinishWithException(ex);
        } else {
            ReadyPromise.SetException(ex);
        }
    }

    TSourceLive::TFragment::TFragment(const bool withMoov, const size_t reserveBufferSize)
        : WithMoov(withMoov)
        , GotFtyp(false)
        , GotMoov(false)
        , GotMoof(false)
        , GotMdat(false)
        , DataSizeControl(0)
        , CurrentBoxStoredSize(0)
        , CurrentBoxNeedSize(0)
    {
        Data.Buffer().Reserve(reserveBufferSize);
    }

    char const* TSourceLive::TFragment::Append(char const* const begin, char const* const end) {
        char const* pos = begin;

        // phases:
        //  0. on the edge between boxes
        //      CurrentBoxStoredSize == 0
        //      CurrentBoxSize empty
        //  1.found some box start but didnt understand its length yet:
        //      CurrentBoxStoredSize > 0
        //      CurrentBoxSize empty
        //  2 reading some box
        //      CurrentBoxStoredSize > 0
        //      CurrentBoxSize > 0
        while (pos != end) {
            if (Ready()) {
                return pos;
            }

            if (CurrentBoxSize.Defined()) {
                const size_t toStore = Min<size_t>(end - pos, *CurrentBoxSize - CurrentBoxStoredSize);
                Data.Buffer().Append(pos, pos + toStore);
                pos += toStore;
                CurrentBoxStoredSize += toStore;
                if (CurrentBoxStoredSize == *CurrentBoxSize) {
                    CurrentBoxSize.Clear();
                    CurrentBoxStoredSize = 0;
                    CurrentBoxNeedSize = 0;
                }
            } else {
                const size_t toStore = Min<size_t>(end - pos, Max(CurrentBoxNeedSize, 1UL));
                Data.Buffer().Append(pos, pos + toStore);
                pos += toStore;
                CurrentBoxStoredSize += toStore;

                if (CurrentBoxStoredSize >= CurrentBoxNeedSize) {
                    TMaybe<NMP4Muxer::TBox> box;
                    Data.SetPosition(Data.Buffer().Size() - CurrentBoxStoredSize);
                    NMP4Muxer::TBox::TryRead(
                        Data,                 // [in]  TReader& reader
                        CurrentBoxStoredSize, // [in]  const size_t size
                        box,                  // [out] TMaybe<TBox> &result
                        CurrentBoxNeedSize);  // [out] size_t &needSize

                    if (box.Defined()) {
                        CurrentBoxSize = box->Size;
                        if (box->Type == 'moof') {
                            Y_ENSURE(!GotMoof);
                            GotMoof = true;
                            DataSizeControl += box->Size;
                        } else if (box->Type == 'mdat') {
                            Y_ENSURE(!GotMdat);
                            GotMdat = true;
                            DataSizeControl += box->Size;
                        } else if (box->Type == 'ftyp') {
                            Y_ENSURE(WithMoov && !GotFtyp);
                            GotFtyp = true;
                            DataSizeControl += box->Size;
                        } else if (box->Type == 'moov') {
                            Y_ENSURE(WithMoov && !GotMoov);
                            GotMoov = true;
                            DataSizeControl += box->Size;
                        } else {
                            Y_ENSURE(false, "TSourceLive::TFragment::Append: unexpected box type " << box->Type);
                        }
                        Data.Buffer().Reserve(Data.Buffer().Size() + *CurrentBoxSize - CurrentBoxStoredSize + MinBoxSize);
                    }
                }
            }
        }

        return pos;
    }

    bool TSourceLive::TFragment::Ready() {
        const bool ready = GotMoof && GotMdat && (GotMoov == WithMoov) && CurrentBoxStoredSize == CurrentBoxSize.GetOrElse(0);
        Y_ENSURE(!ready || Data.Buffer().Size() == DataSizeControl);
        return ready;
    }

    TSourceLive::TConfig::TConfig() {
    }

    TSourceLive::TConfig::TConfig(const TLocationConfig& locationConfig)
    {
        (void)locationConfig;
    }

    void TSourceLive::TConfig::Check() const {
    }
}
