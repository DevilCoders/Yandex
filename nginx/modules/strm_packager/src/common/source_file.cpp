#include <nginx/modules/strm_packager/src/common/source_file.h>

#include <nginx/modules/strm_packager/src/base/config.h>

#include <util/generic/queue.h>

namespace NStrm::NPackager {
    // static
    TSourceFuture TSourceFile::Make(
        TRequestWorker& request,
        const TConfig& config,
        const TString& uri,
        const TString& args,
        const TMaybe<TIntervalP> cropInterval,
        const TMaybe<Ti64TimeP> offset,
        const std::function<NThreading::TFuture<TSimpleBlob>()> moovGetter,
        const NThreading::TFuture<TSimpleBlob> wholeFile) {
        config.Check();

        TStringBuilder cacheKey;
        cacheKey << "file_moov:uri[" << uri << "]args[" << args << "]";

        const NThreading::TFuture<TFileMediaData> moovFuture =
            GetShmCacheFileMediaData(
                wholeFile.Initialized() ? TMaybe<TShmZone<TShmCache>>() : config.MoovShmCacheZone,
                request,
                cacheKey,
                moovGetter,
                cropInterval);

        const NThreading::TFuture<void> toWait =
            wholeFile.Initialized()
                ? PackagerWaitExceptionOrAll<void>({wholeFile.IgnoreResult(), moovFuture.IgnoreResult()})
                : moovFuture.IgnoreResult();

        return toWait.Apply(
            [&request, config, uri, args, cropInterval, offset, moovFuture, wholeFile](const NThreading::TFuture<void>& future) -> ISource* {
                future.TryRethrow();
                return request.GetPoolUtil<TSourceFile>().New(request, config, moovFuture.GetValue(), uri, args, cropInterval, offset, wholeFile.Initialized() ? wholeFile.GetValue() : TSimpleBlob());
            });
    }

    TSourceFile::TSourceFile(
        TRequestWorker& request,
        const TConfig& config,
        const TFileMediaData& moov,
        const TString& uri,
        const TString& args,
        const TMaybe<TIntervalP> cropInterval,
        const TMaybe<Ti64TimeP> offset,
        const TSimpleBlob wholeFile)
        : ISource(request)
        , Config(config)
        , FileMedia(moov)
        , TracksInfo(AllocTracksInfo(moov.TracksInfo, Request))
        , Uri(uri)
        , Args(args)
        , Interval(cropInterval.GetOrElse(TIntervalP{Ti64TimeP(0), FileMedia.Duration}))
        , Offset(offset.GetOrElse(Ti64TimeP(0)))
        , WholeFile(wholeFile)
    {
        if (!WholeFile.empty()) {
            const NThreading::TFuture<void> readyFuture = NThreading::MakeFuture();

            for (auto& track : FileMedia.TracksSamples) {
                for (auto& sample : track) {
                    Y_ENSURE(sample.DataFileOffset + sample.DataSize <= WholeFile.size());

                    sample.DataFuture = readyFuture;
                    sample.DataBlob = &WholeFile;
                    sample.DataBlobOffset = sample.DataFileOffset;
                }
            }
        }
    }

    TIntervalP TSourceFile::FullInterval() const {
        return TIntervalP{Interval.Begin + Offset, Interval.End + Offset};
    }

    TVector<TTrackInfo const*> TSourceFile::GetTracksInfo() const {
        return TracksInfo;
    }

    namespace {
        struct TDataRequest {
            ui64 Begin;
            ui64 End;
            NThreading::TPromise<void> Promise;
            TSimpleBlob* Blob;
        };
    }

    TRepFuture<TMediaData> TSourceFile::GetMedia() {
        return GetMedia(FullInterval(), Ti64TimeP(0));
    }

    TRepFuture<TMediaData> TSourceFile::GetMedia(const TIntervalP interval, const Ti64TimeP addTsOffset) {
        return GetMediaImpl({interval.Begin - Offset, interval.End - Offset}, addTsOffset + Offset);
    }

    // this work as if TSourceFile::Offset is zero
    TRepFuture<TMediaData> TSourceFile::GetMediaImpl(const TIntervalP interval, const Ti64TimeP addTsOffset) {
        Y_ENSURE(interval.Begin < interval.End);

        TVector<NThreading::TFuture<void>> futuresToWait;

        using TFSRange = TPtrRange<TFileMediaData::TFileSample>;

        const auto comp = [](const TFSRange& a, const TFSRange& b) -> bool {
            return a.Begin->DataFileOffset > b.Begin->DataFileOffset;
        };

        const auto skipInit = [&futuresToWait](TFSRange& r) {
            for (; !r.Empty() && r.Begin->DataFuture.Initialized(); ++r.Begin) {
                if (futuresToWait.empty() || futuresToWait.back().StateId() != r.Begin->DataFuture.StateId()) {
                    futuresToWait.push_back(r.Begin->DataFuture);
                }
            }
        };

        TVector<TDataRequest> toRequest;

        TPriorityQueue<TFSRange, TVector<TFSRange>, decltype(comp)> rangeQueue(comp);

        for (auto& trackSamples : FileMedia.TracksSamples) {
            TFSRange range = GetSamplesInterval(trackSamples, interval);
            skipInit(range);
            if (!range.Empty()) {
                rangeQueue.push(range);
            }
        }

        const ui64 MaxGap = 0; // TODO: get from config

        while (!rangeQueue.empty()) {
            TFileMediaData::TFileSample* sample;
            {
                TFSRange range = rangeQueue.top();
                rangeQueue.pop();
                sample = range.Begin;
                ++range.Begin;
                skipInit(range);
                if (!range.Empty()) {
                    rangeQueue.push(range);
                }
            }

            Y_ENSURE(toRequest.empty() || sample->DataFileOffset >= toRequest.back().End);

            if (toRequest.empty() || sample->DataFileOffset > toRequest.back().End + MaxGap) {
                toRequest.emplace_back();
                toRequest.back().Begin = sample->DataFileOffset;
                toRequest.back().End = sample->DataFileOffset + sample->DataSize;
                toRequest.back().Blob = Request.GetPoolUtil<TSimpleBlob>().New();
                toRequest.back().Promise = NThreading::NewPromise<void>();
                futuresToWait.push_back(toRequest.back().Promise.GetFuture());
            } else {
                toRequest.back().End = sample->DataFileOffset + sample->DataSize;
            }

            sample->DataFuture = toRequest.back().Promise.GetFuture();
            sample->DataBlob = toRequest.back().Blob;
            sample->DataBlobOffset = sample->DataFileOffset - toRequest.back().Begin;
        }

        for (TDataRequest& dr : toRequest) {
            ui8* blobPtr = Request.GetPoolUtil<ui8>().Alloc(dr.End - dr.Begin);
            ui64 rangeBegin = dr.Begin;

            *dr.Blob = TSimpleBlob(blobPtr, blobPtr + (dr.End - dr.Begin));

            TVector<NThreading::TFuture<TSimpleBlob>> futures;

            while (rangeBegin < dr.End) {
                ui64 size = dr.End - rangeBegin;
                if (Config.MaxMediaDataSubrequestSize.Defined()) {
                    size = Min(size, *Config.MaxMediaDataSubrequestSize);
                }

                futures.push_back(Request.CreateSubrequest(
                    TSubrequestParams{
                        .Uri = Uri,
                        .Args = Args,
                        .RangeBegin = rangeBegin,
                        .RangeEnd = rangeBegin + size,
                    },
                    NGX_HTTP_PARTIAL_CONTENT,
                    blobPtr,
                    blobPtr + size,
                    true /* = requireEqualSize */));

                blobPtr += size;
                rangeBegin += size;
            }

            PackagerWaitExceptionOrAll(futures).Subscribe(Request.MakeFatalOnException([dr](const NThreading::TFuture<void>& future) mutable {
                try {
                    future.TryRethrow();
                } catch (const TFatalExceptionContainer&) {
                    throw;
                } catch (...) {
                    dr.Promise.SetException(std::current_exception());
                    return;
                }

                dr.Promise.SetValue();
            }));
        }

        TRepPromise<TMediaData> promise = TRepPromise<TMediaData>::Make();

        TMediaData result;
        result.Interval = interval;
        result.Interval.Begin += addTsOffset;
        result.Interval.End += addTsOffset;
        result.TracksInfo.resize(TracksInfo.size());
        result.TracksSamples.resize(TracksInfo.size());
        for (size_t ti = 0; ti < TracksInfo.size(); ++ti) {
            result.TracksInfo[ti] = TracksInfo[ti];

            const auto srcRange = GetSamplesInterval(FileMedia.TracksSamples[ti], interval);

            TVector<TSampleData>& dstSamples = result.TracksSamples[ti];
            dstSamples.reserve(srcRange.End - srcRange.Begin);

            for (auto srcSample = srcRange.Begin; srcSample != srcRange.End; ++srcSample) {
                Y_ENSURE(srcSample->DataBlob);
                Y_ENSURE(!srcSample->DataBlob->empty());
                Y_ENSURE(srcSample->DataBlob->begin() + srcSample->DataBlobOffset + srcSample->DataSize <= srcSample->DataBlob->end());
                dstSamples.push_back(TSampleData{
                    .Dts = srcSample->Dts + addTsOffset,
                    .Cto = srcSample->Cto,
                    .CoarseDuration = srcSample->CoarseDuration,
                    .Flags = srcSample->Flags,
                    .DataSize = srcSample->DataSize,
                    .DataParams = FileMedia.TracksDataParams[ti],
                    .Data = (ui8 const*)srcSample->DataBlob->begin() + srcSample->DataBlobOffset,
                    .DataFuture = srcSample->DataFuture,
                });
            }
        }

        promise.PutData(result);
        promise.Finish();

        return promise.GetFuture();
    }

    TSourceFile::TConfig::TConfig() {
    }

    TSourceFile::TConfig::TConfig(const TLocationConfig& locationConfig)
        : MoovShmCacheZone(locationConfig.MoovShmCacheZone)
        , MaxMediaDataSubrequestSize(locationConfig.MaxMediaDataSubrequestSize)
    {
    }

    void TSourceFile::TConfig::Check() const {
        // MoovShmCacheZone now allowed undefined
        if (MaxMediaDataSubrequestSize.Defined()) {
            Y_ENSURE(*MaxMediaDataSubrequestSize > 0);
        }
    }
}
