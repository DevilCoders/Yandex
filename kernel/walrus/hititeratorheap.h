#pragma once

#include <cerrno>

#include <util/generic/algorithm.h>

#include <library/cpp/deprecated/autoarray/autoarray.h>
#include <util/system/filemap.h>
#include <util/generic/buffer.h>

#include <kernel/keyinv/indexfile/indexfile.h>
#include <kernel/keyinv/indexfile/indexreader.h>
#include <kernel/keyinv/indexfile/rdkeyit.h>
#include <kernel/keyinv/hitlist/hits_coders.h>

#include "advhititer.h"
#include "lfproc.h"

class THitIteratorHeap : public THeapOfHitIteratorsImpl<TAdvancedHitIterator<TBufferedHitIterator>, TReadKeysIteratorImpl<TAdvancedHitIterator<TBufferedHitIterator> > > {
    typedef TAdvancedHitIterator<TBufferedHitIterator> THitIterator;
    typedef TReadKeysIteratorImpl<THitIterator> TKeyIterator;
    typedef THeapOfHitIteratorsImpl<THitIterator, TKeyIterator> TBase;

    TKeyIterator*                   BaseAddress;
    TVector<THitIterator*>          AdditionalIterators;
    size_t                          AdditionalIteratorsUsed;
    IHitsBufferManager*             BufferManager;

public:
    NIndexerCore::TLemmaAndFormsProcessor    Forms;

public:
    THitIteratorHeap(TKeyIterator* iters, size_t count, IHitsBufferManager* bufferManager = nullptr, bool rawKeys = false, bool stripKeys = false);
    ~THitIteratorHeap();

private:
    THitIterator* GetHitIterator(TKeyIterator* rki);
    bool AddIterators(TNumFormArray& formsMap);

public:
    size_t NextLemma();
};

inline THitIteratorHeap::THitIteratorHeap(TKeyIterator* iters, size_t count, IHitsBufferManager* bufferManager, bool rawKeys, bool stripKeys)
    : TBase(iters, count)
    , BaseAddress(iters)
    , AdditionalIteratorsUsed(0)
    , BufferManager(bufferManager)
    , Forms(rawKeys, stripKeys)
{
}

inline THitIteratorHeap::~THitIteratorHeap() {
    for (size_t n = 0; n < AdditionalIterators.size(); n++)
        delete AdditionalIterators[n];
}

inline THitIteratorHeap::THitIterator* THitIteratorHeap::GetHitIterator(TKeyIterator* rki) {
    if (!rki->HiTitersCount++) {
        rki->NextHit(true);
        return &rki->Hits;
    } else {
        if (size() <= CurrentSize + CurrentHeapSize) { // avoid resizing code in main part of the loop
            TVector<THitIterator*> copy(CurrentSize + CurrentHeapSize * 2);
            memcpy(&copy[0], data(), CurrentSize * sizeof(*data()));
            this->swap(copy);
        }
        if (AdditionalIteratorsUsed >= AdditionalIterators.size())
            AdditionalIterators.push_back(new THitIterator);
        THitIterator* hitsIter = AdditionalIterators[AdditionalIteratorsUsed++];
        hitsIter->Init(rki->GetInvFile(), rki->GetHitFormat(), BufferManager);
        hitsIter->FillHeapData(rki->Hits);
        rki->NextHit(*hitsIter, true);
        return hitsIter;
    }
}

inline bool THitIteratorHeap::AddIterators(TNumFormArray& formsMap) {
    if (!CurrentHeapSize) {
        return false;
    }
    TKeyIterator* top = TheHeap[0];
    strcpy(KeyText, top->Str());

    do {
        PopHeap(&TheHeap[0], &TheHeap[0] + CurrentHeapSize, TPtrGreater());
        THitIterator* hitsIter = GetHitIterator(top);
        memcpy(&hitsIter->FormsMap, &formsMap, sizeof(TNumFormArray));
        (*this)[CurrentSize++] = hitsIter;
        if (top->NextKey())
            PushHeap(&TheHeap[0], &TheHeap[0] + CurrentHeapSize, TPtrGreater());
        else
            CurrentHeapSize--;
    } while (CurrentHeapSize && !strcmp(Str(), (top = TheHeap[0])->Str()));

    return true;
};

