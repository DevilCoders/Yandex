#pragma once

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/compiler.h>

#include <limits>

template <class TDoc, class TRelevanceGetter, ui32 Capacity, ui32 TopSizeMultiplier = 2>
class TArrayDocHeap {
    static_assert(TopSizeMultiplier >= 2);
public:
    using TRelevance = typename TRelevanceGetter::TRelevance;

    explicit TArrayDocHeap(ui32 topSize)
        : RelevanceLowerBound_(std::numeric_limits<TRelevance>::lowest())
        , TopSize_(topSize)
        , Size_(0)
    {
        Y_ENSURE(1 <= TopSize_);
        static_assert(std::is_trivially_copyable_v<TDoc>);
        Data_.resize_uninitialized(TopSizeMultiplier * TopSize_);
    }

    void Push(const TDoc& doc) {
        if (Y_UNLIKELY(!Fits(doc))) {
            return;
        }

        Data_[Size_++] = doc;

        if (Y_UNLIKELY(Size_ == Data_.size())) {
            Chop();
        }
    }

    template <class TConsumer>
    void Finish(TConsumer& consumer) {
        if (Y_LIKELY(Size_ > TopSize_)) {
            Chop();
        }

        Y_ASSERT(consumer.Size() <= TopSize_);
        consumer.Consume(Data_.begin(), Data_.begin() + Min<size_t>(TopSize_ - consumer.Size(), Size_));
    }

    void Finish(TVector<TDoc>* vec) {
        if (Y_LIKELY(Size_ > TopSize_)) {
            Chop();
        }

        vec->insert(vec->end(), Data_.begin(), Data_.begin() + + Min<size_t>(TopSize_ - vec->size(), Size_));
    }

    Y_FORCE_INLINE bool Fits(const TDoc& doc) const {
        return Size_ < TopSize_ || TRelevanceGetter::Get(doc) > RelevanceLowerBound_;
    }

    Y_FORCE_INLINE void Clear() {
        Size_ = 0;
        RelevanceLowerBound_ = std::numeric_limits<TRelevance>::lowest();
    }

    Y_FORCE_INLINE void Reset(ui32 topSize) {
        Y_ENSURE(topSize >= 1);
        Clear();
        if (Y_UNLIKELY(TopSize_ != topSize)) {
            TopSize_ = topSize;
            Data_.resize_uninitialized(TopSizeMultiplier * TopSize_);
        }
    }

    ui32 TopSize() const {
        return TopSize_;
    }

    TRelevance RelevanceLowerBound() const {
        return RelevanceLowerBound_;
    }

private:
    void Chop() {
        Y_ASSERT(Size_ > TopSize_);

        std::nth_element(Data_.begin(), Data_.begin() + TopSize_ - 1, Data_.begin() + Size_, TRelevanceGetter::Compare);
        RelevanceLowerBound_ = TRelevanceGetter::Get(Data_[TopSize_ - 1]);
        Size_ = TopSize_;
    }

    TStackVec<TDoc, Capacity> Data_;

    TRelevance RelevanceLowerBound_ = std::numeric_limits<TRelevance>::lowest();
    ui32 TopSize_ = 1;
    ui32 Size_ = 0;
};
