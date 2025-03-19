#include "saveload.h"

#include "iterator_and_impl.h"
#include "iterator_ordered_and_impl.h"
#include "reqbundle_iterator_builder.h"

namespace NReqBundleIteratorImpl {
    void TIteratorSaveLoad::SaveIterator(
        IOutputStream* rh,
        const TIterator& iter)
    {
        iter.SaveIteratorImpl(rh);
    }

    TIteratorPtr TIteratorSaveLoad::LoadIterator(
        IInputStream* rh,
        size_t blockId,
        TMemoryPool& iteratorsMemory,
        NReqBundleIterator::IRBIndexIteratorLoader& loader)
    {
        ui8 iterType = -1;
        LoadPodType(rh, iterType);
        switch (iterType) {
            case TIterator::ItAnd: {
                ui32 numWords = 0;
                LoadPodType(rh, numWords);
                return MakeIteratorAnd(numWords, blockId, iteratorsMemory, rh, loader);
            }
            case TIterator::ItOrderedAnd: {
                ui32 numWords = 0;
                LoadPodType(rh, numWords);
                return MakeIteratorOrderedAnd(numWords, blockId, iteratorsMemory, rh, loader);
            }
            case TIterator::ItIndex: {
                return loader.LoadIndexIterator(rh, blockId, iteratorsMemory);
            }
            default: {
                Y_ENSURE(false, "unknown iterator type id: " << (int) iterType);
            }
        }
    }

    void TIteratorSaveLoad::SaveBlockIterators(
        IOutputStream* rh,
        TIteratorPtrs::const_iterator begin,
        TIteratorPtrs::const_iterator end)
    {
        size_t count = end - begin;
        SavePodType(rh, count);

        for (auto it = begin; it != end; ++it) {
            SaveIterator(rh, **it);
        }
    }

    void TIteratorSaveLoad::LoadBlockIterators(
        IInputStream* rh,
        size_t blockId,
        TIteratorPtrs& iterators,
        TMemoryPool& iteratorsMemory,
        NReqBundleIterator::IRBIndexIteratorLoader& loader)
    {
        size_t count = 0;
        LoadPodType(rh, count);
        iterators.reserve(iterators.size() + count);

        for (size_t i = 0; i != count; ++i) {
            iterators.push_back(LoadIterator(rh, blockId, iteratorsMemory, loader));
        }
    }

    void TIteratorSaveLoad::SaveFormIds(
        IOutputStream* rh,
        TVector<ui16>::const_iterator begin,
        TVector<ui16>::const_iterator end)
    {
        size_t count = end - begin;
        SavePodType(rh, count);

        for (auto it = begin; it != end; ++it) {
            ui16 formId = *it;
            SavePodType(rh, formId);
        }
    }

    void TIteratorSaveLoad::LoadFormIds(
        IInputStream* rh,
        TVector<ui16>& formIds)
    {
        size_t count = 0;
        LoadPodType(rh, count);
        formIds.reserve(formIds.size() + count);

        for (size_t i = 0; i != count; ++i) {
            ui16 formId = 0;
            LoadPodType(rh, formId);
            formIds.push_back(formId);
        }
    }
} // NReqBundleIteratorImpl
