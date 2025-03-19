#pragma once

#include <array>
#include <type_traits>

#include <kernel/doom/wad/wad_lump_id.h>

namespace NDoom {

template<EWadIndexType indexType, class PrefixVectorizer>
class TOffroadDocWadMapping {
public:
    enum {
        DocLumpCount = PrefixVectorizer::TupleSize == 0 ? 1 : 2
    };

    TOffroadDocWadMapping() {}

    std::array<TWadLumpId, DocLumpCount> DocLumps() const {
        std::array<TWadLumpId, DocLumpCount> result;

        result[0] = TWadLumpId(indexType, EWadLumpRole::Hits);
        if (DocLumpCount == 2)
            result[1] = TWadLumpId(indexType, EWadLumpRole::HitSub);

        return result;
    }

    TWadLumpId ModelLump() const {
        return TWadLumpId(indexType, EWadLumpRole::HitsModel);
    }

    template<class Stream, class ArgsRange>
    void ResetStream(Stream* stream, const typename Stream::TTable* table, const ArgsRange& args) const {
        ResetStream(stream, table, args, std::make_index_sequence<DocLumpCount>());
    }

private:
    template<class Stream, class ArgsRange, size_t... indexes>
    void ResetStream(Stream* stream, const typename Stream::TTable* table, const ArgsRange& args, std::index_sequence<indexes...>) const {
        stream->Reset(table, args[indexes]...);
    }
};


} // namespace NDoom
