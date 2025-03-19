#include "main_content_impl.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/stream/str.h>
#include <util/memory/pool.h>
#include <kernel/matrixnet/mn_dynamic.h>
#include <library/cpp/resource/resource.h>
#include <limits>

namespace NSegm {
namespace NPrivate {

struct TFeatureRankInfo {
    float AbsoluteRankWeight = 0;
    float RelativeRankWeight = 0;
};

struct TSegFeature {
    float Value = 0;
    size_t SegIdx = 0;

    TSegFeature(float value, size_t segIdx)
        : Value(value)
        , SegIdx(segIdx)
    {
    }

    static bool ValueGreater(const TSegFeature& a, const TSegFeature& b) {
        return a.Value > b.Value;
    }
};


class TRankFeaturesCalculator {
private:
    size_t FeatureNumber = -1;
    size_t AbsoluteRankFeatureNumber = -1;
    size_t RelativeRankFeatureNumber = -1;

    TVector<TSegFeature, TPoolAllocator> SegmentsFeatures;
    TVector<TFeatureRankInfo, TPoolAllocator> SegmentNumberToFeatureValue;
public:
    explicit TRankFeaturesCalculator(size_t featureNumber, TMemoryPool* pool, size_t segmentsCount)
        : FeatureNumber(featureNumber)
        , AbsoluteRankFeatureNumber((size_t) -1)
        , RelativeRankFeatureNumber((size_t) -1)
        , SegmentsFeatures(pool)
        , SegmentNumberToFeatureValue(pool)
    {
        SegmentsFeatures.reserve(segmentsCount);
        SegmentNumberToFeatureValue.resize(segmentsCount);
    }

    TRankFeaturesCalculator SetAbsoluteRankFeatureNumber(size_t absoluteRankFeatureNumber) {
        TRankFeaturesCalculator result(*this);
        result.AbsoluteRankFeatureNumber = absoluteRankFeatureNumber;
        return result;
    }

    TRankFeaturesCalculator SetRelativeRankFeatureNumber(size_t relativeRankFeatureNumber) {
        TRankFeaturesCalculator result(*this);
        result.RelativeRankFeatureNumber = relativeRankFeatureNumber;
        return result;
    }

    TRankFeaturesCalculator SetRankFeatureNumbers(size_t absoluteRankFeatureNumber, size_t relativeRankFeatureNumber) {
        TRankFeaturesCalculator result(*this);
        result.AbsoluteRankFeatureNumber = absoluteRankFeatureNumber;
        result.RelativeRankFeatureNumber = relativeRankFeatureNumber;
        return result;
    }

    void Add(const TVector<float>& features, size_t segIdx) {
        SegmentsFeatures.push_back(TSegFeature(features[FeatureNumber], segIdx));
    }

    void Finish() {
        Sort(SegmentsFeatures.begin(), SegmentsFeatures.end(), TSegFeature::ValueGreater);

        float prevValue = SegmentsFeatures[0].Value;
        size_t rank = 0;

        size_t realSegmentsCount = SegmentsFeatures.size();

        for (size_t i = 0; i < realSegmentsCount; ++i) {
            if (SegmentsFeatures[i].Value != prevValue) {
                rank = i;
                prevValue = SegmentsFeatures[i].Value;
            }
            TFeatureRankInfo& featureRankInfo = SegmentNumberToFeatureValue[SegmentsFeatures[i].SegIdx];
            featureRankInfo.AbsoluteRankWeight = log(rank + 1.0);
            featureRankInfo.RelativeRankWeight = 1.f - (float)rank / realSegmentsCount;
        }
    }

    void Set(TVector<float>& features, size_t segIdx) {
        Y_ASSERT(features.size() == MCF_COUNT * 3 && "Wrong features size");
        Y_ASSERT(FeatureNumber <= MCF_COUNT * 3 && "FeatureNumber is too big");
        const TFeatureRankInfo* featureRankInfo = &SegmentNumberToFeatureValue[segIdx];
        Y_ASSERT(featureRankInfo && "Feature not found in FeatureValueToRank");
        if (AbsoluteRankFeatureNumber != (size_t) -1) {
            features[AbsoluteRankFeatureNumber] = featureRankInfo->AbsoluteRankWeight;
        }
        if (RelativeRankFeatureNumber != (size_t) -1) {
            features[RelativeRankFeatureNumber] = featureRankInfo->RelativeRankWeight;
        }
    }
};

class TRankFeaturesCalculators {
private:
    TVector<TRankFeaturesCalculator> Calculators;
public:
    TRankFeaturesCalculators(TMemoryPool* pool, size_t segmentsCount) {
        Calculators.reserve(15);

        Calculators.push_back(TRankFeaturesCalculator(MCF_WORDS, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_WORDS_RANK, MCF_WORDS_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_LINKS_RELRANK, pool, segmentsCount)
            .SetRelativeRankFeatureNumber(MCF_LINKS_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_LINKS_PER_WORD, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_LINKS_PER_WORD_RANK, MCF_LINKS_PER_WORD_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_LINK_WORDS, pool, segmentsCount)
            .SetRelativeRankFeatureNumber(MCF_LINK_WORDS_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_LINK_WORDS_PER_WORD, pool, segmentsCount)
            .SetRelativeRankFeatureNumber(MCF_LINK_WORDS_PER_WORD_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_BLOCKS, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_BLOCKS_RANK, MCF_BLOCKS_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_BLOCKS_PER_WORD, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_BLOCKS_PER_WORD_RANK, MCF_BLOCKS_PER_WORD));
        Calculators.push_back(TRankFeaturesCalculator(MCF_HEADERS_PER_SENT, pool, segmentsCount)
            .SetAbsoluteRankFeatureNumber(MCF_HEADERS_PER_SENT_RANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_SENTS_PER_WORD, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_SENTS_PER_WORD_RANK, MCF_SENTS_PER_WORD_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_ALPHAS_PER_SYMBOL, pool, segmentsCount)
            .SetRankFeatureNumbers(MCF_ALPHAS_PER_SYMBOL_RANK, MCF_ALPHAS_PER_SYMBOL_RELRANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_SPACES_PER_SYMBOL, pool, segmentsCount)
            .SetAbsoluteRankFeatureNumber(MCF_SPACES_PER_SYMBOL_RANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_TITLE_WORDS, pool, segmentsCount)
            .SetAbsoluteRankFeatureNumber(MCF_TITLE_WORDS_RANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_FOOTER_WORDS_RANK, pool, segmentsCount)
            .SetAbsoluteRankFeatureNumber(MCF_FOOTER_WORDS_RANK));
        Calculators.push_back(TRankFeaturesCalculator(MCF_METADESCR_WORDS, pool, segmentsCount)
            .SetAbsoluteRankFeatureNumber(MCF_METADESCR_WORDS_RANK));
    }

    void Add(const TVector<float>& features, size_t segIdx) {
        for (size_t i = 0; i < Calculators.size(); ++i) {
            Calculators[i].Add(features, segIdx);
        }
    }

    void Finish() {
        for (size_t i = 0; i < Calculators.size(); ++i) {
            Calculators[i].Finish();
        }
    }

    void Set(TVector<float>& features, size_t segIdx) {
        for (size_t i = 0; i < Calculators.size(); ++i) {
            Calculators[i].Set(features, segIdx);
        }
    }

    void Set(TSegmentFeatures& segmentFeatures) {
        for (size_t segIdx = 0; segIdx < segmentFeatures.size(); ++segIdx) {
            Set(segmentFeatures[segIdx].F, segIdx);
        }
    }
};

void TSegmentFeatures::Calculate(const TDocContext& ctx, const THeaderSpans&, const TSegmentSpans& segs) {
    if (!ctx.Words)
        return;

    bool commheader = false;
    bool commcss = false;
    ui32 wordssofar = 0;

    Y_ASSERT(Pool && "no pool found");
    TRankFeaturesCalculators rankFeaturesCalculators(Pool, segs.size());

    reserve(segs.size());

    size_t segIdx = 0;
    for (TSegmentSpans::const_iterator it = segs.begin(); it != segs.end(); ++it, ++segIdx) {
        commcss |= it->CommentsCSS;
        commheader |= it->CommentsHeader;

        if (!it->MainContentCandidate())
            continue;

        TVector<float>& features = PushBack(it - segs.begin()).F;

        features[MCF_ADS_CSS] = it->AdsCSS;
        features[MCF_COMMENTS_CSS] = it->CommentsCSS;

        features[MCF_TYPE_TRASH] = it->Type == STP_FOOTER || it->Type == STP_HEADER || it->Type == STP_MENU || it->Type == STP_AUX;
        features[MCF_TYPE_LINK] = it->Type == STP_REFERAT || it->Type == STP_LINKS;
        features[MCF_TYPE_CONTENT] = it->Type == STP_CONTENT;

        features[MCF_IN_ARTICLE] = it->InArticle;
        features[MCF_IN_MAIN_CONTENT_READABILITY] = it->InReadabilitySpans;

        features[MCF_BEFORE_COMMENT_HEADER] = !commheader;
        features[MCF_BEFORE_COMMENT_CSS] = !commcss;

        features[MCF_BLOCKS_TO_PREV_ROOT] = log(1.0 + it->FirstBlock.Blocks);
        features[MCF_PARS_TO_PREV_ROOT] = log(1.0 + it->FirstBlock.Paragraphs);
        features[MCF_ITEMS_TO_PREV_ROOT] = log(1.0 + it->FirstBlock.Items);

        features[MCF_BLOCKS_TO_NEXT_ROOT] = log(1.0 + it->LastBlock.Blocks);
        features[MCF_PARS_TO_NEXT_ROOT] = log(1.0 + it->LastBlock.Paragraphs);
        features[MCF_ITEMS_TO_NEXT_ROOT] = log(1.0 + it->LastBlock.Items);

        float segnum = size() - 1;
        features[MCF_SEGNUM_TO_TOTAL] = segnum / ctx.GoodSegs;

        float midsent = (it->Begin.Sent() + it->End.Sent()) / 2;
        features[MCF_MIDDLE_SENT_TO_TOTAL] = midsent / segs.back().End.Sent();

        features[MCF_FIRST_WORD] = log(1.0 + wordssofar);
        features[MCF_FIRST_WORD_TO_TOTAL] = float(wordssofar) / ctx.Words;

        features[MCF_MIDDLE_WORD] = log(1.0 + wordssofar + it->Words / 2);
        features[MCF_MIDDLE_WORD_TO_TOTAL] = float(wordssofar + it->Words / 2) / ctx.Words;

        wordssofar += it->Words;

        features[MCF_WORDS] = log(1.0 + it->Words);
        features[MCF_WORDS_TO_TOTAL] = float(it->Words) / ctx.Words;
        features[MCF_WORDS_TO_MAX_WORDS] = float(it->Words) / ctx.MaxWords;

        float links = it->Links;
        features[MCF_LINKS_RELRANK] = log(1.0 + it->Links);
        features[MCF_LINKS_PER_WORD] = float(links) / it->Words;

        float domains = it->Domains;
        features[MCF_DOMAINS_PER_LINK] = it->Links ? domains / it->Links : 0;

        features[MCF_LOCAL_LINKS] = log(1.0 + it->LocalLinks);
        features[MCF_LOCAL_LINKS_PER_LINK] = it->Links ? float(it->LocalLinks) / it->Links : 0;

        features[MCF_LINK_WORDS] = log(1.0 + it->LinkWords);
        features[MCF_LINK_WORDS_PER_WORD] = float(it->LinkWords) / it->Words;

        features[MCF_BLOCKS] = log(1.0 + it->Blocks);
        features[MCF_BLOCKS_PER_WORD] = float(it->Blocks) / it->Words;

        float sents = it->Sentences();
        features[MCF_SENTS_PER_WORD] = sents / it->Words;

        float heads = it->HeadersCount;
        features[MCF_HEADERS_PER_SENT] = heads / it->Sentences();

        features[MCF_DOMAINS_PER_LINK] = it->Links ? float(it->Domains) / it->Links : 0;
        features[MCF_LOCAL_LINKS_PER_LINK] = it->Links ? float(it->LocalLinks) / it->Links : 1;

        features[MCF_HEADERS_PER_SENT] = float(it->HeadersCount) / it->Sentences();

        features[MCF_LINK_WORDS_PER_WORD] = float(it->LinkWords) / it->Words;
        features[MCF_LINKS_PER_WORD] = Min(1.f, float(it->Links) / it->Words);
        features[MCF_BLOCKS_PER_WORD] = Min(1.f, float(it->Blocks) / it->Words);
        features[MCF_SENTS_PER_WORD] = Min(1.f, float(it->Sentences()) / it->Words);

        features[MCF_ALPHAS_PER_SYMBOL] = float(it->AlphasInText) / it->SymbolsInText;
        features[MCF_SPACES_PER_SYMBOL] = float(it->SpacesInText) / it->SymbolsInText;
        features[MCF_COMMAS_PER_SYMBOL] = float(it->CommasInText) / it->SymbolsInText;
        features[MCF_WORDS_PER_SYMBOL] = float(it->Words) / it->SymbolsInText;

        features[MCF_TITLE_WORDS] = log(1.0 + it->TitleWords);
        features[MCF_TITLE_WORDS_TO_TITLE] = ctx.TitleWords ? float(it->TitleWords) / ctx.TitleWords : 0;

        features[MCF_FOOTER_WORDS_RANK] = log(1.0 + it->FooterWords);
        features[MCF_FRAGMENT_LINKS] = log(1.0 + it->FragmentLinks);
        features[MCF_HAS_SELF_LINK] = it->HasSelfLink;

        features[MCF_CONTENT_BLOCKS_COUNT] = log(1.0 + segs.size());
        features[MCF_METADESCR_WORDS_TOTAL] = log(1.0 + ctx.MetaDescrWords);
        features[MCF_METADESCR_WORDS] = log(1.0 + it->MetaDescrWords);
        features[MCF_METADESCR_WORDS_TO_METADESCR] = ctx.MetaDescrWords ? float(it->MetaDescrWords) / ctx.MetaDescrWords : 0;
        rankFeaturesCalculators.Add(features, segIdx);
    }

    rankFeaturesCalculators.Finish();
    rankFeaturesCalculators.Set(*this);

    for (TSegmentFeatures::iterator it = begin(); it != end(); ++it) {
        if (it != begin()) {
            for (ui32 i = 0; i < MCF_COUNT; ++i)
                it->F[MCF_COUNT + i] = (it - 1)->F[i];
        }

        if (it + 1 != end()) {
            for (ui32 i = 0; i < MCF_COUNT; ++i)
                it->F[2 * MCF_COUNT + i] = (it + 1)->F[i];
        }
    }

}

class TMnSseDynamicWrapper {
public:
    TMnSseDynamicWrapper() {
        try {
            Model = NResource::Find("/main_content_matrixnet_model");
            TStringInput in(Model);
            MatrixnetSse.Load(&in);
        } catch (const std::exception& e) {
            Cerr << "Failed to load model matrixnet model for segmentator: " << e.what() << "\n";
            _exit(1);
        }
    }
    inline void Apply(const TVector<TVector<float> >& factors, TVector<double>& result) const {
        MatrixnetSse.CalcRelevs(factors, result);
    }
private:
    NMatrixnet::TMnSseDynamic MatrixnetSse;
    TString Model;
};

struct TMatrixNet {
    static void Apply(const TVector<TVector<float> >& factors, TVector<double>& result) {
        TMnSseDynamicWrapper* SseMatrixnet = Singleton<TMnSseDynamicWrapper>();
        SseMatrixnet->Apply(factors, result);
    }
};


TMainContentSpans FindMainContentSpans(const TDocContext& ctx,
                                       const THeaderSpans& hp,
                                       TSegmentSpans& sp,
                                       TMemoryPool* pool)
{
    if (sp.empty())
        return TMainContentSpans();

    TSegmentFeatures v(sp.size(), pool);
    v.Calculate(ctx, hp, sp);
    v.SetWeights<TMatrixNet>(sp);

    TSegmentSpans spans = sp;
    StableSort(spans.begin(), spans.end(), TWeightComparator());

    TMainContentSpans mains;

    for (TSegmentSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
        if (it->Weight < 0.03869058846)
            continue;

        mains.push_back(*it);
    }

    Sort(mains.begin(), mains.end());

    TMainContentSpans mmains;
    mmains.reserve(mains.size());

    for (TMainContentSpans::const_iterator it = mains.begin(); it != mains.end(); ++it) {
        if (it == mains.begin() || it->Begin.SentAligned() > mmains.back().End.SentAligned())
            mmains.push_back(*it);
        else
            mmains.back().End = it->End;
    }

    return mmains;
}

}
}
