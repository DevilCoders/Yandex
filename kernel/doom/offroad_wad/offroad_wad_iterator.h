#pragma once

#include <library/cpp/offroad/utility/tagged.h>
#include <library/cpp/offroad/tuple/adaptive_tuple_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>

#include <kernel/doom/info/index_format.h>
#include <kernel/doom/wad/wad_index_type.h>

namespace NDoom {

template <class Hit>
class IOffroadWadIterator {
public:
    virtual ~IOffroadWadIterator() = default;

    virtual bool ReadHit(Hit* hit) = 0;
};

template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadWadIterator final : private NOffroad::NPrivate::TTaggedBase, public IOffroadWadIterator<Hit> {
    using THitReader = NOffroad::TAdaptiveTupleReader<Hit, Vectorizer, Subtractor, PrefixVectorizer>;
    using TKeyReader = NOffroad::TKeyReader<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
public:
    using THit = Hit;

    TOffroadWadIterator() {}

    bool ReadHit(THit* hit) override {
        if (HitReader_.ReadHit(hit)) {
            return hit->DocId() == TermId_ && hit->Range() == TermRange_;
        } else {
            return false;
        }
    }

private:
    template <EWadIndexType IndexType, class OtherHit, class OtherVectorizer, class OtherSubtractor, class OtherPrefixVectorizer>
    friend class TOffroadWadKeySearcher;
    template <EWadIndexType IndexType, class OtherHit, class OtherVectorizer, class OtherSubtractor, class OtherPrefixVectorizer>
    friend class TOffroadWadHitSearcher;
    template <EWadIndexType IndexType, class OtherHit, class OtherVectorizer, class OtherSubtractor, class OtherPrefixVectorizer>
    friend class TOffroadWadSearcher;

private:
    ui32 TermId_ = -1;
    ui32 TermRange_ = -1;
    ui32 DocId_ = -1;
    ui32 KeyRange_ = 0;
    TVector<ui32> PrefetchedDocIds_;
    TVector<TBlob> PrefetchedDocBlobs_;
    TVector<std::array<TArrayRef<const char>, 2>> PrefetchedDocRegions_;
    TBlob Blob_;
    THitReader HitReader_;
    TKeyReader KeyReader_;
};


} // namespace NDoom
