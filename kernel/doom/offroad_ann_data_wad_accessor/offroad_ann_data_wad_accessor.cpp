#include "offroad_ann_data_wad_accessor.h"

#include <kernel/doom/offroad_ann_data_wad/offroad_ann_data_wad_io.h>

namespace NDoom {


template<class Io>
class TOffroadAnnDataWadIterator final: public NIndexAnn::IDocDataIterator {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;
public:
    TOffroadAnnDataWadIterator(const TSearcher* searcher, TIterator iterator, bool usePreloader)
        : Searcher_{ searcher }
        , Iterator_{ std::move(iterator) }
        , UsePreloader_{ usePreloader }
    {
        /* -1 is reserved for 'nothing', doh! */
        Mask_.SetDocId(-2);
    }

    TOffroadAnnDataWadIterator(const TSearcher* searcher, bool usePreloader)
        : TOffroadAnnDataWadIterator{ searcher, TIterator{}, usePreloader }
    {}

    void Restart(const NIndexAnn::THitMask& mask) override {
        Y_ASSERT(mask.IsNull() || mask.HasDoc()); /* Implemented only for the null mask, or a mask having a docId. */
        Y_ASSERT(Io::TPrefixVectorizer::TupleSize >= 2 || !mask.HasRegion()); /* Some IO's support region reading */
        Y_ASSERT(!mask.HasStream()); /* These are not supported, clients are supposed to read a whole break. */

        if (mask.IsNull()) {
            Mask_ = mask;
            FindNextDocIdWithHits(0);
            return;
        }

        if (mask.DocId() != Mask_.DocId())
            if (!Searcher_->Find(mask.DocId(), &Iterator_))
                Iterator_.Reset();

        Mask_ = mask;
        Current_.SetDocId(mask.DocId());

        if (Iterator_.LowerBound(mask, &Current_)) {
            Valid_ = Mask_.Matches(Current_);

            /* Don't advance if there is no match. This will make seeks into
             * the same position more efficient. */
            if (Valid_) {
                NIndexAnn::THit tmp;
                Iterator_.ReadHit(&tmp);
            }
        } else {
            Valid_ = false;
        }
    }

    bool Valid() const override {
        return Valid_;
    }

    const NIndexAnn::THit& Current() const override {
        return Current_;
    }

    const NIndexAnn::THit* Next() override {
        if (Mask_.IsNull()) {
            Valid_ = Iterator_.ReadHit(&Current_);

            if (!Valid_) {
                FindNextDocIdWithHits(Current_.DocId() + 1);
            }
        } else {
            Valid_ = Iterator_.ReadHit(&Current_) && Mask_.Matches(Current_);
        }

        return Valid_ ? &Current_ : nullptr;
    }

    void AnnounceDocIds(TConstArrayRef<ui32> docIds) override {
        if (UsePreloader_) {
            Searcher_->AnnounceDocIds(docIds, &Iterator_);
        }
    }

    void PreLoadDoc(ui32 docId, const NDoom::TSearchDocLoader& loader) override {
        if (UsePreloader_) {
            Searcher_->PreloadDoc(docId, &loader, &Iterator_);
        }
    }

    bool HasDoc(ui32 doc) override {
        return Searcher_->HasDoc(doc, &Iterator_);
    }

    THolder<NIndexAnn::IDocDataIterator> Clone(const NIndexAnn::IDocDataIndex* /* parent */) override {
        return MakeHolder<TOffroadAnnDataWadIterator<Io>>(Searcher_, TIterator{ Iterator_.GetData() }, UsePreloader_);
    }

private:
    void FindNextDocIdWithHits(ui32 nextDocId) {
        while (nextDocId < Searcher_->Size() && (
                !Searcher_->Find(nextDocId, &Iterator_) || !Iterator_.ReadHit(&Current_))) {
            ++nextDocId;
        }
        Valid_ = nextDocId < Searcher_->Size();
        Current_.SetDocId(nextDocId);
    }

    const TSearcher* Searcher_;
    TIterator Iterator_;
    bool UsePreloader_ = false;

    bool Valid_ = false;
    NIndexAnn::THitMask Mask_;
    NIndexAnn::THit Current_;
};

template <class Io>
class TOffroadAnnDataWadIndex : public NIndexAnn::IDocDataIndex {
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename TSearcher::TIterator;
public:
    TOffroadAnnDataWadIndex(THolder<NDoom::IWad>&& wad) {
        LocalWad_ = std::move(wad);
        Reset(LocalWad_.Get(), LocalWad_.Get());
    }

    TOffroadAnnDataWadIndex(const IWad* wad, const IDocLumpMapper* mapper) {
        Reset(wad, mapper);
    }

    THolder<NIndexAnn::IDocDataIterator> DoCreateIterator() const override {
        return MakeHolder<TOffroadAnnDataWadIterator<Io>>(&Searcher_, /* usePreloader= */ !LocalWad_);
    }

    bool HasDoc(ui32 doc, NIndexAnn::IDocDataIterator* iterator) const override {
        if (iterator) {
            return iterator->HasDoc(doc);
        }
        Y_ENSURE(LocalWad_, "Cannot use HasDoc without iterator in external wads mode");

        std::array<TArrayRef<const char>, 1> regions;
        Wad_->LoadDocLumps(doc, Mapping_, regions);
        return !regions[0].empty();
    }

    bool HasDoc(ui32 doc) const override {
        return HasDoc(doc, nullptr);
    }

private:
    void Reset(const IWad* wad, const IDocLumpMapper* mapper) {
        Wad_ = wad;
        Searcher_.Reset(wad, mapper);

        std::array<TWadLumpId, 1> lumps = {{TWadLumpId(Io::IndexType, EWadLumpRole::Hits)}};
        mapper->MapDocLumps(lumps, Mapping_);
    }

private:
    THolder<IWad> LocalWad_;
    const IWad* Wad_;
    TSearcher Searcher_;
    std::array<size_t, 1> Mapping_;
};

template <typename F, typename ...Args>
decltype(auto) DispatchAnnDataWadIndexType(EWadIndexType indexType, F&& callback, Args&& ...args) {
    switch (indexType) {
    case NDoom::FactorAnnDataIndexType:
        return std::invoke(std::forward<F>(callback), TOffroadFactorAnnDataDocWadIo{}, std::forward<Args>(args)...);
    case NDoom::LinkAnnDataIndexType:
        return std::invoke(std::forward<F>(callback), TOffroadLinkAnnDataDocWadIo{}, std::forward<Args>(args)...);
    case NDoom::AnnDataIndexType:
        return std::invoke(std::forward<F>(callback), TOffroadAnnDataDocWadIo{}, std::forward<Args>(args)...);
    case NDoom::FastAnnDataIndexType:
        return std::invoke(std::forward<F>(callback), TOffroadFastAnnDataDocWadIo{}, std::forward<Args>(args)...);
    default:
        Y_ENSURE(false, "Unsupported ann data wad index type");
    }
}

template <typename ...Args>
THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndexImpl(EWadIndexType indexType, const IWad* lumpsWad, Args&& ...args) {
    bool hasRequestedIndexType = AnyOf(lumpsWad->GlobalLumps(), [indexType](NDoom::TWadLumpId id) {
        return indexType == id.Index;
    });

    // TODO(sskvor): Remove after base in the new format
    if (!hasRequestedIndexType) {
        Y_ENSURE(indexType == NDoom::LinkAnnDataIndexType || indexType == NDoom::AnnDataIndexType);
        indexType = NDoom::FactorAnnDataIndexType;
    }

    return DispatchAnnDataWadIndexType(indexType, [](auto io, auto&& ...args) -> THolder<NIndexAnn::IDocDataIndex> {
        return MakeHolder<TOffroadAnnDataWadIndex<decltype(io)>>(std::forward<decltype(args)>(args)...);
    }, std::forward<Args>(args)...);
}

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(EWadIndexType indexType, const IWad* wad, const IDocLumpMapper* mapper) {
    return NewOffroadAnnDataWadIndexImpl(indexType, wad, wad, mapper);
}

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(EWadIndexType indexType, const TString& path, bool lockMemory) {
    THolder<NDoom::IWad> wad = NDoom::IWad::Open(path, lockMemory);
    NDoom::IWad* wadPtr = wad.Get();
    return NewOffroadAnnDataWadIndexImpl(indexType, wadPtr, std::move(wad));
}

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const IWad* wad) {
    return NewOffroadAnnDataWadIndex(wad, wad);
}

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const IWad* wad, const IDocLumpMapper* mapper) {
    return NewOffroadAnnDataWadIndex(NDoom::FactorAnnDataIndexType, wad, mapper);
}

THolder<NIndexAnn::IDocDataIndex> NewOffroadAnnDataWadIndex(const TString& path, bool lockMemory) {
    return NewOffroadAnnDataWadIndex(NDoom::FactorAnnDataIndexType, path, lockMemory);

}

} // namespace NDoom
