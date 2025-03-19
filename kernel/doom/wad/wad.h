#pragma once

#include <util/memory/blob.h>
#include <util/generic/array_ref.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>
#include <util/generic/maybe.h>

#include "mapper.h"
#include "wad_lump_id.h"
#include "wad_index_type.h"

class TBuffer;

namespace NDoom {

/**
 * Abstract base for accessing WAD files in a unified way.
 */
class IWad : public IDocLumpMapper {
public:
    struct TDocLumpData {
        /* Blob holding document. */
        TBlob Blob;
        /* Data regions in Blob corresponding to document lumps.*/
        TConstArrayRef<TArrayRef<const char>> Regions;
    };


public:
    static THolder<IWad> Open(const TString& path, bool lockMemory = false);
    static THolder<IWad> Open(const TArrayRef<const char>& source);
    static THolder<IWad> Open(TBuffer&& buffer);
    static THolder<IWad> Open(const TBlob& blob);

    ~IWad() override = default;

    /**
     * \returns                         Number of documents in this wad.
     */
    virtual ui32 Size() const = 0;

    /**
     * \returns                         A list of all global lumps in this wad.
     */
    virtual TVector<TWadLumpId> GlobalLumps() const = 0;

    /**
     * \param type                      Type of the global lump to check that it exists in this wad
     * \returns                         True iff this lump exists
     */
    virtual bool HasGlobalLump(TWadLumpId id) const = 0;

    /**
     * \param type                      Type of the global lump to read from this wad.
     * \returns                         Global lump for the provided lump type in TBlob.
     * \throws yexception               If the lump is not found.
     */
    virtual TBlob LoadGlobalLump(TWadLumpId id) const = 0;

    /**
     * \returns                         A list of all global lumps in this wad.
     */
    virtual TVector<TString> GlobalLumpsNames() const;

    /**
     * \param type                      Type of the global lump to check that it exists in this wad
     * \returns                         True iff this lump exists
     */
    virtual bool HasGlobalLump(TStringBuf id) const;

    /**
     * \param type                      Type of the global lump to read from this wad.
     * \returns                         Global lump for the provided lump type in TBlob.
     * \throws yexception               If the lump is not found.
     */
    virtual TBlob LoadGlobalLump(TStringBuf id) const;

    /**
     * Loads document lumps. Note that if the provided docid is out of range, this
     * function simply returns empty data regions.
     *
     * \param docId                     Document id to read lumps for.
     * \param mapping                   List of lump type ids to read from the document. Use `MapDocLumps` to obtain it.
     * \param[out] regions              Data regions in the returned blob corresponding to the
     *                                  requested lump types.
     * \returns                         TBlob holding the document.
     */
    virtual TBlob LoadDocLumps(ui32 docId, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<const char>> regions) const = 0;

    virtual TVector<TBlob> LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, TArrayRef<TArrayRef<TArrayRef<const char>>> regions) const;

    /**
     * Iterates over document lumps.
     * \param callback                  Callback function: given index in docIds array, and data on successful document load
     *                                  Nothing on failure.
     */
    virtual void LoadDocLumps(const TArrayRef<const ui32>& docIds, const TArrayRef<const size_t>& mapping, std::function<void(size_t, TMaybe<TDocLumpData>&&)> callback) const;

protected:
    IWad() = default;
};

} // namespace NDoom


