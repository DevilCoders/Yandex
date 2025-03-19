#pragma once

#include "doc_omni_accessor.h"
#include "legacy_omnidoc.h"
#include "omnidoc_fwd.h"
#include "template_stuff.h"

#include <util/folder/dirut.h>
#include <util/generic/ptr.h>
#include <util/generic/array_ref.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NDoom {
    class IWad;
}

namespace NDocOmniWadIndexDetail {
    template <class T>
    struct TGetSearcher {
        using TType = THolder<typename T::TSearcher>;
    };
}

class TDocOmniWadIndex: public TOldOmniIndexObsoleteDiePlease {
    friend class TOmniAccessorFactory;
    friend class TOmniAccessors;

    template <class Ios>
    struct TSearchersFromIos: public TRebindPackWithSubtype<std::tuple, Ios, NDocOmniWadIndexDetail::TGetSearcher> {};

    template <class Searchers>
    struct TSearcherTypesFromSearchers: public TRebindPackFromHoldersTuple<TTypeList, Searchers> {};

    using TStringsWadIos = TTypeList<
        NDoom::TOmniUrlIo,
        NDoom::TOmniTitleIo,
        NDoom::TOmniAliceWebMusicTrackTitleIo,
        NDoom::TOmniAliceWebMusicArtistNameIo,
        NDoom::TOmniRelCanonicalTargetIo
    >;

    using TL1CompressedWadIos = TTypeList<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingIo>;

    using TL1RawWadIos = TTypeList<NDoom::TOmniDssmLogDwellTimeBigramsEmbeddingRawIo, NDoom::TOmniDssmPantherTermsEmbeddingRawIo>;

    using TL2CompressedWadIos = TTypeList<
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding1Io,
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding2Io,
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding3Io,
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding4Io,
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbedding5Io,
        NDoom::TOmniDssmAnnXfDtShowOneCompressedEmbeddingIo,
        NDoom::TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingIo,
        NDoom::TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnCtrCompressedEmbeddingIo,
        NDoom::TOmniAnnRegStatsIo,
        NDoom::TOmniDssmAggregatedAnnRegEmbeddingIo,
        NDoom::TOmniDssmMainContentKeywordsEmbeddingIo,
        NDoom::TOmniDssmBertDistillL2EmbeddingRawIo,
        NDoom::TOmniDssmNavigationL2EmbeddingRawIo,
        NDoom::TOmniDssmFullSplitBertEmbeddingRawIo,
        NDoom::TOmniDssmSinsigL2EmbeddingRawIo,
        NDoom::TOmniReservedEmbeddingRawIo
    >;
    using TL2RawWadIos = TTypeList<
        NDoom::TOmniDssmAnnXfDtShowWeightCompressedEmbeddingsRawIo,
        NDoom::TOmniDssmAnnXfDtShowOneCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnXfDtShowOneSeCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnXfDtShowOneSeAmSsHardCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnSerpSimilarityHardSeCompressedEmbeddingRawIo,
        NDoom::TOmniDssmAnnCtrCompressedEmbeddingRawIo,
        NDoom::TOmniAnnRegStatsRawIo,
        NDoom::TOmniDssmAggregatedAnnRegEmbeddingRawIo,
        NDoom::TOmniDssmMainContentKeywordsEmbeddingRawIo,
        NDoom::TOmniRecDssmSpyTitleDomainCompressedEmb12RawIo,
        NDoom::TOmniRecCFSharpDomainRawIo,
        NDoom::TOmniDssmBertDistillL2EmbeddingRawIo,
        NDoom::TOmniDssmNavigationL2EmbeddingRawIo,
        NDoom::TOmniDssmFullSplitBertEmbeddingRawIo,
        NDoom::TOmniDssmSinsigL2EmbeddingRawIo,
        NDoom::TOmniReservedEmbeddingRawIo
    >;

    using TAllIos = TTypeListCat<
        TStringsWadIos,
        TL1CompressedWadIos,
        TL1RawWadIos,
        TL2CompressedWadIos,
        TL2RawWadIos
    >::TType;

    using TStringsWadSearchers = TSearchersFromIos<TStringsWadIos>;
    using TL1CompressedWadSearchers = TSearchersFromIos<TL1CompressedWadIos>;
    using TL1RawWadSearchers = TSearchersFromIos<TL1RawWadIos>;
    using TL2CompressedWadSearchers = TSearchersFromIos<TL2CompressedWadIos>;
    using TL2RawWadSearchers = TSearchersFromIos<TL2RawWadIos>;


public:
    template<class Io>
    const typename Io::TSearcher* GetSearcher () const {
        const typename Io::TSearcher* result = GetSearcherFromTuple<Io>(
            StringsWadSearchers_,
            L1CompressedWadSearchers_,
            L1RawWadSearchers_,
            L2CompressedWadSearchers_,
            L2RawWadSearchers_
        );

        return result;
    }

    template <class Accessor>
    auto GetByAccessor(const ui32 docId, Accessor* accessor) const {
        if (UseOmniLegacy_) {
            return TOldOmniIndexObsoleteDiePlease::GetField(docId, accessor);
        }
        Y_ASSERT(accessor->IsInited());
        return accessor->GetHit(docId);
    }

    template <class Io, class OmniAccessors>
    auto Get(const ui32 docId, OmniAccessors* accessors) const {
        if (UseOmniLegacy_) {
            auto accessor = accessors->template GetAccessor<Io>();
            return TOldOmniIndexObsoleteDiePlease::GetField(docId, accessor);
        }

        return Get3<Io>(docId, accessors, typename NIndexDoc::THasCompatibilityLayers<Io>::TType());
    }

    template<class Io, class OmniAccessors>
    bool IsInited(OmniAccessors* accessors) const {
        if (UseOmniLegacy_) {
            const auto* accessor = accessors->template GetAccessor<Io>();
            return accessor->IsInited();
        }
        using TIoList = typename NIndexDoc::TCompatibilityLayersWithHead<Io>::TType;
        return IsInited1<TIoList, OmniAccessors>(accessors);
    }

    TDocOmniWadIndex(const NDoom::IWad* wad, const NDoom::IWad* l1wad, const NDoom::IWad* l2wad, const TString& oldIndexPath, bool useWadIndex);
    TDocOmniWadIndex(const NDoom::IChunkedWad* l1wad, const NDoom::IChunkedWad* l2wad, const NDoom::IDocLumpMapper* mapper);
    ~TDocOmniWadIndex();

    size_t Size() const;

private:
    //
    // Get
    //
    template<class Io, class OmniAccessors>
    auto GetStrict(const ui32 docId, OmniAccessors* accessors) const {
        auto accessor = accessors->template GetAccessor<Io>();
        Y_ASSERT(accessor->IsInited());
        return accessor->GetHit(docId);
    }

    template<class Io, class OtherLayers, class OmniAccessors>
    auto Get1(const ui32 docId, OmniAccessors* accessors, std::false_type) const {
        return GetStrict<typename OtherLayers::THead>(docId, accessors);
    }

    template<class Io, class OtherLayers, class OmniAccessors>
    auto Get1(const ui32 docId, OmniAccessors* accessors, std::true_type) const {
        auto accessor = accessors->template GetAccessor<typename OtherLayers::THead>();
        if (accessor->IsInited()) {
            return accessor->GetHit(docId);
        } else {
            return Get2<Io, typename OtherLayers::TTail> (docId, accessors);
        }
    }

    template<class Io, class OtherLayers, class OmniAccessors>
    auto Get2(const ui32 docId, OmniAccessors* accessors) const {
        return Get1<Io, OtherLayers>(docId, accessors, std::integral_constant<bool, (OtherLayers::Length > 1)>());
    }

    template<class Io, class OmniAccessors>
    auto Get3(const ui32 docId, OmniAccessors* accessors, std::true_type) const {
        using TOtherLayers = typename NIndexDoc::TCompatibilityLayers<Io>::TType;
        auto accessor = accessors->template GetAccessor<Io>();
        if (accessor->IsInited()) {
            return accessor->GetHit(docId);
        } else {
            return Get2<Io, TOtherLayers>(docId, accessors);
        }
    }

    template<class Io, class OmniAccessors>
    auto Get3(const ui32 docId, OmniAccessors* accessors, std::false_type) const {
        return GetStrict<Io>(docId, accessors);
    }

    //
    // IsInited
    //
    template<class Io, class OmniAccessors>
    static bool IsInited0(OmniAccessors* accessors) {
        const auto* accessor = accessors->template GetAccessor<Io>();
        return accessor->IsInited();
    }

    template<class IoList, class OmniAccessors>
    static constexpr bool IsInited1(OmniAccessors*, std::enable_if_t<std::is_same<IoList, TNone>::value, TNone> = TNone()) {
        return false;
    }

    template<class IoList, class OmniAccessors>
    static bool IsInited1(OmniAccessors* accessors, std::enable_if_t<!(std::is_same<IoList, TNone>::value), TNone> = TNone()) {
        return IsInited0<typename IoList::THead, OmniAccessors>(accessors) || IsInited1<typename IoList::TTail, OmniAccessors>(accessors);
    }

    //
    // GetSearcher
    //
    template <class SearcherType, class SearcherTypes, class Searchers>
    const SearcherType* GetSearcher0(const Searchers&, std::false_type) const {
        return nullptr;
    }

    template <class SearcherType, class SearcherTypes, class Searchers>
    const SearcherType* GetSearcher0(const Searchers& searchers, std::true_type) const {
        constexpr size_t value = TIndexOfType<SearcherType, SearcherTypes>::Value;
        return std::get<value>(searchers).Get();
    }

    template <class SearcherType, class SearcherTypes, class Searchers>
    const SearcherType* GetSearcher1(const Searchers& searchers) const {
        return GetSearcher0<SearcherType, SearcherTypes>(searchers, std::integral_constant<bool, SearcherTypes::template THave<SearcherType>::value>());
    }

    template <class Io, class Searchers>
    const typename Io::TSearcher* GetSearcher2(const Searchers& searchers) const {
        return GetSearcher1<typename Io::TSearcher, typename TSearcherTypesFromSearchers<Searchers>::TType>(searchers);
    }

    template <class SearcherTypes, class SearcherType>
    constexpr bool ListHasSearcher() const {
        return SearcherTypes::template THave<SearcherType>::value;
    }

    template <class Io, class ... Searchers>
    const typename Io::TSearcher* GetSearcherFromTuple(const Searchers&... searchers) const {
        constexpr bool oneListHasSearcher = TDisjunction<typename TSearcherTypesFromSearchers<Searchers>::TType::template THave<typename Io::TSearcher>...>::value;
        static_assert (oneListHasSearcher, "Searcher asked is not in class lists");

        const typename Io::TSearcher* result  = nullptr;
        const typename Io::TSearcher* searcher = nullptr;

        auto dummy = {
            (
                searcher = GetSearcher2<Io>(searchers),
                result = searcher ? searcher : result
            )...
        };

        Y_UNUSED(dummy);
        return result;
    }

private:
    TStringsWadSearchers::TType StringsWadSearchers_ ;
    TL1CompressedWadSearchers::TType L1CompressedWadSearchers_;
    TL1RawWadSearchers::TType L1RawWadSearchers_;
    TL2CompressedWadSearchers::TType L2CompressedWadSearchers_;
    TL2RawWadSearchers::TType L2RawWadSearchers_;
};


class TOmniAccessorFactory {
public:
    template<class Io>
    static THolder<TDocOmniIndexAccessor<Io>> NewAccessor(const TDocOmniWadIndex* index) {
        //With nullptr case accessor is used only for Compatibility Buffer
        return THolder<TDocOmniIndexAccessor<Io>>(new TDocOmniIndexAccessor<Io>(index ? index->GetSearcher<Io>() : nullptr));
    }

    template<class Io>
    static THolder<TDocOmniIndexAccessor<Io>> NewAccessor(const TDocOmniWadIndex* index, const TSearchDocDataHolder* docDataHolder) {
        //With nullptr case accessor is used only for Compatibility Buffer
        return THolder<TDocOmniIndexAccessor<Io>>(new TDocOmniIndexAccessor<Io>(index ? index->GetSearcher<Io>() : nullptr, docDataHolder));
    }
};
