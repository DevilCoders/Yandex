#pragma once

#include <util/system/types.h>
#include <util/stream/output.h>

// Very stupid Erf chunk

namespace NDoom {
    class TErfHit {
    public:
        TErfHit(ui32 docId = 0, ui32 chunk = 0)
            : DocId_(docId)
            , Chunk_(chunk)
        {
        }

        ui32 DocId() const {
            return DocId_;
        }

        ui32 Chunk() const {
            return Chunk_;
        }

        void SetDocId(ui32 docId) {
            DocId_ = docId;
        }
        void SetChunk(ui32 chunk) {
            Chunk_ = chunk;
        }
        bool operator==(const TErfHit& hit) const {
            return DocId_ == hit.DocId_ && Chunk_ == hit.Chunk_;
        }
        bool operator<(const TErfHit& hit) const {
            return DocId_ < hit.DocId_;
        }
    private:

        friend IOutputStream& operator<<(IOutputStream& stream, const TErfHit& hit) {
            stream << "[" << hit.Chunk() << "]";
            return stream;
        }

    private:
        ui32 DocId_ = 0;
        ui32 Chunk_ = 0;
    };
}
