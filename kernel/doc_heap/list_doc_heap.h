#pragma once

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/generic/ylimits.h>
#include <util/system/compiler.h>

template <class TDoc, class TRelevanceGetter, ui32 Capacity, ui32 RelevanceThreshold>
class TListDocHeap : private TNonCopyable {
public:
    explicit TListDocHeap(ui32 topSize)
        : TopSize_(topSize)
        , Size_(0)
        , MinRelevance_(RelevanceThreshold)
        , MaxRelevance_(0)
    {
        static_assert(std::is_unsigned<typename TRelevanceGetter::TRelevance>());
        Y_ENSURE(0 < TopSize_ && TopSize_ <= Capacity);
        Fill(First_.begin(), First_.end(), END_INDEX);

        static_assert(std::is_trivially_copyable_v<TDoc>);
        Docs_.resize_uninitialized(Capacity);
    }

    Y_FORCE_INLINE bool Fits(const TDoc& doc) const {
        return Size_ < TopSize_ || TRelevanceGetter::Get(doc) > MinRelevance_;
    }

    void Push(const TDoc& doc) {
        Y_ENSURE(Fits(doc));

        const ui32 relevance = TRelevanceGetter::Get(doc);
        Y_ASSERT(relevance < RelevanceThreshold);

        if (Y_UNLIKELY(relevance > MaxRelevance_)) {
            MaxRelevance_ = relevance;
        }

        if (Y_LIKELY(Size_ == TopSize_)) {
            const ui32 emptySlot = First_[MinRelevance_];
            Y_ASSERT(emptySlot != END_INDEX);
            First_[MinRelevance_] = Next_[emptySlot];

            Docs_[emptySlot] = doc;
            Next_[emptySlot] = First_[relevance];
            First_[relevance] = emptySlot;

            while (First_[MinRelevance_] == END_INDEX) {
                ++MinRelevance_;
            }
        } else {
            if (Y_UNLIKELY(relevance < MinRelevance_)) {
                MinRelevance_ = relevance;
            }

            Docs_[Size_] = doc;
            Next_[Size_] = First_[relevance];
            First_[relevance] = Size_;
            ++Size_;
        }
    }

    template<class TConsumer>
    void Finish(TConsumer& consumer) {
        if (Y_UNLIKELY(Size_ == 0 || consumer.Size() >= TopSize_)) {
            return;
        }

        for (i32 relevance = static_cast<i32>(MaxRelevance_), bound = static_cast<i32>(MinRelevance_); relevance >= bound; --relevance) {
            for (ui32 index = First_[relevance]; index != END_INDEX; index = Next_[index]) {
                consumer.Consume(std::move(Docs_[index]));
                if (consumer.Size() == TopSize_) {
                    return;
                }
            }
        }
    }

    void Finish(TVector<TDoc>* vec) {
        if (Y_UNLIKELY(Size_ == 0 || vec->size() >= TopSize_)) {
            return;
        }

        for (i32 relevance = static_cast<i32>(MaxRelevance_), bound = static_cast<i32>(MinRelevance_); relevance >= bound; --relevance) {
            for (ui32 index = First_[relevance]; index != END_INDEX; index = Next_[index]) {
                vec->emplace_back(std::move(Docs_[index]));
                if (vec->size() == TopSize_) {
                    return;
                }
            }
        }
    }

    Y_FORCE_INLINE void Clear() {
        Size_ = 0;
        MinRelevance_ = RelevanceThreshold;
        MaxRelevance_ = 0;
        Fill(First_.begin(), First_.end(), END_INDEX);
    }

    Y_FORCE_INLINE void Reset(ui32 topSize) {
        Clear();
        TopSize_ = topSize;
    }

private:
    ui32 TopSize_ = 1;
    ui32 Size_ = 0;
    ui32 MinRelevance_ = Max<ui32>();
    ui32 MaxRelevance_ = 0;

    std::array<ui32, RelevanceThreshold> First_;
    std::array<ui32, Capacity> Next_;

    TStackVec<TDoc, Capacity> Docs_;

    static constexpr ui32 END_INDEX = Max<ui32>();
};
