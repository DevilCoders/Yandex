#pragma once

#include <nginx/modules/strm_packager/src/base/workers.h>
#include <nginx/modules/strm_packager/src/common/source.h>

namespace NStrm::NPackager {
    enum class EContainer {
        MP4,
        TS,
        WEBVTT,
    };

    class IMuxer {
    protected:
        IMuxer() = default;

        template <typename TResult = TSampleData>
        static TVector<TVector<TResult>> RemoveOverlappingDts(const TVector<TVector<TSampleData>>& tracksSamples);

    public:
        virtual ~IMuxer() = default;
        virtual TSimpleBlob MakeInitialSegment(const TVector<TTrackInfo const*>& tracksInfo) const = 0;
        virtual void SetMediaData(TRepFuture<TMediaData> data) = 0; // must be called no more than once
        virtual TRepFuture<TSendData> GetData() = 0;                // must be called after SetMediaData
        virtual TString GetContentType(const TVector<TTrackInfo const*>& tracksInfo) const = 0;
    };

    using TMuxerFuture = NThreading::TFuture<IMuxer*>;
    using TMuxerPromise = NThreading::TPromise<IMuxer*>;

    inline TMuxerPromise NewMuxerPromise() {
        return NThreading::NewPromise<IMuxer*>();
    }

    template <typename TResult>
    inline TVector<TVector<TResult>> IMuxer::RemoveOverlappingDts(const TVector<TVector<TSampleData>>& tracksSamples) {
        TVector<TVector<TResult>> result(tracksSamples.size());
        for (size_t trackIndex = 0; trackIndex < tracksSamples.size(); ++trackIndex) {
            const TVector<TSampleData>& src = tracksSamples[trackIndex];
            TVector<TResult>& dst = result[trackIndex];

            dst.reserve(src.size());

            for (const auto& s : src) {
                // there can be small DTS overlapping on the border between two different sources
                //  since all cutting is performed based on PTS
                //  so eliminate that overlapping:
                while (!dst.empty() && !(dst.back().Dts < s.Dts)) {
                    dst.pop_back();
                }
                dst.emplace_back(s);
            }
        }
        return result;
    }
}
