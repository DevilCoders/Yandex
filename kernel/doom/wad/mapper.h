#pragma once

#include "wad_lump_id.h"

#include <kernel/doom/wad/mega_wad_common.h>

#include <util/memory/blob.h>
#include <util/generic/vector.h>
#include <util/generic/array_ref.h>

namespace NDoom {

class IDocLumpMapper {
public:
    /**
     * \returns                         A list of per-doc lumps.
     */
    virtual TVector<TWadLumpId> DocLumps() const = 0;

    /**
     * \returns                         A list of per-doc lumps.
     */
    virtual TVector<TStringBuf> DocLumpsNames() const;

    /**
     * Maps lump types to ids that are later to be used in per-doc
     * queries.
     *
     * \param types                     Lump types to map.
     * \param[out] mapping              Resulting lump type ids.
     * \throws yexception               If some of the lump types are not found.
     */
    virtual void MapDocLumps(const TArrayRef<const NDoom::TWadLumpId>& ids, TArrayRef<size_t> mapping) const = 0;

    /**
     * Maps lump types to ids that are later to be used in per-doc
     * queries.
     *
     * \param types                     Lump types to map.
     * \param[out] mapping              Resulting lump type ids.
     * \throws yexception               If some of the lump types are not found.
     */
    virtual void MapDocLumps(TConstArrayRef<TStringBuf> names, TArrayRef<size_t> mapping) const;
    virtual void MapDocLumps(TConstArrayRef<TString> names, TArrayRef<size_t> mapping) const;

    virtual ~IDocLumpMapper() = default;
};

class IDocLumpLoader {
public:
    virtual bool HasDocLump(size_t docLumpId) const = 0;

    virtual void LoadDocRegions(TConstArrayRef<size_t> mapping, TArrayRef<TConstArrayRef<char>> regions) const = 0;

    virtual TBlob DataHolder() const = 0;

    virtual ui32 Chunk() const { return 0; }

    virtual ~IDocLumpLoader() = default;
};

} // namespace NDoom
