#pragma once

#include <nginx/modules/strm_packager/src/common/timestamps.h>
#include <nginx/modules/strm_packager/src/common/repeatable_future.h>

#include <strm/media/transcoder/mp4muxer/mux.h>

#include <library/cpp/threading/future/future.h>

namespace NStrm::NPackager {
    class TRequestWorker;

    // mp4 flags, TODO: make an explicit class?
    using TSampleFlags = ui32;

    struct TSampleData {
        static constexpr i64 Timescale = Ti64TimeP::Timescale;

        struct TDataParams {
            enum class EFormat: ui8 {
                AudioAccRDB,
                AudioAccADTS,
                AudioMp3,
                AudioOpus,
                AudioAc3,
                AudioEAc3,

                VideoAvcc,
                VideoHvcc,
                VideoVpcc,

                TimedMetaId3,

                SubtitleRawText,
                SubtitleTTML,
            };
            EFormat Format;
            TMaybe<ui8> NalUnitLengthSize;

            bool operator==(const TDataParams& p) const {
                return Format == p.Format && p.NalUnitLengthSize == NalUnitLengthSize;
            }
        };

        static constexpr TSampleFlags KeyframeFlags = (2 << 24);
        static constexpr TSampleFlags KeyframeMask = (3 << 24);

        Ti64TimeP Dts;
        Ti32TimeP Cto;
        Ti32TimeP CoarseDuration; // not precise due to convert to Ti32TimeP timescale
        TSampleFlags Flags;
        ui32 DataSize;
        TDataParams DataParams;
        ui8 const* Data;
        NThreading::TFuture<void> DataFuture;

        Ti64TimeP GetPts() const {
            return Dts + Cto;
        }

        bool IsKeyframe() const {
            return (Flags & KeyframeMask) == KeyframeFlags;
        }
    };

    struct TTrackInfo {
        using TAudioParams = NStrm::NMP4Muxer::TTrackMoovData::TAudioParams;
        using TVideoParams = NStrm::NMP4Muxer::TTrackMoovData::TVideoParams;
        using TTimedMetaId3Params = NStrm::NMP4Muxer::TTrackMoovData::TTimedMetaId3Params;
        using TSubtitleParams = NStrm::NMP4Muxer::TTrackMoovData::TSubtitleParams;
        using TParams = NStrm::NMP4Muxer::TTrackMoovData::TParams;

        ui16 Language; // 3-char code: "abc" = ((ui8('a') - x60) << 10) + ((ui8('b') - x60) << 5) + (ui8('c') - x60)
        TString Name;
        TParams Params;
    };

    // all samples in Interval
    struct TMediaData {
        TIntervalP Interval;
        TVector<TTrackInfo const*> TracksInfo;
        TVector<TVector<TSampleData>> TracksSamples;
    };

    // mark Interval as eiter original source, or ad block
    struct TMetaData {
        struct TSourceData {
            // no data
        };

        struct TAdData {
            enum class EType: ui32 {
                Inroll /* "inroll" */,
                Overlay /* "overlay" */,
                S2s /* "s2s" */,
            };

            // whole ad block interval, BlockEnd can be unknown yet
            Ti64TimeP BlockBegin;
            TMaybe<Ti64TimeP> BlockEnd;

            EType Type;
            TMaybe<TString> ImpId;
            TString Id;
        };

        using TData = std::variant<TSourceData, TAdData>;

        TIntervalP Interval;
        TData Data;
    };

    template <typename T>
    struct TPtrRange {
        T* Begin;
        T* End;

        T& Back() {
            return *(End - 1);
        }

        bool Empty() {
            return Begin >= End;
        }
    };

    template <typename TIterator>
    auto GetSamplesInterval(TIterator begin, TIterator end, const TIntervalP interval, const bool kalturaMode) {
        TPtrRange<std::remove_reference_t<decltype(*begin)>> r{nullptr, nullptr};

        for (TIterator s = begin; s != end; ++s) {
            const Ti64TimeP ts = kalturaMode
                                     ? s->Dts
                                     : (s->Dts + s->Cto);

            if (ts >= interval.End) {
                break;
            } else if (!r.Begin && ts >= interval.Begin) {
                r.Begin = &*s;
                r.End = r.Begin + 1;
            } else if (r.Begin) {
                r.End = &*s + 1;
            }
        }

        return r;
    }

    template <typename TContainer>
    auto GetSamplesInterval(TContainer& samples, const TIntervalP interval, const bool kalturaMode) {
        return GetSamplesInterval(samples.begin(), samples.end(), interval, kalturaMode);
    }

    // in the returned all samples wil have ready DataFuture
    TRepFuture<TMediaData> RequireData(const TRepFuture<TMediaData>& data, TRequestWorker& request);

    TRepFuture<TMetaData> MakeSimpleMeta(const TIntervalP interval, const TMaybe<TMetaData::TAdData> ad);

}
