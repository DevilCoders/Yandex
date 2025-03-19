#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/system/compiler.h>

template <class TDoc, class TRelevanceGetter>
class TVectorSortRangeConsumer {
public:
    TVectorSortRangeConsumer(TVector<TDoc>* top, size_t topSize)
        : Top_(top)
    {
        Top_->reserve(topSize);
    }

    Y_FORCE_INLINE void Consume(TDoc* begin, TDoc* end) {
        Sort(begin, end, TRelevanceGetter::Compare);
        Top_->insert(Top_->end(), begin, end);
    }

    Y_FORCE_INLINE void Consume(TDoc&& doc) {
        Top_->push_back(std::move(doc));
    }

    Y_FORCE_INLINE size_t Size() const {
        return Top_->size();
    }

private:
    TVector<TDoc>* Top_ = nullptr;
};

template <class TDoc>
class TVectorConsumer {
public:
    TVectorConsumer(TVector<TDoc>* top, size_t topSize)
        : Top_(top)
    {
        Top_->reserve(topSize);
    }

    Y_FORCE_INLINE void Consume(TDoc* begin, TDoc* end) {
        Top_->insert(Top_->end(), begin, end);
    }

    Y_FORCE_INLINE void Consume(TDoc&& doc) {
        Top_->push_back(std::move(doc));
    }

    Y_FORCE_INLINE size_t Size() const {
        return Top_->size();
    }

private:
    TVector<TDoc>* Top_ = nullptr;
};
