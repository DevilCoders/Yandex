#pragma once

#include "index_yndex_accessor.h"
#include "iterator_impl.h"
#include "iterator_yndex_impl.h"
#include "reqbundle_iterator.h"
#include "reqbundle_iterator_builder.h"

#include <kernel/sent_lens/sent_lens.h>

#include <ysite/yandex/posfilter/filter_tree.h>

using namespace NReqBundleIteratorImpl;

class TReqBundleYndexIteratorBuilder : public IReqBundleIteratorBuilder {
public:
    TReqBundleYndexIteratorBuilder(
        const TYndexRequester& yr,
        IHitsAccess::EIndexType yrType,
        TLangMask langMask,
        TLangMask flatBastardsLangMask,
        const ISentenceLengthsLenReader* sentReader = nullptr)
        : IReqBundleIteratorBuilder(false, false)
        , Index_(&yr.YMain())
        , IndexAccessor_(yr, yrType, langMask, flatBastardsLangMask)
        , SentenceLengthsReader(sentReader)
    {
         Y_ENSURE(yr.IsOpen(), "index not opened");
    }

    THolder<TSharedIteratorData> MakeSharedData() override {
        return MakeHolder<TSharedIteratorData>(SentenceLengthsReader);
    }

    IIndexAccessor& GetIndexAccessor() override {
        return IndexAccessor_;
    }

    TIteratorPtr LoadIndexIterator(IInputStream* rh, size_t blockId, TMemoryPool& iteratorsMemory) override {
        ui8 magicNumber = -1;
        LoadPodType(rh, magicNumber);
        Y_ASSERT(magicNumber == YndexIteratorMagicNumber);
        EIteratorType iteratorType;
        LoadPodType(rh, iteratorType);
        return MakeIteratorYndex(iteratorType, blockId, iteratorsMemory, rh, Index_);
    }

private:
    const IKeysAndPositions* Index_;
    TYndexIndexAccessor IndexAccessor_;
    const ISentenceLengthsLenReader* SentenceLengthsReader = nullptr;
};
