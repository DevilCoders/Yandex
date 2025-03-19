#pragma once

#include <util/system/types.h>
#include <utility>
#include <tuple>

namespace NDoom {

class TClickSimHit {
public:
    TClickSimHit() = default;

    TClickSimHit(ui32 docId, ui32 wordHash, ui32 weight = 0)
        : DocId_(docId)
        , WordHash_(wordHash)
        , Weight_(weight) {
    }

    TClickSimHit(const TClickSimHit&) = default;
    TClickSimHit(TClickSimHit&&) = default;
    TClickSimHit& operator=(const TClickSimHit&) = default;
    TClickSimHit& operator=(TClickSimHit&&) = default;

    ui32 DocId() const {
        return DocId_;
    }

    ui32 WordHash() const {
        return WordHash_;
    }

    ui32 Weight() const {
        return Weight_;
    }

    void SetDocId(ui32 docId) {
        DocId_ = docId;
    }

    void SetWordHash(ui32 wordHash) {
        WordHash_ = wordHash;
    }

    void SetWeight(ui32 weight) {
        Weight_ = weight;
    }

    bool operator == (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) == std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

    bool operator != (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) != std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

    bool operator < (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) < std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

    bool operator <= (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) <= std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

    bool operator > (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) > std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

    bool operator >= (const TClickSimHit& hit) const {
        return std::tie(DocId_, WordHash_, Weight_) >= std::tie(hit.DocId_, hit.WordHash_, hit.Weight_);
    }

private:
    ui32 DocId_ = 0;
    ui32 WordHash_ = 0;
    ui32 Weight_ = 0;
};

} // NDoom
