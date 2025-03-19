#pragma once

#include "loc_resolver.h"

#include <kernel/doom/flat_blob_storage/flat_blob_storage.h>

namespace NDoom {

template <typename TResolver, typename FlatBlobStorage = IFlatBlobStorage>
class TErasureMegaWad
    : TMegaWadCommon
    , public IWad
{
public:
    TErasureMegaWad() = default;

    template<typename... Args>
    TErasureMegaWad(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset() {
        Global_.Reset();
        Resolver_.Reset();
    }

    template <typename... Args>
    void Reset(THolder<IWad> global, THolder<FlatBlobStorage> storage, Args&&... args) {
        Reset();
        Storage_ = std::move(storage);
        Global_ = std::move(global);
        Resolver_.Reset(Global_.Get(), std::forward<Args>(args)...);
        TMegaWadInfo info = LoadMegaWadInfo(Global_->LoadGlobalLump({EWadIndexType::ErasurePartLocations, EWadLumpRole::HitSub}));
        TMegaWadCommon::Reset(std::move(info));
    }

    ui32 Size() const override {
        return Info().DocCount;
    }

    TVector<TWadLumpId> GlobalLumps() const override {
        return Global_->GlobalLumps();
    }

    TVector<TString> GlobalLumpsNames() const override {
        return Global_->GlobalLumpsNames();
    }

    bool HasGlobalLump(TWadLumpId id) const override {
        return Global_->HasGlobalLump(id);
    }

    bool HasGlobalLump(TStringBuf id) const override {
        return Global_->HasGlobalLump(id);
    }

    TVector<TWadLumpId> DocLumps() const override {
        TVector<TWadLumpId> result(Reserve(Info().DocLumps.size()));
        for (TStringBuf lump : Info().DocLumps) {
            TWadLumpId value;
            if (TryFromString(lump, value)) {
                result.push_back(value);
            }
        }
        return result;
    }

    TVector<TStringBuf> DocLumpsNames() const override {
        return TVector<TStringBuf>{Info().DocLumps.begin(), Info().DocLumps.end()};
    }

    TBlob LoadGlobalLump(TWadLumpId id) const override {
        return Global_->LoadGlobalLump(id);
    }

    TBlob LoadGlobalLump(TStringBuf id) const override {
        return Global_->LoadGlobalLump(id);
    }

    void MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const override {
        TMegaWadCommon::MapDocLumps(ids, mapping);
    }

    void MapDocLumps(TConstArrayRef<TStringBuf> names, TArrayRef<size_t> mapping) const override {
        TMegaWadCommon::MapDocLumps(names, mapping);
    }

    void MapDocLumps(TConstArrayRef<TString> names, TArrayRef<size_t> mapping) const override {
        TMegaWadCommon::MapDocLumps(names, mapping);
    }

    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override {
        return LoadDocLumpsInternal(docId, mapping, regions);
    }

    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const override {
        return LoadDocLumpsInternal(docIds, mapping, regions);
    }

    void LoadDocLumps(
        const TArrayRef<const ui32>& docIds,
        const TArrayRef<const size_t>& mapping,
        std::function<void(size_t, TMaybe<TDocLumpData>&&)> callback) const override
    {
        TVector<ui32> parts(Reserve(docIds.size()));
        TVector<ui64> offsets(Reserve(docIds.size()));
        TVector<ui64> sizes(Reserve(docIds.size()));
        TVector<TBlob> result(Reserve(docIds.size()));
        TVector<ui32> blobMapping(Reserve(docIds.size()));
        TVector<TArrayRef<const char>> regions(mapping.size());
        for (size_t i = 0; i < docIds.size(); ++i) {
            if (auto loc = Resolver_.Resolve(docIds[i])) {
                parts.push_back(loc->Part);
                offsets.push_back(loc->Offset);
                sizes.push_back(loc->Size);
                result.push_back(TBlob());
                blobMapping.push_back(i);
            } else {
                callback(i, TDocLumpData{TBlob(), regions});
            }
        }
        Storage_->Read(parts, offsets, sizes,
            [&](size_t id, TMaybe<TBlob>&& blob) {
                if (blob) {
                    TMegaWadCommon::LoadDocLumpRegions(*blob, mapping, regions);
                    callback(blobMapping[id], TDocLumpData{std::move(*blob), regions});
                } else {
                    callback(blobMapping[id], Nothing());
                }
            });
    }

private:
    template<typename Region>
    TVector<TBlob> LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const {
        Y_ENSURE(docIds.size() == regions.size());
        TVector<ui32> parts(Reserve(docIds.size()));
        TVector<ui64> offsets(Reserve(docIds.size()));
        TVector<ui64> sizes(Reserve(docIds.size()));
        TVector<TBlob> result(Reserve(docIds.size()));
        TVector<bool> valid(docIds.size(), false);
        for (size_t i = 0; i < docIds.size(); ++i) {
            if (auto loc = Resolver_.Resolve(docIds[i])) {
                parts.push_back(loc->Part);
                offsets.push_back(loc->Offset);
                sizes.push_back(loc->Size);
                result.push_back(TBlob());
                valid[i] = true;
            }
        }

        Storage_->Read(parts, offsets, sizes, result);
        size_t validIds = parts.size();

        result.resize(docIds.size());
        for (ssize_t i = docIds.size() - 1; i >= 0; --i) {
            if (valid[i]) {
                validIds--;
                result[i] = std::move(result[validIds]);
                TMegaWadCommon::LoadDocLumpRegions(result[i], mapping, regions[i]);
            } else {
                result[i] = TBlob();
                for (size_t j = 0; j < regions[i].size(); ++j) {
                    regions[i][j] = Region();
                }
            }
        }
        return result;
    }

    template<typename Region>
    TBlob LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const {
        if (docId >= Size()) {
            for (Region& v: regions) {
                v = Region();
            }
            return TBlob();
        }
        TBlob docBlob = LoadDocBlob(docId);
        TMegaWadCommon::LoadDocLumpRegions(docBlob, mapping, regions);
        return docBlob;
    }

    TBlob LoadDocBlob(ui32 id) const {
        if (auto loc = Resolver_.Resolve(id)) {
            TBlob result;
            Storage_->Read({&loc->Part, 1}, {&loc->Offset, 1}, {&loc->Size, 1}, {&result, 1});
            return result;
        } else {
            return TBlob();
        }
    }

private:
    THolder<IWad> Global_;
    THolder<FlatBlobStorage> Storage_;
    TResolver Resolver_;
};

template <typename FlatBlobStorage = IFlatBlobStorage>
class TErasureMegaWadWithOptimizedParts : public TErasureMegaWad<TPartOptimizedErasureLocationResolver, FlatBlobStorage> {
public:
    using TBase = TErasureMegaWad<TPartOptimizedErasureLocationResolver, FlatBlobStorage>;

    template<typename... Args>
    TErasureMegaWadWithOptimizedParts(Args&&... args) {
        Reset(std::forward<Args>(args)...);
    }

    void Reset(const TString& offsetPath, THolder<FlatBlobStorage> storage, const TString& partPath) {
        Part_ = IWad::Open(partPath);
        THolder<IWad> global = IWad::Open(offsetPath);
        TBase::Reset(std::move(global), std::move(storage), 0, Part_.Get());
    }

private:
    THolder<IWad> Part_;
};

}
