#pragma once

#include <kernel/doom/wad/mega_wad_info.h>
#include <kernel/doom/wad/mega_wad_common.h>
#include <kernel/doom/wad/mapper.h>
#include <kernel/doom/erasure/wad_writer.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/offroad/streams/bit_input.h>

#include <util/generic/array_ref.h>
#include <util/memory/blob.h>

#include <array>

namespace NDoom {

class TChunkedWadDocLumpLoader final : public IDocLumpLoader {
public:
    TChunkedWadDocLumpLoader() = default;

    TChunkedWadDocLumpLoader(TBlob blob, ui32 chunk, TConstArrayRef<size_t> lumpRemapping, size_t regionsCount)
        : RegionsCount_(regionsCount)
        , LumpRemapping_(lumpRemapping)
        , Blob_(blob)
        , Chunk_(chunk)
    {}

    bool HasDocLump(size_t docLumpId) const override {
        return LumpRemapping_[docLumpId] < RegionsCount_;
    }

    void LoadDocRegions(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const override {
        if (mapping.empty()) {
            return;
        }
        if (Blob_.Empty()) {
            for (auto& region : regions) {
                region = {};
            }
            return;
        }
        Y_ENSURE(mapping.size() == regions.size());
        TStackVec<size_t> mapped;
        TStackVec<size_t> idRemapping;
        for (size_t i = 0; i < mapping.size(); ++i) {
            if (mapping[i] >= LumpRemapping_.size()) {
                regions[i] = {};
                continue;
            }
            size_t id = LumpRemapping_[mapping[i]];
            if (id < RegionsCount_) {
                mapped.push_back(id);
                idRemapping.push_back(i);
            }
        }
        auto regionsSlice = TArrayRef<TConstArrayRef<char>>(regions).Slice(0, idRemapping.size());
        TMegaWadCommon::CommonLoadDocLumpRegions(Blob_, RegionsCount_, mapped, regionsSlice);
        for (int i = static_cast<int>(idRemapping.size()) - 1; i >= 0; --i) {
            regions[idRemapping[i]] = regions[i];
        }
        size_t ptr = 0;
        for (size_t i = 0; i < mapping.size(); ++i) {
            while (ptr < idRemapping.size() && idRemapping[ptr] < i) {
                ++ptr;
            }
            if (ptr < idRemapping.size() && idRemapping[ptr] != i) {
                regions[i] = {};
            }
        }
    }

    TBlob DataHolder() const override {
        return Blob_;
    }

    ui32 Chunk() const override {
        return Chunk_;
    }

private:
    size_t RegionsCount_;
    TConstArrayRef<size_t> LumpRemapping_;
    TBlob Blob_;
    ui32 Chunk_;
};


class TChunkedWadDocLumpMapper : public IDocLumpMapper {
public:
    TChunkedWadDocLumpMapper() = default;

    TChunkedWadDocLumpMapper(TVector<NDoom::TMegaWadInfo> infos) {
        Reset(std::move(infos));
    }

    void Reset(TVector<NDoom::TMegaWadInfo> infos) {
        Remappings_ = TVector<TVector<size_t>>(infos.size());
        Infos_ = std::move(infos);

        for (size_t i = 0; i < Infos_.size(); ++i) {
            for (size_t j = 0; j < Infos_[i].DocLumps.size(); ++j) {
                if (!Mapping_.contains(Infos_[i].DocLumps[j])) {
                    size_t id = Mapping_.size();
                    Mapping_[Infos_[i].DocLumps[j]] = id;
                }
            }
        }
        for (size_t i = 0; i < Infos_.size(); ++i) {
            Remappings_[i].assign(Mapping_.size(), Mapping_.size());
            for (size_t j = 0; j < Infos_[i].DocLumps.size(); ++j) {
                size_t mapped = Mapping_.at(Infos_[i].DocLumps[j]);
                Remappings_[i].resize(Max(Remappings_[i].size(), mapped + 1));
                Remappings_[i][mapped] = j;
            }
        }
    }

    TChunkedWadDocLumpLoader GetLoader(TBlob blob, ui32 chunk) const {
        return TChunkedWadDocLumpLoader(blob, chunk, Remappings_[chunk], Infos_[chunk].DocLumps.size());
    }

    TConstArrayRef<size_t> GetChunkRemapping(ui32 chunk) const {
        return Remappings_[chunk];
    }

    size_t GetChunkDocLumpsCount(ui32 chunk) const {
        return Infos_[chunk].DocLumps.size();
    }

    void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        for (size_t i = 0; i < ids.size(); ++i) {
            if (auto* mapped = Mapping_.FindPtr(ToString(ids[i]))) {
                mapping[i] = *mapped;
            } else {
                mapping[i] = Mapping_.size();
            }
        }
    }

    void MapDocLumps(TConstArrayRef<TStringBuf> names, TArrayRef<size_t> mapping) const override {
        for (size_t i = 0; i < names.size(); ++i) {
            if (auto* mapped = Mapping_.FindPtr(names[i])) {
                mapping[i] = *mapped;
            } else {
                mapping[i] = Mapping_.size();
            }
        }
    }

    TVector<TWadLumpId> DocLumps() const override {
        TVector<TWadLumpId> result;
        for (auto [id, _] : Mapping_) {
            TWadLumpId value;
            if (TryFromString(id, value)) {
                result.push_back(value);
            }
        }
        return result;
    }

    TVector<TStringBuf> DocLumpsNames() const override {
        TVector<TStringBuf> result;
        for (auto&& [id, _] : Mapping_) {
            result.push_back(id);
        }
        return result;
    }

private:
    TVector<NDoom::TMegaWadInfo> Infos_;
    TVector<TVector<size_t>> Remappings_;
    THashMap<TString, size_t> Mapping_;
};


} // namespace NDoom
