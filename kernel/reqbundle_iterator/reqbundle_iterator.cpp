#include "factory.h"
#include "iterator.h"

#include "cache.h"
#include "iterator_and_impl.h"
#include "iterator_ordered_and_impl.h"
#include "iterator_offroad_impl.h"
#include "saveload.h"
#include "index_proc.h"
#include "bundle_proc.h"
#include "iterator_impl.h"
#include "reqbundle_iterator_builder.h"

#include <kernel/reqbundle/serializer.h>

#include <library/cpp/deprecated/iterators_heap/iterators_heap.h>

#include <search/base/blob_cache/lru_cache.h>

#include <util/generic/array_ref.h>

namespace {

    void ParseElem(
        TReqBundleDeserializer& deserializer,
        NReqBundle::TConstSequenceElemAcc elem,
        NReqBundleIterator::NImpl::TBlockBundleData*& blockCommonData,
        TMemoryPool& pool,
        const NReqBundleIterator::TGlobalOptions& globalOptions)
    {
        if (!blockCommonData) {
            if (elem.HasBlock()) {
                blockCommonData = pool.New<NReqBundleIterator::NImpl::TBlockBundleData>(elem.GetBlock(), pool, globalOptions);
            } else {
                NReqBundle::TBlock tempBlock;
                deserializer.ParseBinary(elem.GetBinary(), tempBlock); // Expensive (lz4)
                blockCommonData = pool.New<NReqBundleIterator::NImpl::TBlockBundleData>(tempBlock, pool, globalOptions);
            }
        }
    }

}

namespace NReqBundleIterator {
    //
    // TRBIterator::TImpl
    //

    class TRBIterator::TImpl {
    public:
        using TIteratorsHeap = TDeprecatedIteratorsHeap<
            NImpl::TIterator,
            THoldVector<NImpl::TIterator>>;

        TMemoryPool IteratorsMemory;
        NImpl::TIteratorPtrs Iterators;
        TIteratorsHeap IteratorsHeap;
        TVector<ui16>& RichTreeFormIds;
        TVector<ui32>& RichTreeBlockOffsets; // offsets in RichTreeFormIds
        TVector<ui32> BlockDecayCount;
        NImpl::TIteratorOptions Options;
        THolder<NImpl::TSharedIteratorData> SharedIteratorData;
        ui32 CurrentDocId = Max<ui32>();

    public:
        TImpl(
            TVector<ui16>& richTreeFormIds,
            TVector<ui32>& richTreeBlockOffsets)
            : IteratorsMemory(1 << 16)
            , RichTreeFormIds(richTreeFormIds)
            , RichTreeBlockOffsets(richTreeBlockOffsets)
        {}

        Y_FORCE_INLINE TPosition SkipBadTypes(TDynBitMap* goodBlocks) {
            if (!goodBlocks) {
                return IteratorsHeap.TopIter()->Current();
            }
            for (;;) {
                NImpl::TIterator* top = IteratorsHeap.TopIter();
                NReqBundleIterator::TPosition pos = top->Current();
                if (!pos.Valid()) {
                    return pos;
                }
                if (goodBlocks->Get(pos.BlockId())) { // TDynBitMap is fine with out-of-bounds read, returns false
                    return pos;
                }
                while (top->Current().Valid()) {
                    top->Next();
                }
                IteratorsHeap.SiftTopIter();
            }
        }

        void AnnounceDocIds(TConstArrayRef<ui32> docIds);
        void PreLoadDoc(ui32, const NDoom::TSearchDocLoader&);
        void AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer);

        void PrefetchDocs(const TVector<ui32>& docIds);

        void InitForDoc(ui32 docId);
        ui32 GetCurrentDoc() const {
            return CurrentDocId;
        }

        size_t GetDocumentPositions(
            NReqBundleIterator::TPosition* res,
            size_t bufSize,
            TDynBitMap* goodBlocks);

        bool LookupNextDoc(ui32& docId);
        bool LookupNextDoc(ui32& docId, ui32 maxDecay);

    private:
        bool CheckBlockContraints();
    };

    void TRBIterator::TImpl::AnnounceDocIds(TConstArrayRef<ui32> docIds) {
        SharedIteratorData->AnnounceDocIds(docIds);
    }

    void TRBIterator::TImpl::PrefetchDocs(const TVector<ui32>& docIds) {
        for (const auto& it : Iterators) {
            it->PrefetchDocs(docIds, *SharedIteratorData);
        }
    }

    void TRBIterator::TImpl::PreLoadDoc(ui32 id, const NDoom::TSearchDocLoader& loader) {
        SharedIteratorData->PreLoadDoc(id, loader);
    }

    void TRBIterator::TImpl::AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer) {
        SharedIteratorData->AdviseDocIds(docIds, std::move(consumer));
    }

    // join iterators over individual blocks
    void TRBIterator::TImpl::InitForDoc(ui32 docId) {
        CurrentDocId = docId;
        if (Iterators.empty())
            return;
        SharedIteratorData->HitsStorage.Reset();
        SharedIteratorData->SharedSentenceLengths.Reset();
        SharedIteratorData->BrkBuffer.Clear();
        for (const auto& it : Iterators) {
            it->InitForDoc(docId, true, Options, *SharedIteratorData);
        }
        for (const auto& it : Iterators) {
            it->InitForDoc(docId, false, Options, *SharedIteratorData);
        }
        IteratorsHeap.Restart();
    }

    size_t TRBIterator::TImpl::GetDocumentPositions(
        TPosition* res,
        size_t bufSize,
        TDynBitMap* goodBlocks)
    {
        if (!bufSize) {
            return 0;
        }
        if (Iterators.empty()) {
            return 0;
        }
        size_t bufPos = 0;
        TPosition cur = SkipBadTypes(goodBlocks);
        if (!cur.Valid()) {
            return bufPos;
        }
        for (;;) {
            IteratorsHeap.Next();
            TPosition next = SkipBadTypes(goodBlocks);
            if (!cur.TBlockId::EqualUpper(next)) {
                res[bufPos] = cur;
                ++bufPos;
                cur = next;
                if (!next.Valid()) {
                    break;
                }
                if (bufPos >= bufSize) {
                    break;
                }
            } else {
                cur = Min(cur, next);
            }
        }
        return bufPos;
    }

    bool TRBIterator::TImpl::LookupNextDoc(ui32& docId) {
        docId = Max<ui32>();
        for (const NImpl::TIteratorPtr& iter : Iterators) {
            iter->LookupNextDoc(docId);
        }

        return (docId != Max<ui32>());
    }

    bool TRBIterator::TImpl::LookupNextDoc(ui32& docId, ui32 maxDecay) {
        NImpl::TIteratorDecayInfo info;
        for (const NImpl::TIteratorPtr& iter : Iterators) {
            if (BlockDecayCount[iter->BlockId] < maxDecay) {
                iter->LookupNextDoc(info);
            }
        }
        if (info.MinDocId != Max<ui32>()) {
            BlockDecayCount[info.Iterator->BlockId]++;
            docId = info.MinDocId;
            return true;
        } else {
            return false;
        }
    }

    //
    // TRBIterator
    //

    TRBIterator::TRBIterator()
        : Impl(MakeHolder<TImpl>(RichTreeFormIds, RichTreeBlockOffsets))
    {}

    TRBIterator::~TRBIterator() {
    }

    TRBIterator::TImpl& TRBIterator::GetImpl() {
        return *Impl;
    }

    void TRBIterator::AnnounceDocIds(TConstArrayRef<ui32> docIds) {
        Impl->AnnounceDocIds(docIds);
    }

    void TRBIterator::PrefetchDocs(const TVector<ui32>& docIds) {
        Impl->PrefetchDocs(docIds);
    }

    void TRBIterator::PreLoadDoc(ui32 id, const NDoom::TSearchDocLoader& loader) {
        Impl->PreLoadDoc(id, loader);
    }

    void TRBIterator::AdviseDocIds(TConstArrayRef<ui32> docIds, std::function<void(ui32)> consumer) {
        Impl->AdviseDocIds(docIds, std::move(consumer));
    }

    void TRBIterator::InitForDoc(ui32 docId) {
        Impl->InitForDoc(docId);
    }

    ui32 TRBIterator::GetCurrentDoc() const {
        return Impl->GetCurrentDoc();
    }

    size_t TRBIterator::GetDocumentPositionsPartial(
        TPosition* res,
        size_t bufSize,
        TDynBitMap* goodBlocks)
    {
        return Impl->GetDocumentPositions(res, bufSize, goodBlocks);
    }

    bool TRBIterator::LookupNextDoc(ui32& docId) {
        return Impl->LookupNextDoc(docId);
    }

    bool TRBIterator::LookupNextDoc(ui32& docId, ui32 maxDecay) {
        return Impl->LookupNextDoc(docId, maxDecay);
    }

    //
    // TRBIteratorsFactory::TImpl
    //

    class TRBIteratorsFactory::TImpl {
    private:


        TMaybe<TMemoryPool> IntPool; // constructed only if external mem pool was not passed
        TMemoryPool* Pool = nullptr;

        const TConstReqBundleAcc ReqBundle;
        size_t NumBlocks = 0;

        const TGlobalOptions GlobalOptions;

        TArrayRef<NImpl::TBlockBundleData*> ParsedBlocks;
        TArrayRef<TBlockType> BlockTypes;

    public:
        TImpl(
            TConstReqBundleAcc reqBundle,
            const TGlobalOptions& globalOptions,
            TMemoryPool* pool);
        ~TImpl();

        // should be set before OpenIterator(); default value is DefaultBlockType
        TBlockType& BlockType(size_t blockIdx) {
            Y_ASSERT(blockIdx < NumBlocks);
            return BlockTypes[blockIdx];
        }

        TRBIteratorPtr OpenIterator(
            IRBIteratorBuilderGlobal& builder,
            IRBSharedDataFactory* sharedDataFactory,
            IRBIndexIteratorLoader* iteratorLoader,
            IRBIteratorsHasher* hasher,
            const TRBIteratorOptions& options,
            bool twoPhaseMode = false,
            TRBIteratorOpeningInfo* info = nullptr);
    };

    TRBIteratorsFactory::TImpl::TImpl(
        TConstReqBundleAcc reqBundle,
        const TGlobalOptions& globalOptions,
        TMemoryPool* const pool)
        : ReqBundle(reqBundle)
        , GlobalOptions(globalOptions)
    {
        if (pool) {
            Pool = pool;
        } else {
            IntPool.ConstructInPlace(1 << 16);
            Pool = IntPool.Get();
        }
        Y_ASSERT(Pool);

        const auto seq = ReqBundle.GetSequence();
        NumBlocks = seq.GetNumElems();
        Y_ENSURE(NumBlocks <= TBlockId::MaxValue, "too many blocks in reqbundle");

        ParsedBlocks = {Pool->AllocateArray<NImpl::TBlockBundleData*>(NumBlocks), NumBlocks};
        Fill(ParsedBlocks.begin(), ParsedBlocks.end(), nullptr);

        BlockTypes = {Pool->AllocateArray<TBlockType>(NumBlocks), NumBlocks};
        Fill(BlockTypes.begin(), BlockTypes.end(), DefaultBlockType);
    }

    TRBIteratorsFactory::TImpl::~TImpl()
    {
        for (const size_t i : xrange(NumBlocks)) {
            if (ParsedBlocks[i]) {
                ParsedBlocks[i]->~TBlockBundleData();
            }
        }
    }

    TRBIteratorPtr TRBIteratorsFactory::TImpl::OpenIterator(
        IRBIteratorBuilderGlobal& builder,
        IRBSharedDataFactory* sharedDataFactory,
        IRBIndexIteratorLoader* iteratorLoader,
        IRBIteratorsHasher* hasher,
        const TRBIteratorOptions& options,
        bool twoPhaseMode,
        TRBIteratorOpeningInfo* info)
    {
        const auto seq = ReqBundle.GetSequence();
        Y_ASSERT(NumBlocks == seq.GetNumElems());

        THolder<TRBIterator> result = MakeHolder<TRBIterator>();
        TRBIterator::TImpl& internals = result->GetImpl();

        internals.RichTreeBlockOffsets.resize(NumBlocks, ~0u);

        internals.Options.FetchLimit = options.IteratorsFetchLimit;
        internals.Options.FirstStageFetchLimit = options.IteratorsFirstStageFetchLimit;

        if (sharedDataFactory && !twoPhaseMode) {
            internals.SharedIteratorData = sharedDataFactory->MakeSharedData();
        }

        TBuffer blobBuffer;

        TVector<NImpl::TBlockIndexData, TPoolAllocator> loadedBlocks(Pool);
        TVector<TBlob, TPoolAllocator> loadedBlobs(Pool);
        loadedBlocks.reserve(NumBlocks);
        loadedBlobs.resize(NumBlocks);

        NImpl::IIndexAccessor& indexAccessor = builder.GetIndexAccessor();

        TReqBundleDeserializer deserializer;
        NImpl::TIndexLookupMapping allKeys;
        size_t numCached = 0;
        for (size_t blockIndex = 0; blockIndex < NumBlocks; blockIndex++) {
            loadedBlocks.emplace_back(*Pool);

            const auto elem = seq.GetElem(blockIndex);
            if (elem.HasBinary()) {
                auto binary = NReqBundle::NDetail::BackdoorAccess(elem.GetBinary());
                if (hasher && hasher->LoadOne(binary, loadedBlobs[blockIndex])) {
                    hasher->StoreOne(binary, loadedBlobs[blockIndex]); // Update freshness for blob
                    loadedBlocks.back().NeedFetch = false;
                    ++numCached;
                } else {
                    if (hasher)
                        loadedBlocks.back().NeedSerialize = true;
                    ParseElem(deserializer, elem, ParsedBlocks[blockIndex], *Pool, GlobalOptions);
                }
            } else if (Y_LIKELY(elem.HasBlock())) {
                ParseElem(deserializer, elem, ParsedBlocks[blockIndex], *Pool, GlobalOptions);
            } else {
                ythrow yexception() << "unknown block format in req bundle: " << blockIndex;
            }
            if (loadedBlocks.back().NeedFetch)
                loadedBlocks.back().CollectIndexKeys(*ParsedBlocks[blockIndex], *Pool, builder.Utf8IndexKeys(), builder.GetKeyPrefixes(), allKeys);
        }

        if (info) {
            info->NumBlocks = NumBlocks;
            info->NumBlocksCached = numCached;
        }

        // Fetch hits
        for (auto& [key, lemmas] : allKeys) {
            indexAccessor.PrepareHits(key.Key, lemmas, key.KeyPrefix, options);
        }

        for (size_t blockIndex = 0; blockIndex < NumBlocks; blockIndex++) {
            if (!loadedBlocks[blockIndex].NeedFetch) {
                TBlob& savedBlob = loadedBlobs[blockIndex];
                TMemoryInput in(savedBlob.AsCharPtr(), savedBlob.Size());
                NImpl::TIteratorSaveLoad::LoadBlockIterators(
                    &in,
                    blockIndex,
                    internals.Iterators,
                    internals.IteratorsMemory,
                    *iteratorLoader);
                // Need to push loaded forms and iteratos before jumping to next iteration
                internals.RichTreeBlockOffsets[blockIndex] = internals.RichTreeFormIds.size();
                NImpl::TIteratorSaveLoad::LoadFormIds(&in, internals.RichTreeFormIds);
                continue;
            }

            internals.RichTreeBlockOffsets[blockIndex] = internals.RichTreeFormIds.size();
            size_t numIteratorsBefore = internals.Iterators.size();

            if (loadedBlocks[blockIndex].NumWords == 0) {
                continue;
            }

            Y_ASSERT(ParsedBlocks[blockIndex]);

            if (loadedBlocks[blockIndex].NumWords == 1 && (!options.EnableOrderedAnd || ParsedBlocks[blockIndex]->Type != NReqBundle::NDetail::EBlockType::ExactOrdered)) {
                loadedBlocks[blockIndex].Words[0].MakeIterators(
                    internals.Iterators,
                    &internals.RichTreeFormIds,
                    blockIndex,
                    internals.IteratorsMemory);
            } else {
                THolder<NImpl::TBasicIteratorAnd, TDestructor> iter;
                if (options.EnableOrderedAnd && ParsedBlocks[blockIndex]->Type == NReqBundle::NDetail::EBlockType::ExactOrdered) {
                    iter = NImpl::MakeIteratorOrderedAnd(
                        loadedBlocks[blockIndex].NumWords,
                        blockIndex,
                        internals.IteratorsMemory,
                        ParsedBlocks[blockIndex],
                        loadedBlocks[blockIndex]);
                } else {
                    iter = NImpl::MakeIteratorAnd(
                        loadedBlocks[blockIndex].NumWords,
                        blockIndex,
                        internals.IteratorsMemory,
                        ParsedBlocks[blockIndex],
                        loadedBlocks[blockIndex]);
                }

                if (iter && iter->HasData()) {
                    internals.RichTreeFormIds.push_back(0); // LowLevelFormId is always zero, RichTreeFormId is always zero too
                    internals.Iterators.push_back(std::move(iter));
                }
            }
            if (!loadedBlocks[blockIndex].NeedSerialize)
                continue;

            const NReqBundle::TBinaryBlock& binary = NReqBundle::NDetail::BackdoorAccess(seq.GetBinary(blockIndex));

            if (hasher) {
                blobBuffer.Clear();
                {
                    TBufferOutput out(blobBuffer);

                    NImpl::TIteratorSaveLoad::SaveBlockIterators(&out,
                        internals.Iterators.begin() + numIteratorsBefore,
                        internals.Iterators.end());
                    NImpl::TIteratorSaveLoad::SaveFormIds(&out,
                        internals.RichTreeFormIds.begin() + internals.RichTreeBlockOffsets[blockIndex],
                        internals.RichTreeFormIds.end());
                }
                TBlob blobToSave = TBlob::NoCopy(blobBuffer.data(), blobBuffer.size());

                hasher->StoreOne(binary, blobToSave); // Ok, if it fails
            }
        }

        if (!internals.Iterators.empty()) {
            if (builder.NeedIteratorSorting())
                SortBy((NImpl::TIterator**)internals.Iterators.begin(), (NImpl::TIterator**)internals.Iterators.end(),
                    [](const NImpl::TIterator* it) {return it->GetSortOrder();});
            if (!twoPhaseMode) {
                internals.IteratorsHeap.Restart((NImpl::TIterator**)internals.Iterators.begin(), internals.Iterators.size());
            }
        }

        for (const auto& iter : internals.Iterators) {
            Y_ASSERT(BlockTypes[iter->BlockId] < 32); // we use ui32 for mask in GetDocumentPositionsPartial
            iter->BlockType = BlockTypes[iter->BlockId];
        }

        if (!twoPhaseMode) {
            internals.BlockDecayCount.assign(NumBlocks, 0);
        }

        return result;
    }

    //
    // TRBIteratorsFactory
    //

    TRBIteratorsFactory::TRBIteratorsFactory(
        TConstReqBundleAcc reqBundle,
        const TGlobalOptions& globalOptions,
        TMemoryPool* pool)
        : Impl(MakeHolder<TImpl>(reqBundle, globalOptions, pool))
    {}

    TRBIteratorsFactory::~TRBIteratorsFactory() {
    }

    TBlockType& TRBIteratorsFactory::BlockType(size_t blockIdx) {
        return Impl->BlockType(blockIdx);
    }

    TRBIteratorPtr TRBIteratorsFactory::OpenIterator(
        IRBIteratorBuilder& builder,
        IRBIteratorsHasher* hasher,
        const TRBIteratorOptions& options)
    {
        return Impl->OpenIterator(
            builder,
            &builder,
            &builder,
            hasher,
            options,
            false);
    }

    TString TRBIteratorsFactory::PreOpenIterator(
        IRBIteratorBuilderGlobal& builder,
        IRBIteratorsHasher* hasher,
        const TRBIteratorOptions& options,
        TRBIteratorOpeningInfo* info)
    {
        TRBIteratorPtr iter = Impl->OpenIterator(builder, nullptr, &builder, hasher, options, true, info);
        NReqBundleIteratorProto::TRbIterator proto;

        TRBIterator::TImpl& internals = iter->GetImpl();
        for (const auto& internalIterator: internals.Iterators) {
            internalIterator->SerializeToProto(proto.AddIterators());
        }

        for (ui16 formId : internals.RichTreeFormIds) {
            proto.AddRichTreeFormIds(formId);
        }
        for (ui32 blockOffset : internals.RichTreeBlockOffsets) {
            proto.AddRichTreeBlockOffsets(blockOffset);
        }

        return proto.SerializeAsString();
    }

    TRBIteratorPtr TRBIteratorsFactory::OpenIterator(
        TStringBuf intermediateRepresentation,
        IRBSharedDataFactory& sharedDataFactory,
        IRBIteratorDeserializer& deserializer,
        const TRBIteratorOptions& options)
    {
        Y_UNUSED(intermediateRepresentation, sharedDataFactory);
        NReqBundleIteratorProto::TRbIterator proto;
        Y_ENSURE(proto.ParseFromArray(intermediateRepresentation.data(), intermediateRepresentation.size()));

        THolder<TRBIterator> result = MakeHolder<TRBIterator>();
        TRBIterator::TImpl& internals = result->GetImpl();

        internals.Options.FetchLimit = options.IteratorsFetchLimit;
        internals.Options.FirstStageFetchLimit = options.IteratorsFirstStageFetchLimit;

        internals.SharedIteratorData = sharedDataFactory.MakeSharedData();

        internals.RichTreeFormIds.assign(proto.GetRichTreeFormIds().begin(), proto.GetRichTreeFormIds().end());
        internals.RichTreeBlockOffsets.assign(proto.GetRichTreeBlockOffsets().begin(), proto.GetRichTreeBlockOffsets().end());

        for (const auto& iterator : proto.GetIterators()) {
            internals.Iterators.push_back(deserializer.DeserializeFromProto(iterator, internals.IteratorsMemory));
        }

        if (!internals.Iterators.empty()) {
            internals.IteratorsHeap.Restart((NImpl::TIterator**)internals.Iterators.begin(), internals.Iterators.size());
        }

        const ui32 numBlocks = internals.RichTreeBlockOffsets.size();
        internals.BlockDecayCount.assign(numBlocks, 0);

        return result;
    }

    TRBIteratorPtr TRBIteratorsFactory::OpenIterator(
        IRBSharedDataFactory& sharedDataFactory,
        const TRBIteratorOptions& options)
    {
        THolder<TRBIterator> result = MakeHolder<TRBIterator>();
        TRBIterator::TImpl& internals = result->GetImpl();

        internals.Options.FetchLimit = options.IteratorsFetchLimit;
        internals.Options.FirstStageFetchLimit = options.IteratorsFirstStageFetchLimit;

        internals.SharedIteratorData = sharedDataFactory.MakeSharedData();

        return result;
    }

    void TRBIteratorsFactory::DeserializeAndAppend(
        TStringBuf intermediateRepresentation,
        IRBIteratorDeserializer& deserializer,
        TRBIteratorPtr& rbIteratorPtr)
    {
        NReqBundleIteratorProto::TRbIterator proto;
        Y_ENSURE(proto.ParseFromArray(intermediateRepresentation.data(), intermediateRepresentation.size()));

        TRBIterator::TImpl& internals = rbIteratorPtr->GetImpl();

        const ui32 formIdOffset = internals.RichTreeFormIds.size();
        internals.RichTreeFormIds.insert(internals.RichTreeFormIds.end(), proto.GetRichTreeFormIds().begin(), proto.GetRichTreeFormIds().end());

        const ui32 blocksOffset = internals.RichTreeBlockOffsets.size();
        for (const auto& treeBlockOffset : proto.GetRichTreeBlockOffsets()) {
            internals.RichTreeBlockOffsets.emplace_back(treeBlockOffset + formIdOffset);
        }

        for (const auto& iterator : proto.GetIterators()) {
            internals.Iterators.push_back(deserializer.DeserializeFromProto(iterator, internals.IteratorsMemory, blocksOffset));
        }
    }

    bool TRBIteratorsFactory::PostAppendProcess(
        TRBIteratorPtr& rbIteratorPtr,
        const TRBIteratorOptions& /*options*/,
        TRBIteratorsFactory* blockTypes)
    {
        TRBIterator::TImpl& internals = rbIteratorPtr->GetImpl();
        if (internals.Iterators.empty()) {
            return false;
        }
        if (blockTypes) {
            for (auto it = internals.Iterators.begin(); it != internals.Iterators.end();) {
                auto* iterElem = it->Get();
                iterElem->BlockType = blockTypes->BlockType(iterElem->BlockId);
                it++;
            }
        }
        SortBy((NImpl::TIterator**)internals.Iterators.begin(), (NImpl::TIterator**)internals.Iterators.end(),
            [](const NImpl::TIterator* it) {return it->GetSortOrder();});
        internals.IteratorsHeap.Restart((NImpl::TIterator**)internals.Iterators.begin(), internals.Iterators.size());
        const ui32 numBlocks = internals.RichTreeBlockOffsets.size();
        internals.BlockDecayCount.assign(numBlocks, 0);
        return true;
    }

} // NReqBundleIterator
