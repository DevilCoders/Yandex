#pragma once

#include "mapper.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/hash.h>

namespace NDoom {

TBlob MakeBlobHolder(TVector<TBlob> blobs, TConstArrayRef<char> region = {0, 0});

template<typename BaseLoader>
class TMultiDocLumpLoader final : public IDocLumpLoader {
public:
    TMultiDocLumpLoader() = default;

    TMultiDocLumpLoader(TVector<BaseLoader> loaders, TConstArrayRef<size_t> lumpLoaders, TConstArrayRef<size_t> lumpRemapping)
        : Loaders_(loaders)
        , LumpLoaders_(lumpLoaders)
        , LumpRemapping_(lumpRemapping)
    {
        ExtractChunk();
    }

    bool HasDocLump(size_t docLumpId) const override {
        return LumpLoaders_.size() > docLumpId && Loaders_[LumpLoaders_[docLumpId]].HasDocLump(LumpRemapping_[docLumpId]);
    }

    void LoadDocRegions(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const override {
        for (size_t i = 0; i < mapping.size(); ++i) {
            if (mapping[i] < LumpLoaders_.size()) {
                Loaders_[LumpLoaders_[mapping[i]]].LoadDocRegions(
                    TConstArrayRef<size_t>{&LumpRemapping_[mapping[i]], 1},
                    TArrayRef<TConstArrayRef<char>>{&regions[i], 1});
            } else {
                regions[i] = {};
            }
        }
    }

    TBlob DataHolder() const override {
        TVector<TBlob> result(Reserve(Loaders_.size()));
        for (size_t i = 0; i < Loaders_.size(); ++i) {
            result.push_back(Loaders_[i].DataHolder());
        }
        return MakeBlobHolder(result);
    }

    ui32 Chunk() const override {
        return Chunk_;
    }

    TArrayRef<BaseLoader> BaseLoaders() {
        return Loaders_;
    }

private:
    void ExtractChunk() {
        Y_ENSURE(!Loaders_.empty());

        Chunk_ = Loaders_.front().Chunk();
        for (const auto& loader : Loaders_) {
            Y_ENSURE(loader.Chunk() == Chunk_);
        }
    }

    TVector<BaseLoader> Loaders_;
    TConstArrayRef<size_t> LumpLoaders_;
    TConstArrayRef<size_t> LumpRemapping_;
    ui32 Chunk_ = 0;
};

template<typename BaseMapper = IDocLumpMapper>
class TMultiDocLumpMapper : public IDocLumpMapper {
public:
    TMultiDocLumpMapper() = default;

    TMultiDocLumpMapper(TConstArrayRef<const BaseMapper*> mappers) {
        Reset(mappers);
    }

    void Reset(TConstArrayRef<const BaseMapper*> mappers) {
        for (size_t i = 0; i < mappers.size(); ++i) {
            Mappers_.push_back(mappers[i]);
            TVector<TWadLumpId> lumps = mappers[i]->DocLumps();
            TVector<size_t> ids;
            ids.resize(lumps.size());
            mappers[i]->MapDocLumps(lumps, ids);

            for (size_t j = 0; j < lumps.size(); ++j) {
                Y_ENSURE(!LumpMapping_.contains(lumps[j]), "duplicate lump");
                auto size = LumpMapping_.size();
                LumpMapping_[lumps[j]] = size;
                LumpLoaders_.push_back(i);
                LumpRemapping_.push_back(ids[j]);
            }
        }
    }

    TVector<TWadLumpId> DocLumps() const override {
        TVector<TWadLumpId> result;
        for (auto [id, _] : LumpMapping_) {
            result.push_back(id);
        }
        return result;
    }

    void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        for (size_t i = 0; i < ids.size(); ++i) {
            if (auto* mapped = LumpMapping_.FindPtr(ids[i])) {
                mapping[i] = *mapped;
            } else {
                mapping[i] = LumpMapping_.size();
            }
        }
    }

    template<typename BaseLoader>
    TMultiDocLumpLoader<BaseLoader> GetLoader(TVector<BaseLoader> loaders) const {
        return TMultiDocLumpLoader<BaseLoader>(std::move(loaders), LumpLoaders_, LumpRemapping_);
    }

    TConstArrayRef<const BaseMapper*> Mappers() const {
        return Mappers_;
    }

private:
    THashMap<TWadLumpId, size_t> LumpMapping_;
    TVector<size_t> LumpLoaders_;
    TVector<size_t> LumpRemapping_;
    TVector<const BaseMapper*> Mappers_;
};

} // namespace NDoom
