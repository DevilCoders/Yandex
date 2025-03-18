#include <nginx/modules/strm_packager/src/common/mp4_common.h>
#include <nginx/modules/strm_packager/src/common/source_mp4_file.h>

#include <nginx/modules/strm_packager/src/base/config.h>

#include <strm/media/transcoder/mp4muxer/boxes.h>

namespace NStrm::NPackager {
    class TMoovSpotter: public ISubrequestWorker {
    public:
        using TBufferReader = NMP4Muxer::TBufferReader;

        TMoovSpotter(
            TRequestWorker& mainWorker,
            const TString& uri,
            const TString& args,
            const i64 scanBlockSize,
            const bool getWholeFile);

        NThreading::TFuture<TSimpleBlob> GetMoovFuture() {
            return MoovPromise;
        }

        NThreading::TFuture<TSimpleBlob> GetWholeFileFuture() {
            Y_ENSURE(GetWholeFile);
            return WholeFilePromise;
        }

    private:
        void AcceptHeaders(const THeadersOut& headers) override;
        void AcceptData(char const* const begin, char const* const end) override;
        void SubrequestFinished(const TFinishStatus status) override;

        TRequestWorker& MainWorker;
        const TString Uri;
        const TString Args;
        const ui64 ScanBlockSize;
        const bool GetWholeFile;
        ui64 CurrentRangeBegin;
        ui64 CurrentRangeEnd;
        ui64 AcceptedSize;
        TMaybe<ui64> FileSize;

        TBufferReader Data;

        int SubrequestsCount;

        NThreading::TPromise<TSimpleBlob> MoovPromise;
        NThreading::TPromise<TSimpleBlob> WholeFilePromise;
        bool Finished;
    };

    TMoovSpotter::TMoovSpotter(
        TRequestWorker& mainWorker,
        const TString& uri,
        const TString& args,
        const i64 scanBlockSize,
        const bool getWholeFile)
        : MainWorker(mainWorker)
        , Uri(uri)
        , Args(args)
        , ScanBlockSize(scanBlockSize)
        , GetWholeFile(getWholeFile)
        , AcceptedSize(0)
        , SubrequestsCount(0)
        , MoovPromise(NThreading::NewPromise<TSimpleBlob>())
        , Finished(false)
    {
        if (GetWholeFile) {
            CurrentRangeBegin = 0;
            CurrentRangeEnd = 0;
            WholeFilePromise = NThreading::NewPromise<TSimpleBlob>();

            MainWorker.CreateSubrequest(
                TSubrequestParams{
                    .Uri = Uri,
                    .Args = Args,
                },
                this);
        } else {
            CurrentRangeBegin = 0;
            CurrentRangeEnd = scanBlockSize;
            MainWorker.CreateSubrequest(
                TSubrequestParams{
                    .Uri = Uri,
                    .Args = Args,
                    .RangeBegin = CurrentRangeBegin,
                    .RangeEnd = CurrentRangeEnd,
                },
                this);
        }
    }

    void TMoovSpotter::AcceptHeaders(const THeadersOut& headers) try {
        if (Finished) {
            return;
        }

        if (GetWholeFile) {
            Y_ENSURE(SubrequestsCount == 0);
            Y_ENSURE_EX(NGX_HTTP_OK == headers.Status(), THttpError(ProxyBadUpstreamHttpCode(headers.Status()), TLOG_WARNING) << "moov sr got " << headers.Status());

            Y_ENSURE(headers.ContentLength().Defined());

            FileSize = *headers.ContentLength();
            CurrentRangeBegin = 0;
            CurrentRangeEnd = *FileSize;

            Data.Buffer().Reserve(*FileSize);
        } else {
            Y_ENSURE_EX(NGX_HTTP_PARTIAL_CONTENT == headers.Status(), THttpError(ProxyBadUpstreamHttpCode(headers.Status()), TLOG_WARNING) << "moov sr got " << headers.Status());

            const TMaybe<TContentRange> range = headers.ContentRange();
            Y_ENSURE(range);
            FileSize = FileSize.GetOrElse(range->Size);
            Y_ENSURE(range->Size == *FileSize);

            Y_ENSURE(range->Begin == CurrentRangeBegin);
            Y_ENSURE(range->End == Min(CurrentRangeEnd, *FileSize));

            if (headers.ContentLength().Defined()) {
                Data.Buffer().Reserve(Data.Buffer().Size() + *headers.ContentLength());
            }
        }
    } catch (const TFatalExceptionContainer&) {
        throw;
    } catch (...) {
        Finished = true;
        MoovPromise.SetException(std::current_exception());
        if (GetWholeFile) {
            WholeFilePromise.SetException(std::current_exception());
        }
    }

    void TMoovSpotter::AcceptData(char const* const begin, char const* const end) try {
        if (Finished) {
            return;
        }
        Data.Buffer().Append(begin, end);
        AcceptedSize += end - begin;
    } catch (const TFatalExceptionContainer&) {
        throw;
    } catch (...) {
        Finished = true;
        MoovPromise.SetException(std::current_exception());
        if (GetWholeFile) {
            WholeFilePromise.SetException(std::current_exception());
        }
    }

    void TMoovSpotter::SubrequestFinished(const TFinishStatus status) try {
        if (Finished) {
            return;
        }
        CheckFinished(status);

        Y_ENSURE(FileSize.Defined());
        const ui64 expectedSize = Min(CurrentRangeEnd, *FileSize) - CurrentRangeBegin;
        Y_ENSURE_EX(AcceptedSize == expectedSize, THttpError(NGX_HTTP_BAD_GATEWAY, TLOG_ERR) << "subrequest got " << AcceptedSize << " but expected " << expectedSize);
        AcceptedSize = 0;
        ++SubrequestsCount;

        TMaybe<ui64> nextRangeBegin;

        size_t needSize = 0;
        while (true) {
            needSize = 0;
            const ui64 pos = Data.GetPosition();
            TMaybe<NMP4Muxer::TBox> box;
            NMP4Muxer::TBox::TryRead(Data, Data.Buffer().Size() - pos, box, needSize);
            Data.SetPosition(pos);

            if (box.Defined() && box->Type == 'moov') {
                if (pos + box->Size <= Data.Buffer().Size()) {
                    if (SubrequestsCount > 3) {
                        MainWorker.LogWarn()
                            << "found moov with " << SubrequestsCount << " subrequests at "
                            << (CurrentRangeBegin + pos) << " bytes from file start, moov box size: " << box->Size << " bytes";
                    }

                    const ui64 moovBegin = Data.GetPosition();
                    const ui64 moovEnd = moovBegin + box->Size;

                    const TSimpleBlob dataBlob = MainWorker.HangDataInPool(std::move(Data.Buffer()));
                    const TSimpleBlob moovDataBlob(dataBlob.begin() + moovBegin, dataBlob.begin() + moovEnd);

                    MoovPromise.SetValue(moovDataBlob);
                    if (GetWholeFile) {
                        Y_ENSURE(SubrequestsCount == 1);
                        Y_ENSURE(dataBlob.size() == FileSize);
                        WholeFilePromise.SetValue(dataBlob);
                    }
                } else {
                    CurrentRangeBegin = CurrentRangeEnd;
                    CurrentRangeEnd = CurrentRangeBegin + (pos + box->Size - Data.Buffer().Size());
                    Y_ENSURE(CurrentRangeEnd <= *FileSize);
                    MainWorker.CreateSubrequest(
                        TSubrequestParams{
                            .Uri = Uri,
                            .Args = Args,
                            .RangeBegin = CurrentRangeBegin,
                            .RangeEnd = CurrentRangeEnd,
                        },
                        this);
                }
                return;
            } else if (box.Defined() && pos + box->Size <= Data.Buffer().Size()) {
                Data.SetPosition(pos + box->Size);
            } else if (box.Defined()) {
                nextRangeBegin = CurrentRangeEnd + pos + box->Size - Data.Buffer().Size();
                break;
            } else {
                nextRangeBegin = CurrentRangeEnd - Data.Buffer().Size() + pos;
                break;
            }
        }
        Y_ENSURE(nextRangeBegin.Defined());
        Data.SetPosition(0);
        Data.Buffer().Clear();

        CurrentRangeBegin = *nextRangeBegin;
        CurrentRangeEnd = CurrentRangeBegin + Max(needSize, ScanBlockSize);
        Y_ENSURE(*FileSize > CurrentRangeBegin);
        Y_ENSURE(CurrentRangeEnd - CurrentRangeBegin >= needSize);

        MainWorker.CreateSubrequest(
            TSubrequestParams{
                .Uri = Uri,
                .RangeBegin = CurrentRangeBegin,
                .RangeEnd = CurrentRangeEnd,
            },
            this);
    } catch (const TFatalExceptionContainer&) {
        throw;
    } catch (...) {
        Finished = true;
        MoovPromise.SetException(std::current_exception());
        if (GetWholeFile) {
            WholeFilePromise.SetException(std::current_exception());
        }
    }

    // static
    TSourceFuture TSourceMp4File::Make(
        TRequestWorker& request,
        const TConfig& config,
        const TString& uri,
        const TString& args,
        const TMaybe<TIntervalP> cropInterval,
        const TMaybe<Ti64TimeP> offset,
        const bool getWholeFile) {
        config.Check();

        std::function<NThreading::TFuture<TSimpleBlob>()> moovGetter;
        NThreading::TFuture<TSimpleBlob> wholeFile;

        if (getWholeFile) {
            TMoovSpotter* spotter = request.GetPoolUtil<TMoovSpotter>().New(request, uri, args, *config.MoovScanBlockSize, /*getWholeFile = */ true);
            wholeFile = spotter->GetWholeFileFuture();
            moovGetter = [moov = spotter->GetMoovFuture()]() {
                return moov;
            };
        } else {
            moovGetter = [&request, config, uri, args]() {
                TMoovSpotter* spotter = request.GetPoolUtil<TMoovSpotter>().New(request, uri, args, *config.MoovScanBlockSize, /*getWholeFile = */ false);
                return spotter->GetMoovFuture();
            };
        }

        return TSourceFile::Make(
            request,
            config,
            uri,
            args,
            cropInterval,
            offset,
            moovGetter,
            wholeFile);
    }

    TSourceMp4File::TConfig::TConfig() {
    }

    TSourceMp4File::TConfig::TConfig(const TLocationConfig& locationConfig)
        : TSourceFile::TConfig(locationConfig)
        , MoovScanBlockSize(locationConfig.MoovScanBlockSize)
    {
    }

    void TSourceMp4File::TConfig::Check() const {
        TSourceFile::TConfig::Check();
        Y_ENSURE(MoovScanBlockSize.Defined());
    }

}
