#pragma once

#include <type_traits>

#include "offroad_doc_codec.h"
#include "offroad_doc_common.h"
#include "offroad_doc_wad_mapping.h"

namespace NDoom {

template <size_t DocLumpCount>
struct TOffroadDocWadIteratorData : public TSimpleRefCount<TOffroadDocWadIteratorData<DocLumpCount>> {
    TVector<ui32> PrefetchedDocIds_;
    TVector<TBlob> PrefetchedDocBlobs_;
    TVector<std::array<TArrayRef<const char>, DocLumpCount>> PrefetchedDocRegions_;
    TBlob Blob_;
};

template<class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EOffroadDocCodec codec = AdaptiveDocCodec>
class TOffroadDocWadIterator {
    using TReader = typename TOffroadDocCommon<codec, Hit, Vectorizer, Subtractor, PrefixVectorizer>::TReader;

public:
    enum {
        DocLumpCount = PrefixVectorizer::TupleSize == 0 ? 1 : 2
    };

    using THit = Hit;
    using TData = TOffroadDocWadIteratorData<DocLumpCount>;
    using TDataPtr = TIntrusivePtr<TData>;

    TOffroadDocWadIterator()
        : Data_{ MakeIntrusive<TData>() }
    {
    }

    TOffroadDocWadIterator(TDataPtr data)
        : Data_{ data }
    {
    }

    void Reset() {
        Reader_.Reset();
        Data_->PrefetchedDocIds_.clear();
        Data_->PrefetchedDocBlobs_.clear();
        Data_->PrefetchedDocRegions_.clear();
    }

    bool ReadHit(THit* hit) {
        return Reader_.ReadHit(hit);
    }

    template <class Consumer>
    bool ReadHits(const Consumer& consumer) {
        return Reader_.ReadHits(consumer);
    }

    bool LowerBound(const THit& prefix, THit* first) {
        return DoLowerBound(prefix, first, std::integral_constant<size_t, PrefixVectorizer::TupleSize>());
    }

    void Swap(TOffroadDocWadIterator& other) {
        SwapInternal(other);
    }

    TDataPtr GetData() {
        return Data_;
    }

private:
    template<class Any>
    bool DoLowerBound(const THit& prefix, THit* first, Any) {
        return Reader_.LowerBound(prefix, first);
    }

    bool DoLowerBound(const THit&, THit* first, std::integral_constant<size_t, 0>) {
        // TODO: this is evil, drop!

        /* LowerBound is essentially impossible to do, but some clients rely on this operation moving read pointer to start. */
        Reader_.Restart();
        if (!Reader_.ReadHit(first))
            return false;

        Reader_.Restart();
        return true;
    }

    void SwapInternal(TOffroadDocWadIterator& other) {
        Data_.Swap(other.Data_);
        DoSwap(Reader_, other.Reader_);
    }

    template<EWadIndexType, class OtherHit, class OtherVectorizer, class OtherSubtractor, class OtherPrefixVectorizer, EOffroadDocCodec>
    friend class TOffroadDocWadSearcher;

    template <
        EWadIndexType indexType,
        class Hash,
        class HashVectorizer,
        class HashSubtractor,
        class OtherHit,
        class OtherVectorizer,
        class OtherSubtractor,
        EOffroadDocCodec otherCodec,
        size_t blockSize>
    friend class TOffroadHashedKeyInvWadSearcher;

    TVector<ui32>& PrefetchedDocIds() {
        return Data_->PrefetchedDocIds_;
    }

    TVector<TBlob>& PrefetchedDocBlobs() {
        return Data_->PrefetchedDocBlobs_;
    }

    TVector<std::array<TArrayRef<const char>, DocLumpCount>>& PrefetchedDocRegions() {
        return Data_->PrefetchedDocRegions_;
    }

    TBlob& Blob() {
        return Data_->Blob_;
    }

private:
    TDataPtr Data_;
    TReader Reader_;
};


} // namespace NDoom
