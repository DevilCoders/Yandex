#pragma once

#include "reqbundle_hits_provider.h"

#include <kernel/reqbundle/reqbundle_fwd.h>

#include <util/generic/ptr.h>

namespace NBaseSearch {
    template <typename TKey>
    class ILruBlobCache;
} // NBaseSearch

namespace NReqBundleIteratorImpl {

} // NReqBundleIteratorImpl

namespace NReqBundleIterator {
    namespace NImpl {
        using namespace NReqBundleIteratorImpl;
    } // NImpl

    using namespace NReqBundle;

    class TPosition;
    class TRBIterator;
    struct TRBIteratorOptions;
    class TRBIteratorsFactory;
    class IRBIteratorBuilder;
    class TRBIteratorsHashers;
    struct TPosBuf;
    class IRBIteratorBuilderGlobal;
    class IRBSharedDataFactory;
    class IRBIndexIteratorLoader;
    class IRBIteratorDeserializer;

    using IRBIteratorsHasher = NBaseSearch::ILruBlobCache<NReqBundle::TBinaryBlock>;

    using TRBIteratorPtr = THolder<TRBIterator>;

    using TBlockType = ui8;
    constexpr TBlockType DefaultBlockType = 0;
} // NReqBundleIterator

using IReqBundleHitsProvider = NReqBundleIterator::IRBHitsProvider;
using TReqBundleIterator = NReqBundleIterator::TRBIterator;
using TReqBundleIteratorOptions = NReqBundleIterator::TRBIteratorOptions;
using TReqBundleIteratorPtr = NReqBundleIterator::TRBIteratorPtr;
using TReqBundleIteratorsFactory = NReqBundleIterator::TRBIteratorsFactory;
using IReqBundleIteratorBuilder = NReqBundleIterator::IRBIteratorBuilder;
using TReqBundleIteratorsHashers = NReqBundleIterator::TRBIteratorsHashers;
using IReqBundleIteratorsHasher = NReqBundleIterator::IRBIteratorsHasher;
