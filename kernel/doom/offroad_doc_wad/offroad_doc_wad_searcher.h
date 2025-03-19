#pragma once

#include <kernel/doom/chunked_wad/chunked_wad.h>
#include <kernel/doom/chunked_wad/single_chunked_wad.h>

#include "offroad_doc_wad_iterator.h"
#include "offroad_doc_wad_mapping.h"
#include "offroad_doc_codec.h"

namespace NDoom {


template<EWadIndexType indexType, class Hit, class Vectorizer, class Subtractor, class PrefixVectorizer, EOffroadDocCodec codec = AdaptiveDocCodec>
class TOffroadDocWadSearcher {
    using TMapping = TOffroadDocWadMapping<indexType, PrefixVectorizer>;
public:
    using TIterator = TOffroadDocWadIterator<Hit, Vectorizer, Subtractor, PrefixVectorizer, codec>;
    using THit = typename TIterator::THit;

    using TTable = typename TIterator::TReader::TTable;
    using TModel = typename TIterator::TReader::TModel;

    TOffroadDocWadSearcher() {}

    template <typename...Args>
    TOffroadDocWadSearcher(Args&&...args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TTable* table, const IWad* wad) {
        WrapWad_.Reset(new TSingleChunkedWad(wad));
        if (table)
            Reset(MakeArrayRef(&table, 1), WrapWad_.Get(), WrapWad_.Get());
        else
            Reset(TArrayRef<const TTable*>(), WrapWad_.Get(), WrapWad_.Get());
    }

    void Reset(const IWad* wad) {
        WrapWad_.Reset(new TSingleChunkedWad(wad));
        Reset(WrapWad_.Get());
    }

    void Reset(const IChunkedWad* wad) {
        Reset(TArrayRef<const TTable*>(), wad, wad);
    }

    void Reset(const IWad* wad, const NDoom::IDocLumpMapper* mapper) {
        WrapWad_.Reset(new TSingleChunkedWad(wad));
        Reset(WrapWad_.Get(), mapper);
    }

    void Reset(const IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper) {
        Reset(TArrayRef<const TTable*>(), wad, mapper);
    }

    void Reset(TArrayRef<const TTable*> tables, const IChunkedWad* wad, const NDoom::IDocLumpMapper* mapper) {
        Y_VERIFY(tables.empty() || tables.size() == wad->Chunks(), "Size of passed tables and number of wad chunks are not the same");
        Tables_.assign(tables.begin(), tables.end());

        Wad_ = wad;
        mapper->MapDocLumps(Mapping_.DocLumps(), DocLumpMapping_);

        if (Tables_.empty()) {
            Tables_.resize(Wad_->Chunks(), nullptr);
            LocalTables_.resize(Wad_->Chunks());
            TModel model;
            for (ui32 chunkId = 0; chunkId < Wad_->Chunks(); chunkId++) {
                model.Load(Wad_->LoadChunkGlobalLump(chunkId, Mapping_.ModelLump()));
                LocalTables_[chunkId].Reset(model);
                Tables_[chunkId] = &LocalTables_[chunkId];
            }
        }
    }

    size_t Size() const {
        return Wad_->Size();
    }

    void AnnounceDocIds(TConstArrayRef<ui32> docIds, TIterator* iterator) const {
        iterator->PrefetchedDocIds() = TVector<ui32>(docIds.begin(), docIds.end());
        Sort(iterator->PrefetchedDocIds());
        iterator->PrefetchedDocBlobs().resize(docIds.size());
        iterator->PrefetchedDocRegions().resize(docIds.size());
    }

    void ClearPreloadedDocs(TIterator* iterator) const {
        iterator->PrefetchedDocBlobs().clear();
        iterator->PrefetchedDocRegions().clear();
        iterator->PrefetchedDocIds().clear();
    }

    template<typename Loader>
    void PreloadDoc(ui32 docId, Loader* loader, TIterator* iterator) const {
        const size_t index = GetPrefetchedDocPosition(docId, iterator);
        Y_ENSURE(index < iterator->PrefetchedDocIds().size() && iterator->PrefetchedDocIds()[index] == docId);
        loader->LoadDocRegions(
            DocLumpMapping_,
            iterator->PrefetchedDocRegions()[index]);
        iterator->PrefetchedDocBlobs()[index] = loader->DataHolder();
    }

    bool Find(ui32 docId, TIterator* iterator) const {
        std::array<TArrayRef<const char>, TMapping::DocLumpCount> regions;
        if (!iterator->PrefetchedDocIds().empty()) {
            const size_t index = GetPrefetchedDocPosition(docId, iterator);
            if (index != iterator->PrefetchedDocIds().size() && iterator->PrefetchedDocIds()[index] == docId) {
                iterator->Blob() = iterator->PrefetchedDocBlobs()[index];
                regions = iterator->PrefetchedDocRegions()[index];
            }
        } else {
            TBlob blob = Wad_->LoadDocLumps(docId, DocLumpMapping_, regions);
            if (regions[0].empty())
                return false;
            iterator->Blob().Swap(blob);
        }
        const size_t chunkId = Wad_->DocChunk(docId);
        Mapping_.ResetStream(&iterator->Reader_, Tables_[chunkId], regions);
        return true;
    }

    void PrefetchDocs(const TArrayRef<ui32>& docIds, TIterator* iterator) const {
        Y_ENSURE(IsSorted(docIds.begin(), docIds.end()));

        iterator->PrefetchedDocIds().assign(docIds.begin(), docIds.end());
        iterator->PrefetchedDocRegions().resize(docIds.size());
        if (docIds.size() <= 16) {
            std::array<TArrayRef<TArrayRef<const char>>, 16> view;
            for (size_t i = 0; i < docIds.size(); ++i) {
                view[i] = TArrayRef<TArrayRef<const char>>(iterator->PrefetchedDocRegions()[i]);
            }
            iterator->PrefetchedDocBlobs() = Wad_->LoadDocLumps(
                iterator->PrefetchedDocIds(),
                DocLumpMapping_,
                TArrayRef<TArrayRef<TArrayRef<const char>>>(view.data(), docIds.size()));
        } else {
            TVector<TArrayRef<TArrayRef<const char>>> view(docIds.size());
            for (size_t i = 0; i < docIds.size(); ++i) {
                view[i] = TArrayRef<TArrayRef<const char>>(iterator->PrefetchedDocRegions()[i]);
            }

            iterator->PrefetchedDocBlobs() = Wad_->LoadDocLumps(
                iterator->PrefetchedDocIds(),
                DocLumpMapping_,
                view);
        }
    }

    TVector<bool> Find(const TArrayRef<ui32>& docIds, TArrayRef<TIterator> iterators) const {
        TVector<bool> found(docIds.size());
        TIterator iterator;
        PrefetchDocs(docIds, &iterator);
        Y_ASSERT(iterators.size() == docIds.size());
        Y_ASSERT(iterator.PrefetchedDocIds().size() == docIds.size());
        for (size_t i = 0; i < iterator.PrefetchedDocIds().size(); ++i) {
            TIterator iter;
            if (Find(iterator.PrefetchedDocIds()[i], &iterator)) {
                iter.Blob().Swap(iterator.Blob());
                iter.Reader_.Swap(iterator.Reader_);
                iterators[i].Swap(iter);
                found[i] = true;
            }
        }
        return found;
    }

    bool HasDoc(ui32 docId, TIterator* iterator) const {
        std::array<TArrayRef<const char>, TMapping::DocLumpCount> regions;
        if (!iterator->PrefetchedDocIds().empty()) {
            const size_t index = GetPrefetchedDocPosition(docId, iterator);
            if (index != iterator->PrefetchedDocIds().size() && iterator->PrefetchedDocIds()[index] == docId) {
                iterator->Blob() = iterator->PrefetchedDocBlobs()[index];
                regions = iterator->PrefetchedDocRegions()[index];
            }
        } else {
            Wad_->LoadDocLumps(docId, DocLumpMapping_, regions);
        }
        return !regions[0].empty();
    }

private:
    size_t GetPrefetchedDocPosition(ui32 docId, TIterator* iterator) const {
        return LowerBound(iterator->PrefetchedDocIds().begin(), iterator->PrefetchedDocIds().end(), docId) - iterator->PrefetchedDocIds().begin();
    }

private:
    TMapping Mapping_;
    TVector<TTable> LocalTables_;
    TVector<const TTable*> Tables_;
    const IChunkedWad* Wad_ = nullptr;
    THolder<IChunkedWad> WrapWad_;
    std::array<size_t, TMapping::DocLumpCount> DocLumpMapping_;
};


} // namespace NDoom
