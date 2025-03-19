#pragma once

#include <util/system/types.h>
#include <util/stream/output.h>
#include <utility>

namespace NDoom {
    class TSentHit {
    public:
        TSentHit()
            : DocId_(0)
            , Offset_(0)
        {
        }

        explicit TSentHit(ui32 docId, ui32 offset = 0)
            : DocId_(docId)
            , Offset_(offset)
        {
        }

        TSentHit(const TSentHit& sent)
            : DocId_(sent.DocId_)
            , Offset_(sent.Offset_)
        {
        }

        ~TSentHit() = default;

        TSentHit& operator=(const TSentHit& hit) {
            DocId_ = hit.DocId_;
            Offset_ = hit.Offset_;
            return *this;
        }

        ui32 DocId() const {
            return DocId_;
        }

        ui32 Offset() const {
            return Offset_;
        }

        void SetDocId(ui32 docId) {
            DocId_ = docId;
        }

        void SetOffset(ui32 offset) {
            Offset_ = offset;
        }

        bool operator<(const TSentHit& hit) const {
            return Pair() < hit.Pair();
        }

        bool operator<=(const TSentHit& hit) const {
            return Pair() <= hit.Pair();
        }

        bool operator>(const TSentHit& hit) const {
            return Pair() > hit.Pair();
        }

        bool operator>=(const TSentHit& hit) const {
            return Pair() >= hit.Pair();
        }

        bool operator==(const TSentHit& hit) const {
            return Pair() == hit.Pair();
        }

        bool operator!=(const TSentHit& hit) const {
            return Pair() != hit.Pair();
        }

    private:
        std::pair<ui32, ui32> Pair() const {
            return std::make_pair(DocId_, Offset_);
        }


        friend IOutputStream& operator<<(IOutputStream& stream, const TSentHit& hit) {
            stream << "[" << hit.DocId() << ":" << hit.Offset() << "]";
            return stream;
        }

    private:
        ui32 DocId_;
        ui32 Offset_;
    };
}
