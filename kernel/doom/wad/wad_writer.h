#pragma once

#include "wad_lump_id.h"

#include <util/generic/array_ref.h>
#include <util/generic/array_ref.h>

namespace NDoom {


class IWadWriter {
public:
    virtual ~IWadWriter() = default;

    virtual void RegisterDocLumpType(TWadLumpId id) = 0;
    virtual void RegisterDocLumpType(TStringBuf id);

    void RegisterDocLumpTypes(TArrayRef<const TWadLumpId> ids) {
        for (TWadLumpId id : ids) {
            RegisterDocLumpType(id);
        }
    }

    virtual IOutputStream* StartGlobalLump(TWadLumpId id) = 0;
    virtual IOutputStream* StartGlobalLump(TStringBuf id);

    template <typename Id>
    void WriteGlobalLump(Id id, const TArrayRef<const char>& data) {
        IOutputStream* output = StartGlobalLump(id);
        output->Write(data.data(), data.size());
    }

    virtual IOutputStream* StartDocLump(ui32 docId, TWadLumpId id) = 0;
    virtual IOutputStream* StartDocLump(ui32 docId, TStringBuf id);

    template <typename Id>
    void WriteDocLump(ui32 docId, Id id, const TArrayRef<const char>& data) {
        IOutputStream* output = StartDocLump(docId, id);
        output->Write(data.data(), data.size());
    }

    virtual bool IsFinished() const = 0;

    virtual void Finish() = 0;
};


} // namespace NDoom
