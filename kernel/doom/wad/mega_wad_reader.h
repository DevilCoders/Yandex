#pragma once

#include <util/generic/array_ref.h>

#include <kernel/doom/progress/progress.h>

#include "wad.h"

namespace NDoom {

/**
 * Sequential read adaptor for `IWad` implementations that makes writing
 * sequential decoders of per-doc data on top of it way simpler.
 */
class TMegaWadReader {
public:
    TMegaWadReader() = default;

    TMegaWadReader(const IWad* wad, TArrayRef<const TWadLumpId> docLumps) {
        Reset(wad, docLumps);
    }

    void Reset(const IWad* wad, TArrayRef<const TWadLumpId> docLumps) {
        Wad_ = wad;

        DocLumpMapping_.resize(docLumps.size());
        Wad_->MapDocLumps(docLumps, DocLumpMapping_);

        CurrentDoc_ = 0;
        DocCount_ = Wad_->Size();
    }

    void Restart() {
        CurrentDoc_ = 0;
    }

    /**
     * This one just duplicates the method in `IWad` for simplicity.
     *
     * \param id                        Id of the global lump to load.
     * \returns                         Loaded lump.
     */
    TBlob LoadGlobalLump(TWadLumpId id) const {
        return Wad_->LoadGlobalLump(id);
    }

    /**
     * Reads the next non-empty document from the underlying wad.
     *
     * \param[out] docId                Document id.
     * \param[out] regions              Contiguous container of `TArrayRef<const char>`s that is filled
     *                                  with doc lumps. The order is the same as was
     *                                  supplied in constructor.
     * \returns                         Whether the operation was successful. A return value of false
     *                                  signals end of wad.
     */
    template<class DataRegionRange>
    bool ReadDoc(ui32* docId, DataRegionRange* regions) {
        return ReadDoc(docId, MakeArrayRef(*regions));
    }

    bool ReadDoc(ui32* docId, TArrayRef<TArrayRef<const char>> regions) {
        do {
            if (CurrentDoc_ >= DocCount_)
                return false;

            CurrentBlob_ = Wad_->LoadDocLumps(CurrentDoc_, DocLumpMapping_, regions);
            CurrentDoc_++;
        } while (AllEmpty(regions));

        *docId = CurrentDoc_ - 1;
        return true;
    }

    TProgress Progress() const {
        return TProgress(CurrentDoc_, DocCount_);
    }

public:
    static bool AllEmpty(const TArrayRef<TArrayRef<const char>>& regions) {
        for (const TArrayRef<const char>& region : regions)
            if (!region.empty())
                return false;
        return true;
    }

private:
    TBlob CurrentBlob_;
    const IWad* Wad_ = nullptr;
    TVector<size_t> DocLumpMapping_;
    size_t CurrentDoc_ = 0;
    size_t DocCount_ = 0;
};

} // namespace NDoom
