#pragma once

#include "urlseq_reader.h"

#include <kernel/stringmatch_tracker/lcs_wrapper.h>
#include <kernel/stringmatch_tracker/trigram_wrappers.h>

namespace NSequences {

struct TParams {
    const TArray* Array = nullptr;
    const TString Request;

    TParams(const TArray* array, const TString& request = "")
        : Array(array)
        , Request(request)
    {
    }
};

namespace NDetails {
    struct TUrlFractionFeatures {
        float DomainFraction = 0;
        float PathAndParamsFraction = 0;
        float FullUrlFraction = 0;
    };

    struct TUrlSimilarityFeatures {
        float DomainSimilarityFixed = 0;
        float QueryUrlLCSNormalizedByUrl = 0;
        float QueryUrlLCSNormalizedByQuery = 0;
        float EditDistNormalizedByMaxLen = 0;
    };

    struct TUrlTextFeatures {
        float IsHttps = 0;
        float IsYandexMusicAlbum = 0;
        float IsYandexMusicTrack = 0;
    };

    struct TUrlVideoFeatures {
        float DomainSimilarityFixed = 0;
        float PathAndParamsFraction = 0;
        float QueryDomainLCSQueryFraction = 0;
    };

    struct TTitleFeatures {
        float TitleTrigramsQ = 0;
        float TitleTrigramsT = 0;
    };

    class IUrlCalcer {
    public:
        virtual ~IUrlCalcer() {
        }
        virtual bool CalcFractionFeatures(ui32 docId, TUrlFractionFeatures& features) const = 0;
        virtual bool CalcSimilarityFeatures(ui32 docId, TUrlSimilarityFeatures& features) const = 0;
        virtual bool CalcUrlTextFeatures(ui32 docId, TUrlTextFeatures& features) const = 0;
        virtual bool CalcVideoFeatures(ui32 docId, TUrlVideoFeatures& features) const = 0;
    };

    class ITitleCalcer {
    public:
        virtual ~ITitleCalcer() {
        }
        virtual bool CalcFeatures(ui32 docId, TTitleFeatures& features) const = 0;
    };

    template <class RI>
    class TUrlCalcerImpl : public IUrlCalcer {
    public:
        typedef RI TReaderImpl;

    private:
        const TString& Request;

        THolder<TReaderImpl> Reader;
        THolder<::NStringMatchTracker::TLCSCalcer> LcsCalcer;
        THolder<::NStringMatchTracker::TQueryTrigramOverDocCalcer> TrigramCalcer;
        bool IsEditDistEnabled = false;

    public:
        TUrlCalcerImpl(TReaderImpl* reader, const TString& request, bool newQuorumQueryEditDistIsEnabled = false);
        bool CalcFractionFeatures(ui32 docId, TUrlFractionFeatures& features) const override;
        bool CalcSimilarityFeatures(ui32 docId, TUrlSimilarityFeatures& features) const override;
        bool CalcUrlTextFeatures(ui32 docId, TUrlTextFeatures& features) const override;
        bool CalcVideoFeatures(ui32 docId, TUrlVideoFeatures& features) const override;
    };

    template <class RI>
    class TTitleCalcerImpl : public ITitleCalcer {
    public:
        typedef RI TReaderImpl;

    private:
        const TString& Request;

        THolder<TReaderImpl> Reader;
        THolder<::NStringMatchTracker::TQueryTrigramOverDocCalcer> TrigramCalcer;

    public:
        TTitleCalcerImpl(TReaderImpl* reader, const TString& requeset);
        bool CalcFeatures(ui32 docId, TTitleFeatures& features) const override;
    };

};

class TUrlFeatureCalcer {
private:
    THolder<NDetails::IUrlCalcer> Impl;
    bool IsEditDistEnabled;

public:
    TUrlFeatureCalcer(
        const TParams& params,
        TOmniUrlAccessor* accessor,
        TBasesearchErfAccessor* erfAccessor,
        const IOnlineUrlStorage* urlStorage = nullptr,
        bool newQuorumQueryEditDistIsEnabled = false)
        : IsEditDistEnabled(newQuorumQueryEditDistIsEnabled)
    {
        if (urlStorage) {
            Impl.Reset(new NDetails::TUrlCalcerImpl<TRealTimeReader<IOnlineUrlStorage>>(
                           new TRealTimeReader<IOnlineUrlStorage>(*urlStorage, accessor, erfAccessor), params.Request, IsEditDistEnabled));
        }
        else if (params.Array) {
            Impl.Reset(new NDetails::TUrlCalcerImpl<TReader>(
                           new TReader(*params.Array), params.Request, IsEditDistEnabled));
        }
    }

public:
    bool CalcFractionFeatures(ui32 docId, NDetails::TUrlFractionFeatures& features) const {
        return Impl.Get() ? Impl->CalcFractionFeatures(docId, features) : false;
    }

    bool CalcSimilarityFeatures(ui32 docId, NDetails::TUrlSimilarityFeatures& features) const {
        return Impl.Get() ? Impl->CalcSimilarityFeatures(docId, features) : false;
    }

    bool CalcUrlTextFeatures(ui32 docId, NDetails::TUrlTextFeatures& features) const {
        return Impl.Get() ? Impl->CalcUrlTextFeatures(docId, features) : false;
    }

    bool CalcVideoFeatures(ui32 docId, NDetails::TUrlVideoFeatures& features) const {
        return Impl.Get() ? Impl->CalcVideoFeatures(docId, features) : false;
    }
};

class TTitleFeatureCalcer {
private:
    THolder<NDetails::ITitleCalcer> Impl;

public:
    TTitleFeatureCalcer(const TParams& params, TOmniTitleAccessor* accessor, TBasesearchErfAccessor* erfAccessor, const IOnlineTitleStorage* titleStorage = nullptr) {
        if (titleStorage) {
            Impl.Reset(new NDetails::TTitleCalcerImpl<TRealTimeReader<IOnlineTitleStorage>>(
                           new TRealTimeReader<IOnlineTitleStorage>(*titleStorage, accessor, erfAccessor), params.Request));
        }
        else if (params.Array) {
            Impl.Reset(new NDetails::TTitleCalcerImpl<TReader>(
                           new TReader(*params.Array), params.Request));
        }
    }

    bool CalcFeatures(ui32 docId, NDetails::TTitleFeatures& features) const {
        return Impl.Get() ? Impl->CalcFeatures(docId, features) : false;
    }
};

}; // namespace NSequences
