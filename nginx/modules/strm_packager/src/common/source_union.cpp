#include <nginx/modules/strm_packager/src/common/source_union.h>
#include <nginx/modules/strm_packager/src/base/config.h>

namespace NStrm::NPackager {
    // static
    TSourceFuture TSourceUnion::Make(
        TRequestWorker& request,
        const TConfig& config,
        TVector<TSourceMaker> SourceMakers) {
        config.Check();
        Y_ENSURE(!SourceMakers.empty());

        for (const auto& sm : SourceMakers) {
            Y_ENSURE(sm);
        }

        while (SourceMakers.size() > 1) {
            const auto sourceMaker = SourceMakers[SourceMakers.size() - 2];
            const auto backupMaker = SourceMakers[SourceMakers.size() - 1];

            const TSourceMaker unionMaker = [&request, config, sourceMaker, backupMaker]() {
                return Make(request, config, sourceMaker(), backupMaker);
            };

            SourceMakers.pop_back();
            SourceMakers.pop_back();
            SourceMakers.push_back(unionMaker);
        }

        return SourceMakers[0]();
    }

    // static
    TSourceFuture TSourceUnion::Make(
        TRequestWorker& request,
        const TConfig& config,
        TSourceFuture source,
        TSourceMaker backupMaker) {
        Y_ENSURE(backupMaker);
        config.Check();

        TSourcePromise promise = NewSourcePromise();

        source.Subscribe(request.MakeFatalOnException(
            [promise, &request, config, backupMaker](const TSourceFuture& sourceFuture) mutable {
                ISource* source = nullptr;
                try {
                    source = sourceFuture.GetValue();
                } catch (const TFatalExceptionContainer&) {
                    throw;
                } catch (...) {
                    request.LogException("TSourceUnion::Make source creation failed:", std::current_exception());
                    backupMaker().Subscribe(request.MakeFatalOnException(
                        [promise](const TSourceFuture& backupSourceFuture) mutable {
                            ISource* backup = nullptr;
                            try {
                                backup = backupSourceFuture.GetValue();
                            } catch (const TFatalExceptionContainer&) {
                                throw;
                            } catch (...) {
                                promise.SetException(std::current_exception());
                                return;
                            }
                            promise.SetValue(backup);
                        }));
                    return;
                }

                Y_ENSURE(source);

                promise.SetValue(request.GetPoolUtil<TSourceUnion>().New(
                    request,
                    config,
                    *source,
                    backupMaker));
            }));

        return promise;
    }

    TSourceUnion::TSourceUnion(
        TRequestWorker& request,
        const TConfig& config,
        ISource& source,
        TSourceMaker backupMaker)
        : ISource(request)
        , Source(source)
        , BackupMaker(backupMaker)
        , Backup(nullptr)
        , Interval(Source.FullInterval())    // no exceptions here
        , TracksInfo(Source.GetTracksInfo()) // no exceptions here
        , MaxMediaTsGap(*config.MaxMediaTsGap)
        , MediaPromiseReady(false)
        , BackupMedia{.Interval = {Interval.Begin, Interval.Begin}}
    {
    }

    TIntervalP TSourceUnion::FullInterval() const {
        return Interval;
    }

    TVector<TTrackInfo const*> TSourceUnion::GetTracksInfo() const {
        return TracksInfo;
    }

    void TSourceUnion::RequireBackup() {
        if (BackupEnabled()) {
            return;
        }

        // ensure MediaPromise initialized
        GetMedia();

        // get backup
        Backup = BackupMaker();
        Backup.Subscribe(Request.MakeFatalOnException([this](const TSourceFuture&) {
            try {
                Backup.TryRethrow();
            } catch (const TFatalExceptionContainer&) {
                throw;
            } catch (...) {
                BackupException = std::current_exception();
                UpdateMedia();
                return;
            }
            Backup.GetValue()->GetMedia().AddCallback(Request.MakeFatalOnException(std::bind(&TSourceUnion::AcceptBackupMedia, this, std::placeholders::_1)));
        }));
    }

    TRepFuture<TMediaData> TSourceUnion::GetMedia() {
        if (MediaPromise.Initialized()) {
            return MediaPromise.GetFuture();
        }

        MediaPromise = TRepPromise<TMediaData>::Make();
        MediaPromiseEnd = Interval.Begin;

        Source.GetMedia().AddCallback(Request.MakeFatalOnException(std::bind(&TSourceUnion::AcceptSourceMedia, this, std::placeholders::_1)));

        return MediaPromise.GetFuture();
    }

    void TSourceUnion::FinishSourceMedia() {
        // add Media with empty interval
        SourceMedia.push_back(TMediaData{
            .Interval = {Interval.End, Interval.End},
            .TracksInfo = TracksInfo,
            .TracksSamples = TVector<TVector<TSampleData>>(TracksInfo.size()),
        });
    }

    void TSourceUnion::AcceptSourceMedia(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Exception()) {
            Request.LogException("TSourceUnion source Media failed:", dataRef.Exception());
            FinishSourceMedia();
            RequireBackup();
        } else if (dataRef.Empty()) {
            FinishSourceMedia();
        } else {
            SourceMedia.push_back(dataRef.Data());
        }
        UpdateMedia();
    }

    void TSourceUnion::AcceptBackupMedia(const TRepFuture<TMediaData>::TDataRef& dataRef) {
        if (dataRef.Exception()) {
            BackupException = dataRef.Exception();
        } else if (dataRef.Empty()) {
            if (BackupMedia.Interval.End != Interval.End) {
                BackupException = std::make_exception_ptr(yexception() << "TSourceUnion::AcceptBackupMedia: gap in backup media");
            }
        } else {
            const TMediaData& data = dataRef.Data();
            if (data.Interval.Begin != BackupMedia.Interval.End) {
                BackupException = std::make_exception_ptr(yexception() << "TSourceUnion::AcceptBackupMedia: gap in backup media");
            } else {
                Y_ENSURE(data.TracksSamples.size() == TracksInfo.size());
                BackupMedia.TracksSamples.resize(data.TracksSamples.size());

                for (size_t trackIndex = 0; trackIndex < data.TracksSamples.size(); ++trackIndex) {
                    const TVector<TSampleData>& src = data.TracksSamples[trackIndex];
                    TVector<TSampleData>& dst = BackupMedia.TracksSamples[trackIndex];

                    dst.reserve(dst.size() + src.size());
                    for (const auto& s : src) {
                        dst.push_back(s);
                    }
                }
                BackupMedia.Interval.End = data.Interval.End;
            }
        }
        UpdateMedia();
    }

    void TSourceUnion::UpdateMedia() {
        if (MediaPromiseReady) {
            return;
        }

        if (BackupException) {
            if (!MediaPromise.Initialized()) {
                MediaPromise = TRepPromise<TMediaData>::Make();
            }
            MediaPromiseReady = true;
            MediaPromise.FinishWithException(BackupException);
            return;
        }

        Y_ENSURE(MediaPromise.Initialized());

        if (BackupEnabled()) {
            // get every interval and every gap interval from source
            //   and fix it using backup data
            // and then put it to MediaPromise
            while (!SourceMedia.empty()) {
                const TMediaData& sdata = SourceMedia.front();
                if (sdata.Interval.End > BackupMedia.Interval.End) {
                    const Ti64TimeP canSendEnd = Min(sdata.Interval.Begin, BackupMedia.Interval.End);
                    if (canSendEnd > MediaPromiseEnd) {
                        // put media with only backup data
                        TMediaData rdata{
                            .Interval = {MediaPromiseEnd, canSendEnd},
                            .TracksInfo = TracksInfo,
                            .TracksSamples = TVector<TVector<TSampleData>>(TracksInfo.size()),
                        };
                        for (size_t trackIndex = 0; trackIndex < TracksInfo.size(); ++trackIndex) {
                            const auto bsr = GetSamplesInterval(BackupMedia.TracksSamples[trackIndex], rdata.Interval);
                            rdata.TracksSamples[trackIndex].assign(bsr.Begin, bsr.End);
                        }

                        MediaPromiseEnd = rdata.Interval.End;
                        MediaPromise.PutData(std::move(rdata));
                    }

                    // wait for more backup data
                    return;
                }

                TMediaData rdata{
                    .Interval = {MediaPromiseEnd, sdata.Interval.End},
                    .TracksInfo = TracksInfo,
                    .TracksSamples = TVector<TVector<TSampleData>>(TracksInfo.size()),
                };

                for (size_t trackIndex = 0; trackIndex < TracksInfo.size(); ++trackIndex) {
                    const TVector<TSampleData>& sourceSamples = sdata.TracksSamples[trackIndex];
                    const TVector<TSampleData>& backupSamples = BackupMedia.TracksSamples[trackIndex];
                    TVector<TSampleData>& resultSamples = rdata.TracksSamples[trackIndex];

                    // backup samples range
                    auto bsr = GetSamplesInterval(backupSamples, rdata.Interval);

                    resultSamples.reserve(sourceSamples.size() + (bsr.End - bsr.Begin));

                    TMaybe<Ti64TimeP> dtsEnd;
                    for (const TSampleData& ss : sourceSamples) {
                        for (; bsr.Begin < bsr.End; ++bsr.Begin) {
                            const TSampleData& bs = *bsr.Begin;
                            if (bs.Dts + bs.CoarseDuration > ss.Dts) {
                                break;
                            }
                            if (dtsEnd.Empty() || bs.Dts >= dtsEnd) {
                                resultSamples.push_back(bs);
                            }
                        }

                        resultSamples.push_back(ss);
                        dtsEnd = ss.Dts + ss.CoarseDuration;
                    }

                    for (; bsr.Begin < bsr.End; ++bsr.Begin) {
                        const TSampleData& bs = *bsr.Begin;
                        if (dtsEnd.Empty() || bs.Dts >= dtsEnd) {
                            resultSamples.push_back(bs);
                        }
                    }
                }

                MediaPromiseEnd = rdata.Interval.End;
                MediaPromise.PutData(std::move(rdata));
                SourceMedia.pop_front();

                if (MediaPromiseEnd == Interval.End) {
                    MediaPromiseReady = true;
                    MediaPromise.Finish();
                    return;
                }
            }
        } else {
            // just put source media data as long as they have no gap larger than MaxMediaTsGap
            //   but if gap found, backup is required
            while (!SourceMedia.empty()) {
                const TMediaData& data = SourceMedia.front();
                if (data.Interval.Begin != MediaPromiseEnd || WithLargeGap(data, MaxMediaTsGap)) {
                    RequireBackup();
                    return;
                }
                if (data.Interval.Begin == Interval.End) {
                    MediaPromiseReady = true;
                    MediaPromise.Finish();
                    return;
                }

                MediaPromiseEnd = data.Interval.End;
                MediaPromise.PutData(std::move(SourceMedia.front()));
                SourceMedia.pop_front();
            }
        }
    }

    // static
    bool TSourceUnion::WithLargeGap(const TMediaData& media, const Ti64TimeP maxGap) {
        if (media.Interval.Begin == media.Interval.End) {
            return false;
        }

        for (const auto& track : media.TracksSamples) {
            if (track.empty()) {
                return true;
            }

            Ti64TimeP minPts = track.front().GetPts();
            Ti64TimeP maxPts = track.front().GetPts() + track.front().CoarseDuration;

            for (size_t i = 1; i < track.size(); ++i) {
                const TSampleData& sample = track[i];
                const TSampleData& prev = track[i - 1];

                if (sample.Dts > prev.Dts + prev.CoarseDuration + maxGap) {
                    return true;
                }

                minPts = Min(minPts, sample.GetPts());
                maxPts = Max(maxPts, sample.GetPts() + sample.CoarseDuration);
            }

            if (minPts > media.Interval.Begin + maxGap || maxPts + maxGap < media.Interval.End) {
                return true;
            }
        }

        return false;
    }

    TSourceUnion::TConfig::TConfig() {
    }

    TSourceUnion::TConfig::TConfig(const TLocationConfig& locationConfig)
        : MaxMediaTsGap(locationConfig.MaxMediaTsGap)
    {
    }

    void TSourceUnion::TConfig::Check() const {
        Y_ENSURE(MaxMediaTsGap.Defined());
    }
}
