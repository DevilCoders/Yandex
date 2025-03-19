#pragma once

#include "iterator_impl.h"

namespace NReqBundleIteratorImpl {
    // Iterators serialization
    class TIteratorSaveLoad {
    public:
        static void SaveIterator(IOutputStream* rh, const TIterator& iter);

        static TIteratorPtr LoadIterator(
            IInputStream* rh,
            size_t blockId,
            TMemoryPool& iteratorsMemory,
            NReqBundleIterator::IRBIndexIteratorLoader& loader);

        static void SaveBlockIterators(
            IOutputStream* rh,
            TIteratorPtrs::const_iterator begin,
            TIteratorPtrs::const_iterator end);

        static void LoadBlockIterators(
            IInputStream* rh,
            size_t blockId,
            TIteratorPtrs& iterators,
            TMemoryPool& iteratorsMemory,
            NReqBundleIterator::IRBIndexIteratorLoader& loader);

        static void SaveFormIds(
            IOutputStream* rh,
            TVector<ui16>::const_iterator begin,
            TVector<ui16>::const_iterator end);

        static void LoadFormIds(
            IInputStream* rh,
            TVector<ui16>& formIds);
    };

}
