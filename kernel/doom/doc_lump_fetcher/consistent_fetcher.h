#pragma once

#include "doc_lump_fetcher.h"

#include <kernel/doom/wad/mapper.h>

namespace NDoom {

template<typename BaseLoader = IDocLumpLoader>
class TConsistentDocLumpLoader final : public IDocLumpLoader {
public:
    using Base = BaseLoader;

public:
    TConsistentDocLumpLoader() = default;

    TConsistentDocLumpLoader(BaseLoader loader, TConstArrayRef<size_t> lumpRemapping)
        : Loader_(std::move(loader))
        , LumpRemapping_(lumpRemapping)
    {
    }

    bool HasDocLump(size_t docLumpId) const override {
        return LumpRemapping_.size() > docLumpId && Loader_.HasDocLump(LumpRemapping_[docLumpId]);
    }

    void LoadDocRegions(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const final {
        for (size_t i = 0; i < mapping.size(); ++i) {
            if (mapping[i] < LumpRemapping_.size()) {
                Loader_.LoadDocRegions(
                    TConstArrayRef<size_t>{&LumpRemapping_[mapping[i]], 1},
                    TArrayRef<TConstArrayRef<char>>{&regions[i], 1});
            } else {
                regions[i] = {};
            }
        }
    }

    ui32 Chunk() const override { return Loader_.Chunk(); }

    TBlob DataHolder() const override {
        return Loader_.DataHolder();
    }

private:
    BaseLoader Loader_;
    TConstArrayRef<size_t> LumpRemapping_;
};

template<typename BaseMapper = IDocLumpMapper>
class TConsistentDocLumpMapper : public IDocLumpMapper {
public:
    TConsistentDocLumpMapper(const BaseMapper* base, THashMap<TWadLumpId, size_t>& provider)
        : Base_(base)
        , ConsistentMapping_(&provider)
    {
        TVector<TWadLumpId> docLumps = base->DocLumps();
        for (auto lumpId : docLumps) {
            if (!provider.contains(lumpId)) {
                size_t newId = provider.size();
                provider[lumpId] = newId;
            }
        }

        TVector<size_t> ids(docLumps.size());
        base->MapDocLumps(docLumps, ids);
        for (size_t i = 0; i < ids.size(); ++i) {
            size_t conistentId = provider[docLumps[i]];
            Remapping_.resize(Max<size_t>(Remapping_.size(), conistentId + 1), Max<size_t>());
            Remapping_[conistentId] = ids[i];
        }
    }

    TVector<TWadLumpId> DocLumps() const override {
        return Base_->DocLumps();
    }

    void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const final {
        for (size_t i = 0; i < ids.size(); ++i) {
            if (auto* mapped = ConsistentMapping_->FindPtr(ids[i])) {
                mapping[i] = *mapped;
            } else {
                mapping[i] = ConsistentMapping_->size();
            }
        }
    }

    template<typename Loader>
    TConsistentDocLumpLoader<Loader> GetLoader(Loader&& loader) const {
        return TConsistentDocLumpLoader<Loader>(std::forward<Loader>(loader), Remapping_);
    }

private:
    const BaseMapper* Base_;
    const THashMap<TWadLumpId, size_t>* ConsistentMapping_;
    TVector<size_t> Remapping_;
};

template<typename BaseLoader = IDocLumpLoader>
class TConsistentFetcher : public IDocLumpFetcher<TConsistentDocLumpLoader<BaseLoader>> {
public:
    TConsistentFetcher(THolder<IDocLumpFetcher<BaseLoader>> fetcher, THashMap<TWadLumpId, size_t>& provider)
        : Fetcher_(std::move(fetcher))
        , Mapper_(&Fetcher_->Mapper(), provider)
    {
    }

    TUnanswersStats Fetch(TConstArrayRef<ui32> ids, std::function<void(size_t, TConsistentDocLumpLoader<BaseLoader>*)> cb) const override {
        return Fetcher_->Fetch(ids, [&](size_t i, BaseLoader* loader) {
                if (loader) {
                    TConsistentDocLumpLoader<BaseLoader> consistentLoader = Mapper_.GetLoader(std::move(*loader));
                    cb(i, &consistentLoader);
                } else {
                    cb(i, nullptr);
                }
            });
    }

    const IDocLumpMapper& Mapper() const override {
        return Mapper_;
    }

private:
    THolder<IDocLumpFetcher<BaseLoader>> Fetcher_;
    TConsistentDocLumpMapper<> Mapper_;
};

} // namespace NDoom
