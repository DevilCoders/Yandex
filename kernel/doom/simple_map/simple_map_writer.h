#pragma once

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NDoom {

/**
 * TSimpleMapWriter implements common index-writer interface.
 * It is used only for testing needs to be compared to read writers.
 */
template<class Hit>
class TSimpleMapWriter {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using THit = Hit;
    using TIndexMap = TMap<TKey, TVector<THit>>;

    TSimpleMapWriter(TIndexMap& index)
        : Index_(index)
    {
    }

    void WriteHit(const THit& hit) {
        Hits_.push_back(hit);
    }

    void WriteKey(const TKeyRef& key) {
        Y_ASSERT(Index_.empty() || Index_.rbegin()->first <= key);
        TVector<THit>& keyHits = Index_[TString{key}];
        keyHits.insert(keyHits.end(), Hits_.begin(), Hits_.end());
        Hits_.clear();
    }

    void Finish() {
    }

private:
    TIndexMap& Index_;
    TVector<THit> Hits_;
};

} // namespace NDoom
