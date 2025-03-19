#pragma once

#include <util/generic/fwd.h>


namespace NDoom {


class TDocChunkMapping {
public:
    static const TDocChunkMapping Invalid;

    TDocChunkMapping() = default;

    TDocChunkMapping(ui32 chunk, ui32 localDocId)
        : Chunk_(chunk)
        , LocalDocId_(localDocId)
    {

    }

    ui32 Chunk() const {
        return Chunk_;
    }

    ui32 LocalDocId() const {
        return LocalDocId_;
    }

    friend bool operator==(const TDocChunkMapping& l, const TDocChunkMapping& r) {
        return l.Chunk_ == r.Chunk_ && l.LocalDocId_ == r.LocalDocId_;
    }

    friend bool operator!=(const TDocChunkMapping& l, const TDocChunkMapping& r) {
        return l.Chunk_ != r.Chunk_ || l.LocalDocId_ != r.LocalDocId_;
    }

private:
    ui32 Chunk_ = 0;
    ui32 LocalDocId_ = 0;
};

struct TDocChunkMappingVectorizer {
    enum {
        TupleSize = 2
    };

    template <class Slice>
    static void Scatter(const TDocChunkMapping& mapping, Slice&& slice) {
        slice[0] = mapping.Chunk();
        slice[1] = mapping.LocalDocId();
    }

    template <class Slice>
    static void Gather(Slice&& slice, TDocChunkMapping* mapping) {
        *mapping = TDocChunkMapping(slice[0], slice[1]);
    }
};


} // namespace NDoom
