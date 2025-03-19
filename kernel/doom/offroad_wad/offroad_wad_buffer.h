#pragma once

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

namespace NDoom {


template<class Hit>
class TOffroadWadBuffer {
public:
    using THit = Hit;

    TOffroadWadBuffer() {}

    void Reset() {
        TermId_ = 0;
        HitsByDocId_.clear();
    }

    void Write(const THit& hit) {
        ui32 docId = hit.DocId();
        if (docId >= HitsByDocId_.size())
            HitsByDocId_.resize(docId + 1);

        THit tmp = hit;
        tmp.SetDocId(TermId_);
        HitsByDocId_[docId].push_back(tmp);
    }

    void WriteSeekPoint() {
        TermId_++;
    }

    size_t Size() const {
        return HitsByDocId_.size();
    }

    const TVector<THit>& Hits(ui32 docId) {
        return HitsByDocId_[docId];
    }

    void RemapTermIds(const TVector<ui32>& positions) {
        for (size_t docId = 0, docIdEnd = HitsByDocId_.size(); docId < docIdEnd; ++docId) {
            for (THit& hit : HitsByDocId_[docId]) {
                hit.SetDocId(positions[hit.DocId()]);
            }
            Sort(HitsByDocId_[docId].begin(), HitsByDocId_[docId].end());
        }
    }

private:
    ui32 TermId_ = 0;
    TVector<TVector<THit>> HitsByDocId_;
};


} // namespace NDoom

