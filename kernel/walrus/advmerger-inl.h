#pragma once

#include "yxformbuf.h"
#include "hitbuffermanager.h"
#include "hititeratorheap.h"

#include <library/cpp/containers/mh_heap/mh_heap.h>

namespace {
    //for merge of persts with many keys
    struct TManyKeyHit {
        TManyKeyHit() {}
        TManyKeyHit(ui32 key, SUPERLONG hit)
            : mKey(key)
            , mHit(hit) {
        }

        bool operator< (const TManyKeyHit& other) const {
            return mKey < other.mKey ||
                (mKey == other.mKey && mHit < other.mHit);
        }

        ui32 Key() const {
            return mKey;
        }

        SUPERLONG Hit() const {
            return mHit;
        }
    private:
        ui32        mKey;
        SUPERLONG   mHit;
    };
}

template<class SentWriter>
void TAdvancedIndexMerger::TInternalMerging::Merge(const TOutputKeys& outputKeys, size_t maxKeys, TFormRemap& formRemap, bool haveForms, SentWriter& sentWriter) {
    CurKeys.resize(outputKeys.size());

    for (size_t key = 0; key < maxKeys; ++key) {
        // detect hit formats and remap forms
        for (size_t dst = 0, dstsize = Writers.size(); dst != dstsize; ++dst) {
            if (key >= outputKeys[dst].size())
                continue;
            CurKeys[dst] = outputKeys[dst][key].Key;
            if (haveForms) {
                for (ui32 form = 0; form < (ui32)outputKeys[dst][key].KeyFormCount; ++form) {
                    ui32 gform = outputKeys[dst][key].FormIndexes[form];
                    formRemap[dst][gform] = form;
                }
            }
        }

        MultHitHeap<TAdvancedHitIterator<TBufferedHitIterator> > hitHeap(&InputHeap[0], InputHeap.Size());
        hitHeap.Restart();

        Resolver.Reset();

        for (; hitHeap.Valid(); ++hitHeap) {
            TWordPosition hit(hitHeap.Current());
            TAdvancedHitIterator<TBufferedHitIterator>& hitIter = *hitHeap.TopIter();
            if (DeleteLogic.OnPos(hit.Pos, hitIter.GetUseDeleteLogic()))
                continue;
            ui32 dst, dstDocid;
            if (FinalRemapTable.GetDst(hitIter.ArrIndex, hit.Doc(), dst, dstDocid)) {
                Y_ASSERT(hit.Doc() == dstDocid); // DocId must not be changed
                sentWriter.AddHit(hit.SuperLong(), dst);
                if (TBitIterator::IsBitset(dst)) {
                    if (haveForms) {
                        const ui32 gform = hitIter.FormsMap[hit.Form()];
                        TBitIterator bi(dst);
                        while (bi.Next()) {
                            const ui32 i = bi.Get();
                            const ui32 form = formRemap[i][gform];
                            if (form == (ui32)-1)
                                continue;
                            Resolver.StoreHit(hit.SuperLong(), form, i);
                        }
                    } else {
                        TBitIterator bi(dst);
                        while (bi.Next())
                            Writers[bi.Get()]->WriteHit(hit.SuperLong());
                    }
                } else {
                    if (haveForms) {
                        const ui32 gform = hitIter.FormsMap[hit.Form()];
                        const ui32 form = formRemap[dst][gform];
                        if (form == (ui32)-1)
                            continue;
                        Resolver.StoreHit(hit.SuperLong(), form, dst);
                    } else
                        Writers[dst]->WriteHit(hit.SuperLong());
                }
            }
        }
        for (size_t dst = 0, dstsize = Writers.size(); dst != dstsize; ++dst) {
            if (key >= outputKeys[dst].size())
                continue;
            if (haveForms) {
                for (ui32 form = 0; form < (ui32)outputKeys[dst][key].KeyFormCount; ++form) {
                    ui32 gform = outputKeys[dst][key].FormIndexes[form];
                    formRemap[dst][gform] = (ui32)-1;
                }
                Resolver.StorePrevHit(dst);
            }
            Writers[dst]->WriteKey(CurKeys[dst]);
        }
    }

    CurKeys.clear();
}

template<class SentWriter>
bool TAdvancedIndexMerger::Run(SentWriter& sentWriter, TVector<ui32>* docHits, size_t bufSize) {
    if (HasMemoryPortions) {
        return MergeMemoryPortions();
    }

    if (RawKeys && !UseRemapTables && !UseDeleteLogic && Task->FinalRemapTable.IsEmpty() &&
        !Task->UniqueHits && // actually it can be implemented in TOldIndexFileImpl::StoreNextHit()
        Task->Outputs.size() && Task->Outputs[0].Version == YNDEX_VERSION_RAW64_HITS) {
        return MergeNoForms();
    } else if ((!ExternalHitWriter) && (!UseRemapTables && (Task->FinalRemapTable.IsEmpty() || !Task->FinalRemapTable.HasChangedDocIds()) && !Task->UniqueHits)) {
        return MergeNoDocIdChanges(sentWriter, docHits, bufSize);
    } else {
        return Merge(sentWriter, docHits, bufSize);
    }
}

template<class SentWriter>
bool TAdvancedIndexMerger::MergeNoDocIdChanges(SentWriter& sentWriter, TVector<ui32>* docHits, size_t bufSize) {
    BufferManager.Reset(new THitsBufferManager());
    const TInputIndices indices(CreateIndexIterators(InputFiles, BufferManager.Get(), Task->UseDirectInput));

    const size_t outputBufferSize = 1024 * 1024;
    TVector< TSimpleSharedPtr<NIndexerCore::TOutputIndexFile> > outputs;
    TVector< TSimpleSharedPtr<TInvKeyWriterHitCount> > writers;
    TSorterBuffers sorterBuffers;

    for (size_t dst = 0; dst < Task->Outputs.size(); ++dst) {
        const TAdvancedMergeTask::TMergeOutput& mergeOutput = Task->Outputs[dst];
        TSimpleSharedPtr<NIndexerCore::TOutputIndexFile> output(new NIndexerCore::TOutputIndexFile(IYndexStorage::FINAL_FORMAT, mergeOutput.Version));
        size_t bufSz = dst == 0 ? bufSize : outputBufferSize;
        output->Open(mergeOutput.FilePrefix.data(), bufSz, bufSz, false);
        outputs.push_back(output);
        writers.push_back(TSimpleSharedPtr<TInvKeyWriterHitCount>(new TInvKeyWriterHitCount(*output, mergeOutput.HasSubIndex, dst == 0 ? docHits : nullptr)));
    }

    THitIteratorHeap inputHeap(indices.Array, indices.Count, BufferManager.Get(), RawKeys, Task->StripKeys);

    TFormCounts formCounts;     // [dst][globalform] - for each output index, count of hits for each global form for current lemma
    formCounts.resize(outputs.size());

    TFormRemap formRemap;       // [dst][globalform] - for each output index, remap of global word form to its own word form
    formRemap.resize(outputs.size());

    TOutputKeys outputKeys;     // [dst][dstkey] - for each output index, the keys that go into that index
    outputKeys.resize(outputs.size());

    TInternalMerging internalMerging(inputHeap, writers, DeleteLogic, Task->FinalRemapTable);

    // main loop
    while (inputHeap.NextLemma()) {
        if (StopFlag && (*StopFlag))
            return false;
        // skip deleted keys from indices that need delete logic
        if (DeleteLogic.OnKey(inputHeap.Str())) { // it will work for attributes only? because keys with lemma can differ from each other...
            for (size_t i = 0, size = inputHeap.Size(); i < size; ++i) {
                TAdvancedHitIterator<TBufferedHitIterator>* const hi = inputHeap[i];
                if (hi->GetUseDeleteLogic())
                    hi->Drain();
            }
        }

        bool haveForms = (0 != inputHeap.Forms.GetTotalFormCount());

        if (haveForms) {
            CountWordForms(formCounts, inputHeap);
            ConstructKeys(formCounts, formRemap, outputKeys, inputHeap.Forms);
        } else {
            ConstructEmptyKeys(outputKeys, inputHeap.Forms);
        }

        // calculate maximum number of keys
        size_t maxKeys = 0;
        for (size_t dst = 0, dstsize = outputs.size(); dst < dstsize; ++dst) {
            if (maxKeys < outputKeys[dst].size())
                maxKeys = outputKeys[dst].size();
        }

        sentWriter.SetKey(inputHeap.Str());
        if (maxKeys < ExternalMergeLimit)
            internalMerging.Merge(outputKeys, maxKeys, formRemap, haveForms, sentWriter);
        else
            ExternalMerge<TManyKeyHit, SentWriter>(inputHeap, writers, sorterBuffers, outputKeys, formRemap, haveForms, sentWriter);
    }

    for (size_t i = 0; i < outputs.size(); ++i)
        outputs[i]->CloseEx();
    return true;
}

template<class SentWriter>
bool TAdvancedIndexMerger::Merge(SentWriter& sentWriter, TVector<ui32>* docHits, size_t bufSize) {
    const TInputIndices indices(CreateIndexIterators(InputFiles, nullptr, Task->UseDirectInput));

    TVector< TSimpleSharedPtr<YxFileWBL> > outputs; // in case of exception they are closed from destructors
    CreateOutputs(outputs, bufSize);

    TInputHeap inputHeap(indices.Array, indices.Count);
    while (inputHeap.NextKey()) {
        if (StopFlag && (*StopFlag))
            return false;
        // skip deleted keys from indices that need delete logic
        if (DeleteLogic.OnKey(inputHeap.Str())) {
            for (size_t i = 0; i < inputHeap.Size(); ++i) {
                TAdvancedHitIterator<TBufferedHitIterator>* hi = inputHeap[i];
                if (hi->GetUseDeleteLogic())
                    hi->Drain();
            }
        }

        for (size_t i = 0; i < outputs.size(); ++i)
            outputs[i]->SetKey(inputHeap.Str());

        sentWriter.SetKey(inputHeap.Str());
        if (ExternalHitWriter)
            ExternalHitWriter->StartKey(inputHeap.Str());
        for (size_t n = 0; n < inputHeap.Size(); ++n) {
            TAdvancedHitIterator<TBufferedHitIterator>* hi = inputHeap[n];
            while (hi->Next()) {
                TWordPosition hit(hi->Current());
                if (DeleteLogic.OnPos(hit.SuperLong(), hi->GetUseDeleteLogic()))
                    continue;

                ui32 dstCluster = 0;
                ui32 newDocId = 0;
                if (Task->FinalRemapTable.GetDst(hi->ArrIndex, hit.Doc(), dstCluster, newDocId)) {
                    hit.SetDoc(newDocId);
                    sentWriter.AddHit(hit.SuperLong(), dstCluster);
                    AddDocHit(docHits, hit);
                    if (TBitIterator::IsBitset(dstCluster)) {
                        TBitIterator bi(dstCluster);
                        while (bi.Next()) {
                            if (ExternalHitWriter)
                                ExternalHitWriter->AddHit(hi->ArrIndex, bi.Get(), outputs[bi.Get()], hit);
                            else
                                outputs[bi.Get()]->AddHit(hit.SuperLong());
                        }
                    } else {
                        if (ExternalHitWriter)
                            ExternalHitWriter->AddHit(hi->ArrIndex, dstCluster, outputs[dstCluster], hit);
                        else
                            outputs[dstCluster]->AddHit(hit.SuperLong());
                    }
                }
            }
        }
        if (ExternalHitWriter)
            ExternalHitWriter->FinishKey();
    }
    return true;
}

template <typename TKeyHit, class SentWriter>
void TAdvancedIndexMerger::ExternalMerge(THitIteratorHeap& inputHeap, TIndexWriters& writers, TSorterBuffers& sorterBuffers, TOutputKeys& outputKeys, TFormRemap& formRemap, bool haveForms, SentWriter& sentWriter) {
    TFormRemap keyRemap;
    keyRemap.resize(writers.size());
    TVector< TAutoPtr< NSorter::TSorter<TKeyHit> > > Sorters;
    for (size_t dst = 0, dstsize = writers.size(); dst < dstsize; ++dst) {
        keyRemap[dst].resize(inputHeap.Forms.GetTotalFormCount(), (ui32)-1);
        TVector<NIndexerCore::TOutputKey>& outKeys = outputKeys[dst];
        for (size_t key = 0; key < outKeys.size(); ++key) {
            for (ui32 form = 0; form < (ui32)outKeys[key].KeyFormCount; ++form) {
                ui32 gform = outKeys[key].FormIndexes[form];
                formRemap[dst][gform] = form;
                keyRemap[dst][gform] = key;
            }
        }
        while (dst >= sorterBuffers.size())
            sorterBuffers.push_back(new char[SorterBuffersSize]);
        Sorters.push_back(new NSorter::TSorter<TKeyHit>(SorterBuffersSize / sizeof(TKeyHit), (TKeyHit*)sorterBuffers[dst].Get()));
        const TString portionNamePrefix((Task->TmpFileDir.empty() ? (Task->Outputs[dst].FilePrefix + "-mergetmp") : (Task->TmpFileDir + "/tmp")) + ToString(getpid()) + "-");
        Sorters.back()->SetFileNameCallback(new NSorter::TPortionFileNameCallback2((portionNamePrefix + ToString(dst)).data()), 0);
    }

    for (size_t n = 0; n < inputHeap.Size(); n++) {
        TAdvancedHitIterator<TBufferedHitIterator>& hitIter = *inputHeap[n];
        hitIter.Restart();
        for (; hitIter.Valid(); ++hitIter) {
            TWordPosition hit(hitIter.Current());
            if (DeleteLogic.OnPos(hit.Pos, hitIter.GetUseDeleteLogic()))
                continue;
            const ui32 gform = hitIter.FormsMap[hit.Form()];
            ui32 dst, dstDocid;
            if (Task->FinalRemapTable.GetDst(hitIter.ArrIndex, hit.Doc(), dst, dstDocid)) {
                hit.SetDoc(dstDocid);
                sentWriter.AddHit(hit.SuperLong(), dst);
                if (TBitIterator::IsBitset(dst)) {
                    if (haveForms) {
                        TBitIterator bi(dst);
                        while (bi.Next()) {
                            const ui32 i = bi.Get();
                            const ui32 key = keyRemap[i][gform];
                            hit.SetWordForm(formRemap[i][gform]);
                            Sorters[i]->PushBack(TKeyHit(key, hit.SuperLong()));
                        }
                    } else {
                        TBitIterator bi(dst);
                        while (bi.Next())
                            Sorters[bi.Get()]->PushBack(TKeyHit(0, hit.SuperLong()));
                    }
                } else {
                    ui32 key = 0;
                    if (haveForms) {
                        hit.SetWordForm(formRemap[dst][gform]);
                        key = keyRemap[dst][gform];
                    }
                    Sorters[dst]->PushBack(TKeyHit(key, hit.SuperLong()));
                }
            }
        }
    }

    for (ui32 dst = 0, dstsize = writers.size(); dst < dstsize; ++dst) {
        ui32 curKey = Max<ui32>();
        if (!Sorters[dst]->NumPortions()) {
            //optimization for short persts
            TKeyHit* it;
            TKeyHit* end;
            Sorters[dst]->SortAndRelease(it, end);
            for (; it != end; ++it) {
                if (it->Key() != curKey) {
                    if (curKey != Max<ui32>())
                        writers[dst]->WriteKey(haveForms ? outputKeys[dst][curKey].Key : inputHeap.Str());
                    curKey = it->Key();
                }
                writers[dst]->WriteHit(it->Hit());
            }
        } else {
            typename NSorter::TIterator<TKeyHit> it;
            for (Sorters[dst]->Close(it); !it.Finished(); ++it) {
                if (it->Key() != curKey) {
                    if (curKey != Max<ui32>())
                        writers[dst]->WriteKey(haveForms ? outputKeys[dst][curKey].Key : inputHeap.Str());
                    curKey = it->Key();
                }
                writers[dst]->WriteHit(it->Hit());
            }
        }
        if (curKey != Max<ui32>())
            writers[dst]->WriteKey(haveForms ? outputKeys[dst][curKey].Key : inputHeap.Str());
    }
}
