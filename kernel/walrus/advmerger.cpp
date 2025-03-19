#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/folder/dirut.h>
#include <kernel/keyinv/indexfile/indexutil.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/memoryportion.h>
#include <library/cpp/sorter/sorter.h>
#include <util/folder/path.h>

#include "advmerger.h"
#include "keys.h"

using namespace NIndexerCore;

void TAdvancedIndexMerger::TFormCollisionResolver::StoreHit(SUPERLONG hit, ui32 form, ui32 dst) {
    TWordPosition::SetWordForm(hit, form);
    THitCacheItem& item = HitCache[dst];

    // if the difference is in non-form part of the hit
    if ((hit ^ item.PrevHit) & ~((SUPERLONG)NFORM_LEVEL_Max)) {
        if (item.HitCount == 1)
            Writers[dst]->WriteHit(item.PrevHit);
        else if (item.HitCount > 1)
            SaveFormPosCollision(dst);
        item.HitCount = 1;
        item.PrevHit = hit;
    } else {
        item.Forms.push_back(form);
        item.HitCount++;
    }
}

void TAdvancedIndexMerger::TFormCollisionResolver::StorePrevHit(ui32 dst) {
    THitCacheItem& item = HitCache[dst];
    if (item.HitCount == 1)
        Writers[dst]->WriteHit(item.PrevHit);
    else if (item.HitCount > 1)
        SaveFormPosCollision(dst);
}

void TAdvancedIndexMerger::TFormCollisionResolver::Reset() {
    for (THitCache::iterator it = HitCache.begin(), end = HitCache.end(); it != end; ++it) {
        it->PrevHit = -1;
        it->HitCount = 0;
        it->Forms.clear();
    }
}

void TAdvancedIndexMerger::TFormCollisionResolver::SaveFormPosCollision(ui32 dst) {
    TInvKeyWriterHitCount& writer = *Writers[dst];
    THitCacheItem& item = HitCache[dst];

    TWordPosition hit(item.PrevHit);
    item.Forms.push_back(hit.Form());
    Sort(item.Forms.begin(), item.Forms.end());
    for (TVector<ui32>::const_iterator it = item.Forms.begin(), end = item.Forms.end(); it != end; ++it) {
        hit.SetWordForm(*it);
        writer.WriteHit(hit.SuperLong());
    }
    item.Forms.clear();
}

TAdvancedIndexMerger::TInputIndices TAdvancedIndexMerger::CreateIndexIterators(const TVector<TIndexFileInfo>& inputFiles, IHitsBufferManager* bufferManager, bool directInput) {
    TIndexIterator* const arr = (TIndexIterator*)y_allocate(sizeof(TIndexIterator) * inputFiles.size());

    size_t i = 0;
    try {
        for (; i != inputFiles.size(); ++i) {
            const TIndexFileInfo& info = inputFiles[i];
            TIndexIterator* const p = arr + i;
            new (p) TIndexIterator(info.MergeInput->Format, info.MergeInput->Version, info.MergeInput->HasSubIndex, i < 2 ? 27 : TBufferedHitIterator::DEF_PAGE_SIZE_BITS);
            if (info.MergeInput->Format == IYndexStorage::FINAL_FORMAT) {
                if (info.MergeInput->NeedsRemap)
                    p->Hits.SetRemapTable(&(info.MergeInput->RemapTable));
                if (info.MergeInput->UseDeleteLogic)
                    p->Hits.SetUseDeleteLogic();
            }
//            fprintf(stderr, "opening %s\n", info.InvName.c_str());
//            size_t bufSize = i < 2 ? 128 << 20 : INIT_FILE_BUF_SIZE;
            p->Init(info.KeyName.c_str(), info.InvName.c_str(), bufferManager, directInput && (i < 2));
        }
    } catch (...) {
        for (TIndexIterator* p = arr + i - 1; p >= arr; --p)
            p->~TIndexIterator();
        y_deallocate(arr);
        throw;
    }

    return TInputIndices(arr, inputFiles.size());
}

void TAdvancedIndexMerger::DestroyIndexIterators(TInputIndices& inputIndices) {
    TIndexIterator* const end = inputIndices.Array + inputIndices.Count;
    for (TIndexIterator* p = inputIndices.Array; p != end; ++p) {
        p->Close();
        p->~TIndexIterator();
    }
    y_deallocate(inputIndices.Array);
}

void TAdvancedIndexMerger::SetExternalHitWriter(IExternalHitWriter* externalHitWriter) {
    ExternalHitWriter = externalHitWriter;
}


void TAdvancedIndexMerger::TryAddMergerInput(const TAdvancedMergeTask::TMergeInput& mergeInput, const TString& keySuffix, const TString& invSuffix) {
    const TString keyName = mergeInput.FilePrefix + keySuffix;
    if (NFs::Exists(keyName))
        InputFiles.push_back(TIndexFileInfo(keyName, mergeInput.FilePrefix + invSuffix, &mergeInput));
}

TAdvancedIndexMerger::TAdvancedIndexMerger(TSimpleSharedPtr<const TAdvancedMergeTask> task, const std::atomic<bool>* stopFlag, bool rawKeys)
    : DeleteLogic(task->ExcludedKeys, task->DeleteZero, task->InvertExcludedKeys)
    , RawKeys(true)
    , UseRemapTables(false)
    , UseDeleteLogic(false)
    , HasMemoryPortions(false)
    , StopFlag(stopFlag)
    , ExternalHitWriter(nullptr)
{
    if (task->Inputs.empty())
        warnx("No input indices specified");
    Task = task;

    for (size_t src = 0; src < Task->Inputs.size(); ++src) {
        const TAdvancedMergeTask::TMergeInput& mergeInput = Task->Inputs[src];
        if (mergeInput.Format == IYndexStorage::FINAL_FORMAT) {
            if (mergeInput.FilePrefix.empty()) {
                HasMemoryPortions = true;
                InputFiles.push_back(TIndexFileInfo(&mergeInput));
            } else {
                if (mergeInput.NeedsRemap)
                    UseRemapTables = true;
                if (mergeInput.UseDeleteLogic)
                    UseDeleteLogic = true;
                InputFiles.push_back(TIndexFileInfo(mergeInput.FilePrefix + KEY_SUFFIX, mergeInput.FilePrefix + INV_SUFFIX, &mergeInput));
            }
        } else if (mergeInput.Format == IYndexStorage::PORTION_FORMAT) {
            if (mergeInput.FilePrefix.empty()) {
                HasMemoryPortions = true;
                InputFiles.push_back(TIndexFileInfo(&mergeInput));
            } else {
                if (mergeInput.FilePrefix.back() == 'w') {
                    TryAddMergerInput(mergeInput, "k", "i");
                } else {
                    TryAddMergerInput(mergeInput, "ak", "ai");
                    TryAddMergerInput(mergeInput, "lk", "li");
                }
            }
        } else if (mergeInput.Format == IYndexStorage::PORTION_FORMAT_ATTR) {
            TryAddMergerInput(mergeInput, "ak", "ai");
        } else if (mergeInput.Format == IYndexStorage::PORTION_FORMAT_LEMM) {
            TryAddMergerInput(mergeInput, "lk", "li");
        }
        if (mergeInput.Version != YNDEX_VERSION_RAW64_HITS)
            RawKeys = false;
    }
    if (rawKeys)
        RawKeys = true;
}

void TAdvancedIndexMerger::CreateOutputs(TVector< TSimpleSharedPtr<YxFileWBL> >& outputs, size_t bufSize) {
    const size_t outputBufferSize = 1024 * 1024;

    int flags = (Task->SortHits ? YxFileWBL::NeedSort : 0) | (YxFileWBL::UseMmap);
    if (Task->UniqueHits)
        flags |= YxFileWBL::FormPosUnic;
    if (Task->StripKeys)
        flags |= YxFileWBL::StripKeys;
    if (RawKeys)
        flags |= YxFileWBL::RawKeys;

    for (size_t dst = 0; dst < Task->Outputs.size(); ++dst) {
        const TAdvancedMergeTask::TMergeOutput& mergeOutput = Task->Outputs[dst];
        TSimpleSharedPtr<YxFileWBL> output(new YxFileWBL(dst == 0 ? bufSize : outputBufferSize,
            (flags | (mergeOutput.HasSubIndex ? 0 : YxFileWBL::NoSubIndex)), mergeOutput.Version, outputBufferSize));
        int fileBufSize = dst == 0 ? 128 << 20 : NIndexerCore::INIT_FILE_BUF_SIZE;
        output->Open(mergeOutput.FilePrefix.data(), ToString(getpid()).data(), Task->TmpFileDir.data(), Task->UseDirectOutput, fileBufSize, fileBufSize);
        outputs.push_back(output);
    }
}

void TAdvancedIndexMerger::CountWordForms(TFormCounts& formCounts, THitIteratorHeap& inputHeap) {
    for (TFormCounts::iterator it = formCounts.begin(), end = formCounts.end(); it != end; ++it) {
        (*it).clear();
        (*it).resize(inputHeap.Forms.GetTotalFormCount(), 0);
    }
    for (size_t src = 0; src < inputHeap.Size(); ++src) {
        TAdvancedHitIterator<TBufferedHitIterator>* iter = inputHeap[src];
        if (Task->FinalRemapTable.IsEmpty())
            iter->CountWordForms(formCounts, nullptr, 0, DeleteLogic, Task->FinalRemapTable.GetDefaultOutputCluster());
        else
            iter->CountWordForms(formCounts, Task->FinalRemapTable.GetInputClusterItems(iter->ArrIndex),
                Task->FinalRemapTable.GetInputClusterSize(iter->ArrIndex), DeleteLogic, Task->FinalRemapTable.GetDefaultOutputCluster());
    }
}

void TAdvancedIndexMerger::ConstructKeys(TFormCounts& formCounts, TFormRemap& formRemap, TOutputKeys& outputKeys, NIndexerCore::TLemmaAndFormsProcessor& lemforms)
{
    for (size_t dst = 0, dstsize = Task->Outputs.size(); dst != dstsize; ++dst) {
        for (size_t form = 0, formsize = lemforms.GetTotalFormCount(); form < formsize; ++form)
            lemforms.SetFormCount(form, formCounts[dst][form]);
        lemforms.ConstructOutKeys(outputKeys[dst], true);
        formCounts[dst].clear();
        formRemap[dst].clear();
        formRemap[dst].resize(lemforms.GetTotalFormCount(), (ui32)-1);
    }
}

void TAdvancedIndexMerger::ConstructEmptyKeys(TOutputKeys& outputKeys, NIndexerCore::TLemmaAndFormsProcessor& lemforms) {
    for (size_t dst = 0, dstsize = Task->Outputs.size(); dst != dstsize; ++dst) {
        lemforms.ConstructOutKeys(outputKeys[dst], true);
    }
}

void MergeTempAttributes(ui32 /*maxDocId*/, const TString& oldIndexPrefix, const TString& reindexedIndexPrefix, const TString& resultPrefix, const TString& tempFileDir) {
    TAdvancedMergeTask::TMergeInput oldInput(oldIndexPrefix, IYndexStorage::FINAL_FORMAT, false, true); // Delete all entries except temporary attributes
    TAdvancedMergeTask::TMergeInput reindexedInput(reindexedIndexPrefix);
    TAdvancedMergeTask::TMergeOutput output(resultPrefix);

    TSimpleSharedPtr<TAdvancedMergeTask> mergeTask(new TAdvancedMergeTask());
    mergeTask->Inputs.push_back(reindexedInput);
    mergeTask->Inputs.push_back(oldInput);
    mergeTask->Outputs.push_back(output);
    mergeTask->ExcludedKeys = WalrusExcludedKeys;
    mergeTask->InvertExcludedKeys = true;
    mergeTask->DeleteZero = DZ_OLD;
    mergeTask->TmpFileDir = tempFileDir;
    TAdvancedIndexMerger merger(mergeTask);
    merger.Run();
}

void AddInputPortions(TAdvancedMergeTask& task, const TVector<TString>& names, IYndexStorage::FORMAT format, ui32 version) {
    for (size_t i = 0; i < names.size(); ++i) {
        TAdvancedMergeTask::TMergeInput input(names[i], format);
        input.Version = version;
        input.HasSubIndex = false;
        task.Inputs.push_back(input);
    }
}

void AddInputPortions(TAdvancedMergeTask& task, const TVector<TPortionBuffers>& buffers, IYndexStorage::FORMAT format, ui32 version) {
    for (size_t i = 0; i < buffers.size(); ++i) {
        TAdvancedMergeTask::TMergeInput input(buffers[i], format);
        input.Version = version; // in case of FINAL_FORMAT will be read from inv-stream
        task.Inputs.push_back(input);
    }
}

void AddInputPortions(TAdvancedMergeTask& task, const TVector<const NIndexerCore::TMemoryPortion*>& portions, IYndexStorage::FORMAT format, ui32 version) {
    for (size_t i = 0; i < portions.size(); ++i) {
        const NIndexerCore::TMemoryPortion* p = portions[i];
        const TBuffer& keybuf = p->GetKeyBuffer();
        const TBuffer& invbuf = p->GetInvBuffer();
        TAdvancedMergeTask::TMergeInput input(TPortionBuffers(keybuf.Data(), keybuf.Size(), invbuf.Data(), invbuf.Size()), format);
        input.Version = version;
        task.Inputs.push_back(input);
    }
}

void MergePortions(NIndexerCore::TIndexStorageFactory& storageFactory, ui32 outputVersion, bool outputHasSubIndex, bool sortHits, const TVector<ui32>& remap) {
    {
        const bool rawKeys = (storageFactory.Flags & Portion_NoForms);
        const ui32 inputVersion = (rawKeys ? YNDEX_VERSION_RAW64_HITS : YNDEX_VERSION_CURRENT); // see also TIndexStorageFactory::GetStorage()

        TSimpleSharedPtr<TAdvancedMergeTask> task(new TAdvancedMergeTask());
        task->StripKeys = true;
        task->SortHits = sortHits;
        if (!remap.empty()) {
            task->FinalRemapTable.Create(1, 0);
            for (ui32 id = 0; id < remap.size(); ++id) {
                if (remap[id] == (ui32)-1)
                    task->FinalRemapTable.SetDeletedDoc(0, id);
                else
                    task->FinalRemapTable.SetNewDocId(0, id, remap[id]);
            }
        }
        AddInputPortions(*task, storageFactory.GetNames(), inputVersion);
        task->Outputs.push_back(TAdvancedMergeTask::TMergeOutput(storageFactory.GetIndexPrefix().c_str(), outputVersion, outputHasSubIndex));

        TAdvancedIndexMerger merger(task);
        merger.Run();
    }
    storageFactory.RemovePortions();
}

bool TAdvancedIndexMerger::Run(const char* sentlens, TVector<ui32>* docHits, size_t bufSize) {
    if (sentlens == nullptr) {
        TFakeLens lens;
        return Run(lens, docHits, bufSize);
    }
    else {
        TSentMultiWriter lens(Task->Outputs.size());
        bool res = Run(lens, docHits, bufSize);
        if (res) {
            bool isAbs = TFsPath(sentlens).IsAbsolute();
            Y_ASSERT(!isAbs || Task->Outputs.size() == 1);
            for (ui32 dest = 0; dest < Task->Outputs.size(); ++dest) {
                TString sentPath = Task->Outputs[dest].FilePrefix + sentlens;
                if (isAbs)
                    sentPath = sentlens;
                lens.Save(sentPath.data(), dest);
            }
        }
        return res;
    }
}

bool TAdvancedIndexMerger::MergeNoForms() {
    // all inputs YNDEX_VERSION_RAW64_HITS, no remap tables, don't use delete logic
    // single output YNDEX_VERSION_RAW64_HITS
    Y_ASSERT(Task->FinalRemapTable.IsEmpty());
    // Task->SortHits is true/false - hits must be already sorted and they will be sorted in the output
    Y_ASSERT(!Task->UniqueHits); // @todo it can be implemented in TOldIndexFileImpl::StoreNextHit()
    Y_ASSERT(Task->Outputs.size() && !Task->Outputs[0].HasSubIndex);

    const TInputIndices indices(CreateIndexIterators(InputFiles, nullptr, Task->UseDirectInput));

    TVector< TSimpleSharedPtr<TRawIndexFile> > outputs;
    const size_t outputBufferSize = 1024 * 1024;
    TRawIndexFile output;
    output.Open(Task->Outputs[0].FilePrefix.data(), outputBufferSize, outputBufferSize, true);

    TInputHeap inputHeap(indices.Array, indices.Count);
    while (inputHeap.NextKey(true)) {
        if (StopFlag && StopFlag->load())
            return false;
        if (inputHeap.Size() == 1 && inputHeap[0]->GetCount()) { // hits can be stored "as blob" - they can't have subindex
            inputHeap[0]->WriteHits(output);
        } else {
            MultHitHeap<TAdvancedHitIterator<TBufferedHitIterator> > hitHeap(&inputHeap[0], inputHeap.Size());
            hitHeap.Restart();
            for (; hitHeap.Valid(); ++hitHeap)
                output.StoreNextHit(hitHeap.Current());
        }

        output.FlushNextKey(inputHeap.Str());
    }

    output.CloseEx();
    return true;
}

struct TCombinedHitIterator : private TNonCopyable {
    TAdvancedHitIterator<TMemoryHitIterator>* MemoryHitIterator;
    TAdvancedHitIterator<TBufferedHitIterator>* FileHitIterator;

    TCombinedHitIterator()
        : MemoryHitIterator(nullptr)
        , FileHitIterator(nullptr)
    {
    }
};

class TCombinedKeyIterator : private TNonCopyable {
    THolder<TMemoryKeyIterator> MemoryKeyIterator;
    typedef TReadKeysIteratorImpl< TAdvancedHitIterator<TBufferedHitIterator> > TFileKeyIterator;
    THolder<TFileKeyIterator> FileKeyIterator;
public:
    TCombinedHitIterator Hits;

public:
    void SetHitIteratorIndex(size_t i) {
        if (!MemoryKeyIterator)
            FileKeyIterator->SetHitIteratorIndex(i);
        else
            MemoryKeyIterator->SetHitIteratorIndex(i);
    }
    void Close() {
        if (!MemoryKeyIterator)
            FileKeyIterator->Close();
    }
    void Init(const TPortionBuffers& portionBuffers, IYndexStorage::FORMAT format) {
        MemoryKeyIterator.Reset(new TMemoryKeyIterator());
        MemoryKeyIterator->Init(portionBuffers, format);
        Hits.MemoryHitIterator = &MemoryKeyIterator->Hits;
    }
    void Init(const char* keyName, const char* invName, IYndexStorage::FORMAT format) {
        FileKeyIterator.Reset(new TFileKeyIterator(format));
        FileKeyIterator->Init(keyName, invName, nullptr, true);
        Hits.FileHitIterator = &FileKeyIterator->Hits;
    }
    const char* Str() const {
        return (!MemoryKeyIterator ? FileKeyIterator->Str() : MemoryKeyIterator->Str());
    }
    bool NextKey() {
        return (!MemoryKeyIterator ? FileKeyIterator->NextKey() : MemoryKeyIterator->NextKey());
    }
    void NextHit(bool memorize = false) {
        (!MemoryKeyIterator ? FileKeyIterator->NextHit(memorize) : MemoryKeyIterator->NextHit(memorize));
    }
    bool operator>(const TCombinedKeyIterator& other) const {
        const char* const thisStr = (!MemoryKeyIterator ? FileKeyIterator->Str() : MemoryKeyIterator->Str());
        const char* const otherStr = (!other.MemoryKeyIterator ? other.FileKeyIterator->Str() : other.MemoryKeyIterator->Str());
        return strcmp(thisStr, otherStr) > 0;
    }
};

struct TAdvancedIndexMerger::TCombinedKeyIterators : private TNonCopyable {
    TCombinedKeyIterator* const Array;
    const size_t Count;

    explicit TCombinedKeyIterators(const TVector<TIndexFileInfo>& inputFiles)
        : Array((TCombinedKeyIterator*)y_allocate(sizeof(TCombinedKeyIterator) * inputFiles.size()))
        , Count(inputFiles.size())
    {
        size_t i = 0;
        try {
            for (; i < Count; ++i) {
                const TIndexFileInfo& info = inputFiles[i];
                const TAdvancedMergeTask::TMergeInput& mergeInput = *info.MergeInput;
                TCombinedKeyIterator* p = Array + i;
                new (p) TCombinedKeyIterator();
                if (mergeInput.FilePrefix.empty()) {
                    p->Init(mergeInput.PortionBuffers, info.MergeInput->Format);
                } else {
                    p->Init(info.KeyName.c_str(), info.InvName.c_str(), info.MergeInput->Format);
                    if (info.MergeInput->Format == IYndexStorage::FINAL_FORMAT) {
                        if (info.MergeInput->NeedsRemap)
                            p->Hits.FileHitIterator->SetRemapTable(&(info.MergeInput->RemapTable));
                        if (info.MergeInput->UseDeleteLogic)
                            p->Hits.FileHitIterator->SetUseDeleteLogic();
                    }
                }
            }
        } catch (const yexception& e) {
            for (TCombinedKeyIterator* p = Array + i - 1; p >= Array; --p)
                p->~TCombinedKeyIterator();
            y_deallocate(Array);
            throw e;
        }
    }
    ~TCombinedKeyIterators() {
        TCombinedKeyIterator* const end = Array + Count;
        for (TCombinedKeyIterator* p = Array; p != end; ++p) {
            p->Close();
            p->~TCombinedKeyIterator();
        }
        y_deallocate(Array);
    }
};

template <typename THitIterator>
static void AddHits(THitIterator* hi, TAttrDeleteLogic& deleteLogic, const TFinalRemapTable& remapTable, TVector< TSimpleSharedPtr<YxFileWBL> >& outputs) {
    while (hi->Next()) {
        TWordPosition hit(hi->Current());
        if (deleteLogic.OnPos(hit.SuperLong(), hi->GetUseDeleteLogic()))
            continue;

        ui32 dstCluster = 0;
        ui32 newDocId = 0;
        if (remapTable.GetDst(hi->ArrIndex, hit.Doc(), dstCluster, newDocId)) {
            hit.SetDoc(newDocId);
            if (TBitIterator::IsBitset(dstCluster)) {
                TBitIterator bi(dstCluster);
                while (bi.Next())
                    outputs[bi.Get()]->AddHit(hit.SuperLong());
            } else
                outputs[dstCluster]->AddHit(hit.SuperLong());
        }
    }
}

bool TAdvancedIndexMerger::MergeMemoryPortions() {
    TCombinedKeyIterators keyIterators(InputFiles);

    TVector< TSimpleSharedPtr<YxFileWBL> > outputs; // in case of exception they are closed from destructors
    CreateOutputs(outputs);

    THeapOfHitIteratorsImpl<TCombinedHitIterator, TCombinedKeyIterator> inputHeap(keyIterators.Array, keyIterators.Count);
    while (inputHeap.NextKey()) {
        if (StopFlag && StopFlag->load())
            return false;
        // skip deleted keys from indices that need delete logic
        if (DeleteLogic.OnKey(inputHeap.Str())) {
            for (size_t i = 0; i < inputHeap.Size(); ++i) {
                TCombinedHitIterator* hi = inputHeap[i];
                if (!hi->MemoryHitIterator) {
                    if (hi->FileHitIterator->GetUseDeleteLogic())
                        hi->FileHitIterator->Drain();
                }
            }
        }

        for (size_t i = 0; i < outputs.size(); ++i)
            outputs[i]->SetKey(inputHeap.Str());

        for (size_t n = 0; n < inputHeap.Size(); ++n) {
            TCombinedHitIterator* hi = inputHeap[n];
            if (!hi->MemoryHitIterator)
                AddHits(hi->FileHitIterator, DeleteLogic, Task->FinalRemapTable, outputs);
            else
                AddHits(hi->MemoryHitIterator, DeleteLogic, Task->FinalRemapTable, outputs);
        }
    }

    return true;
}

struct TMemoryKeyIterators : private TNonCopyable {
    const size_t Count;
    TMemoryKeyIterator* const Array;

    template <typename TBuffers>
    TMemoryKeyIterators(const TBuffers* input, size_t count, IYndexStorage::FORMAT format)
        : Count(count)
        , Array((TMemoryKeyIterator*)y_allocate(sizeof(TMemoryKeyIterator) * Count))
    {
        size_t i = 0;
        try {
            for (; i < Count; ++i) {
                TMemoryKeyIterator* p = Array + i;
                new (p) TMemoryKeyIterator();
                p->Init(input[i], format);
            }
        } catch (const yexception& e) {
            for (TMemoryKeyIterator* p = Array + i - 1; p >= Array; --p)
                p->~TMemoryKeyIterator();
            y_deallocate(Array);
            throw e;
        }
    }
    ~TMemoryKeyIterators() {
        for (TMemoryKeyIterator* p = Array; p != Array + Count; ++p)
            p->~TMemoryKeyIterator();
        y_deallocate(Array);
    }
};

template <typename TBuffers>
inline void MergeMemoryPortionsImpl(const TBuffers* input, size_t count, IYndexStorage::FORMAT format,
    const ui32* remap, bool intersectedRanges, NIndexerCore::TMemoryPortion& output)
{
    TMemoryKeyIterators iterators(input, count, format);
    THeapOfHitIteratorsImpl<TMemoryKeyIterator::THitIterator, TMemoryKeyIterator> inputHeap(iterators.Array, iterators.Count);
    TVector<std::pair<ui32, TMemoryKeyIterator::THitIterator*> > hitIterators;
    TVector<SUPERLONG> keyHits;
    while (inputHeap.NextKey()) {
        output.StoreKey(inputHeap.Str());
        if (remap) {
            for (size_t i = 0; i < inputHeap.Size(); ++i) {
                TMemoryKeyIterator::THitIterator* it = inputHeap[i];
                if (!it->Next())
                    ythrow yexception() << "key has no hits";
                hitIterators.push_back(std::make_pair(remap[it->ArrIndex], it));
            }
            Sort(hitIterators.begin(), hitIterators.end());
            for (size_t j = 0; j < hitIterators.size(); ++j) {
                TMemoryKeyIterator::THitIterator* it = hitIterators[j].second;
                const ui32 oldDocID = TWordPosition::Doc(it->Current());
                do {
                    Y_ASSERT(TWordPosition::Doc(it->Current()) == oldDocID); // all hits must have the same docID
                    TWordPosition hit(it->Current());
                    hit.SetDoc(hitIterators[j].first);
                    output.StoreHit(hit.SuperLong());
                } while (it->Next());
            }
        } else {
            if (intersectedRanges) {
                keyHits.clear();
                for (size_t i = 0; i < inputHeap.Size(); ++i) {
                    TMemoryKeyIterator::THitIterator* it = inputHeap[i];
                    while (it->Next()) {
                        keyHits.push_back(it->Current());
                    }
                }
                Sort(keyHits.begin(), keyHits.end());
                for (const SUPERLONG& hit : keyHits) {
                    output.StoreHit(hit);
                }
            } else { /* ranges of docIds are not intersected in different keys */
                for (size_t i = 0; i < inputHeap.Size(); ++i) {
                    TMemoryKeyIterator::THitIterator* it = inputHeap[i];
                    if (it->Next())
                        hitIterators.push_back(std::make_pair(TWordPosition::Doc(it->Current()), it)); // ranges of docIDs must not have intersections
                    // if ranges have intersections then the assertions will fail in THitWriterImpl: HitCoder.GetCurrent() <= hit
                }
                Sort(hitIterators.begin(), hitIterators.end());
                for (size_t j = 0; j < hitIterators.size(); ++j) {
                    TMemoryKeyIterator::THitIterator* it = hitIterators[j].second;
                    do {
                        TWordPosition hit(it->Current());
                        output.StoreHit(hit.SuperLong());
                    } while (it->Next());
                }
            }
        }
        hitIterators.clear();
    }

    output.Flush();
    if (output.GetFormat() == IYndexStorage::FINAL_FORMAT)
        output.WriteVersion();
}

void MergeMemoryPortions(const TPortionBuffers* input, size_t count, IYndexStorage::FORMAT format,
    const ui32* remap, bool intersectedRanges, NIndexerCore::TMemoryPortion& output)
{
    MergeMemoryPortionsImpl(input, count, format, remap, intersectedRanges, output);
}

void MergeMemoryPortions(const NIndexerCore::TMemoryPortion** input, size_t count, IYndexStorage::FORMAT format,
    const ui32* remap, bool intersectedRanges, NIndexerCore::TMemoryPortion& output)
{
    MergeMemoryPortionsImpl(input, count, format, remap, intersectedRanges, output);
}

