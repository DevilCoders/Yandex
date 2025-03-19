#include "wad_router.h"

namespace NDoom {

TWadRouter::TWadRouter() {
}

TWadRouter::~TWadRouter() {
}

// WAD interface:
ui32 TWadRouter::Size() const {
    Y_ASSERT(Const_);

    return Size_;
}

TVector<TWadLumpId> TWadRouter::GlobalLumps() const {
    Y_ASSERT(Const_);

    TVector<TWadLumpId> res;
    for (auto&& lumpInfo : GlobalLumps_)
        res.push_back(lumpInfo.first);
    return res;
}

bool TWadRouter::HasGlobalLump(TWadLumpId id) const {
    Y_ASSERT(Const_);

    return GlobalLumps_.contains(id);
}

TVector<TWadLumpId> TWadRouter::DocLumps() const {
    Y_ASSERT(Const_);

    TVector<TWadLumpId> res;
    res.reserve(DocLumpMapping_.size());
    for (auto&& id : DocLumpMapping_)
        res.push_back(id.first);

    return res;
}

TBlob TWadRouter::LoadGlobalLump(TWadLumpId id) const {
    Y_ASSERT(Const_);
    Y_ASSERT(HasGlobalLump(id));

    return GlobalLumps_.at(id)->LoadGlobalLump(id);
}

void TWadRouter::MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const {
    Y_ASSERT(Const_);
    Y_ASSERT(ids.size() == mapping.size());
    Y_ENSURE(ids.size() == mapping.size());

    for (size_t i = 0; i < ids.size(); ++i) {
        Y_ASSERT(DocLumpMapping_.contains(ids[i]));
        Y_ENSURE(DocLumpMapping_.contains(ids[i]));

        mapping[i] = DocLumpMapping_.at(ids[i]);
    }
}

TBlob TWadRouter::LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const {
    return TWadRouter::LoadDocLumpsInternal(docId, mapping, regions);
}

TVector<TBlob> TWadRouter::LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const {
    return TWadRouter::LoadDocLumpsInternal(docIds, mapping, regions);
}

// Additional methods:
void TWadRouter::AddWad(THolder<IWad>&& wadArg) {
    Y_ASSERT(!Const_);
    Y_ENSURE(!Const_);

    Wads_.emplace_back(std::forward<THolder<IWad>>(wadArg));
}

void TWadRouter::Finish() {
    Y_ASSERT(!Const_);
    Y_ENSURE(!Const_);

    for (size_t index = 0; index < Wads_.size(); ++index) {
        const auto& wad = Wads_[index];

        // Check size:
        if (wad->Size() > Size_)
            Size_ = wad->Size();

        auto globalLumps = wad->GlobalLumps();
        for (auto&& lump : globalLumps) {
            GlobalLumps_[lump] = wad.Get();
        }

        auto docLumps = wad->DocLumps();
        TVector<size_t> mapping(docLumps.size());
        wad->MapDocLumps(docLumps, mapping);

        for (size_t i = 0; i < docLumps.size(); ++i) {
            TWadLumpId lump = docLumps[i];
            size_t lumpIndex = DocLumps_.size();

            DocLumps_.emplace_back(lump, index, mapping[i]);
            DocLumpMapping_[lump] = lumpIndex;
        }
    }

    Const_ = true;
}

template <class Region>
TBlob TWadRouter::LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const {
    Y_ASSERT(Const_);

    Y_ASSERT(mapping.size() == regions.size());
    Y_ENSURE(mapping.size() == regions.size());

    constexpr size_t cacheSize = 256;
    size_t localMapping[cacheSize];

    // Here I hope that user provided long sequences of lumps from one WAD. Of cause it is not always true
    // but document identifier is a constant so WAD shouldn't unload the BLOB even if it was mapped from SSD.
    // All in all it is the only way I see to prevent memory allocation at this place and I think that memory
    // allocation would be worse.
    for (size_t i = 0; i < mapping.size(); ) {
        size_t wad = DocLumps_[mapping[i]].WadIdx;
        localMapping[0] = DocLumps_[mapping[i]].LocalIndex;

        size_t j;
        for (j = 1; i + j < mapping.size() && j < cacheSize && DocLumps_[mapping[i + j]].WadIdx == wad; ++j) {
            localMapping[j] = DocLumps_[mapping[i + j]].LocalIndex;
        }
        // Be aware that blob returned by LoadDocLumps is discarded, which sometimes leads to
        // memory cleanup.
        Wads_[wad]->LoadDocLumps(docId, TArrayRef<const size_t>{localMapping, localMapping + j}, regions.Slice(i, j));

        i += j;
    }

    return {};
}

template <class Region>
TVector<TBlob> TWadRouter::LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const {
    Y_ASSERT(docIds.size() == regions.size());
    TVector<TBlob> result(docIds.size());
    for (size_t i = 0; i < docIds.size(); ++i) {
        result[i] = LoadDocLumps(docIds[i], mapping, regions[i]);
    }
    return result;
}

}
