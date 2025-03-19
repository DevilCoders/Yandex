#pragma once

#include <util/system/types.h>
#include <util/stream/output.h>
#include <utility>

namespace NDoom {
    class TSentLenHit {
    public:
        TSentLenHit()
            : DocId_(0)
            , Length_(0)
        {
        }

        explicit TSentLenHit(ui32 docId, ui32 length = 0)
            : DocId_(docId)
            , Length_(length)
        {
        }

        ui32 DocId() const {
            return DocId_;
        }

        ui32 Length() const {
            return Length_;
        }

        void SetDocId(ui32 docId) {
            DocId_ = docId;
        }

        void SetLength(ui32 length) {
            Length_ = length;
        }

        bool operator<(const TSentLenHit& hit) const {
            return Pair() < hit.Pair();
        }

        bool operator<=(const TSentLenHit& hit) const {
            return Pair() <= hit.Pair();
        }

        bool operator>(const TSentLenHit& hit) const {
            return Pair() > hit.Pair();
        }

        bool operator>=(const TSentLenHit& hit) const {
            return Pair() >= hit.Pair();
        }

        bool operator==(const TSentLenHit& hit) const {
            return Pair() == hit.Pair();
        }

        bool operator!=(const TSentLenHit& hit) const {
            return Pair() != hit.Pair();
        }

    private:
        std::pair<ui32, ui32> Pair() const {
            return std::make_pair(DocId_, Length_);
        }


        friend IOutputStream& operator<<(IOutputStream& stream, const TSentLenHit& hit) {
            stream << "[" << hit.DocId() << ":" << hit.Length() << "]";
            return stream;
        }

    private:
        ui32 DocId_ = 0;
        ui32 Length_ = 0;
    };
}
