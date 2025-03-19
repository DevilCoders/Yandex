#pragma once

#include <kernel/doom/wad/wad.h>

#include <util/generic/hash.h>
#include <util/generic/set.h>

/*
 * Universal WAD merger which works at the runtime.
 * This class allows you to move indexes between the WADs without changing the reader code.
 * I've written it for migrating from one MegaWAD to three L1/L2/L3 WADs in Images but I hope it could be useful for other teams too.
 */

namespace NDoom {

class TWadRouter: public IWad {
public:
    TWadRouter();
    TWadRouter(const TWadRouter&) = delete;
    TWadRouter(TWadRouter&&) = delete;
    TWadRouter& operator=(const TWadRouter&) = delete;
    TWadRouter& operator=(TWadRouter&&) = delete;

    ~TWadRouter() override;

    // WAD interface:
    ui32 Size() const override;
    TVector<TWadLumpId> GlobalLumps() const override;
    bool HasGlobalLump(TWadLumpId id) const override;
    TVector<TWadLumpId> DocLumps() const override;
    TBlob LoadGlobalLump(TWadLumpId id) const override;
    void MapDocLumps(const TArrayRef<const TWadLumpId>& ids, TArrayRef<size_t> mapping) const override;
    // WARNING: LoadDocLumps ALWAYS returns empty region here.
    TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const override;
    TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const override;

    // Additional methods:
    /*
     * Adds additional WAD to the internal storage and takes ownership of this WAD.
     * After you add this WAD you'll be able to use all the lumps from it.
     * If there was found a collision of the lump types new index would have higher priority.
     */
    void AddWad(THolder<IWad>&& wad);

    /*
     * Opens new WAD using IWad::Open with your arguments and adds it to the internal storage.
     */
    template <typename...T>
    void OpenWad(T&&...args) {
        AddWad(std::forward<THolder<IWad>>(IWad::Open(std::forward<T>(args)...)));
    }

    /*
     * You should call Finish after you've added all the WADs.
     */
    void Finish();

private:
    template <class Region>
    TBlob LoadDocLumpsInternal(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<Region> regions) const;
    template <class Region>
    TVector<TBlob> LoadDocLumpsInternal(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<Region>> regions) const;

    bool Const_ = false;

    TVector<THolder<IWad>> Wads_;
    ui32 Size_ = 0;

    struct TLumpInfo {
        TLumpInfo(TWadLumpId id, size_t wad, size_t idx)
            : Id(id)
            , WadIdx(wad)
            , LocalIndex(idx)
        {}

        TWadLumpId Id;
        size_t WadIdx = 0;
        size_t LocalIndex = 0;
    };

    TVector<TLumpInfo> DocLumps_;
    THashMap<TWadLumpId, size_t> DocLumpMapping_;
    THashMap<TWadLumpId, const IWad*> GlobalLumps_;
};

}


