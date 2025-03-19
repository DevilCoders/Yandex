#pragma once

#include "array_doc_heap.h"
#include "list_doc_heap.h"

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/system/compiler.h>

/* void TConsumer::Consume(TDoc* begin, TDoc* end);
   void TConsumer::Consume(TDoc &&doc);
   size_t TConsumer::Size() const;

   static TRelevanceGetter::TRelevance TRelevanceGetter::Get(const TDoc& doc);
   static bool TRelevanceGetter::Compare(const TDoc& doc1, const TDoc& doc2);
*/

template <class TDoc, class TRelevanceGetterLow, class TRelevanceGetterHigh, ui32 Capacity, ui32 RelevanceThreshold>
class TSmartDocHeap {
public:
    explicit TSmartDocHeap(ui32 topSize)
        : HighRelevanceHeap_(topSize)
        , LowRelevanceHeap_(topSize)
    {
    }

    void Push(const TDoc& doc) {
        if (Y_UNLIKELY(TRelevanceGetterLow::Get(doc) >= RelevanceThreshold)) {
            HighRelevanceHeap_.Push(doc);
        } else {
            LowRelevanceHeap_.Push(doc);
        }
    }

    template <class TConsumer>
    void Finish(TConsumer& consumer) {
        HighRelevanceHeap_.Finish(consumer);
        LowRelevanceHeap_.Finish(consumer);
    }

    void Finish(TVector<TDoc>* vec) {
        HighRelevanceHeap_.Finish(vec);
        LowRelevanceHeap_.Finish(vec);
    }

    Y_FORCE_INLINE bool Fits(const TDoc& doc) const {
        if (Y_UNLIKELY(TRelevanceGetterLow::Get(doc) >= RelevanceThreshold)) {
            return HighRelevanceHeap_.Fits(doc);
        } else {
            return LowRelevanceHeap_.Fits(doc);
        }
    }

    Y_FORCE_INLINE void Clear() {
        HighRelevanceHeap_.Clear();
        LowRelevanceHeap_.Clear();
    }

    Y_FORCE_INLINE void Reset(ui32 topSize) {
        HighRelevanceHeap_.Reset(topSize);
        LowRelevanceHeap_.Reset(topSize);
    }

private:
    TArrayDocHeap<TDoc, TRelevanceGetterHigh, Capacity> HighRelevanceHeap_;
    TListDocHeap<TDoc, TRelevanceGetterLow, Capacity, RelevanceThreshold> LowRelevanceHeap_;
};
