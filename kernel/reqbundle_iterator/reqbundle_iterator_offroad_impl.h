#pragma once

#include <functional>

#include "iterator_impl.h"
#include "reqbundle_iterator.h"

namespace NReqBundleIteratorImpl {
    template<class TOffroadSearcher>
    class TOffroadSharedIteratorData : public TSharedIteratorData {
    public:
        using TOffroadIterator = typename TOffroadSearcher::IIterator;

        TOffroadSearcher& OffroadSearcher;
        THolder<TOffroadIterator> OffroadIterator;

        TOffroadSharedIteratorData(TOffroadSearcher& searcher, THolder<TOffroadIterator>&& iterator, const ISentenceLengthsLenReader* sentReader = nullptr)
            : TSharedIteratorData(sentReader)
            , OffroadSearcher(searcher)
            , OffroadIterator(std::forward<THolder<TOffroadIterator>>(iterator))
        {
        }

        void PreLoadDoc(ui32 id, const NDoom::TSearchDocLoader& loader) override {
            TSharedIteratorData::PreLoadDoc(id, loader);
            OffroadSearcher.PreLoadDoc(id, loader, OffroadIterator.Get());
        }

        void AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer) override {
            OffroadSearcher.FetchDocs(docIds, std::move(consumer), OffroadIterator.Get());
        }
    };
} // NReqBundleIteratorImpl
