#pragma once

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <kernel/doom/progress/progress.h>

namespace NDoom {

/**
 * `TSimpleMapReader` implements a common index reader interface for reading
 * from an `TMap`. It is used only for testing.
 */
template<class Hit>
class TSimpleMapReader {
public:
    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = void;
    using TIndexMap = TMap<TString, TVector<Hit>>;

    enum {
        HasLowerBound = false /* Can be implemented, but I'm just too lazy to do it. */
    };

    TSimpleMapReader(const TIndexMap& index)
        : IndexMap_(index)
    {
        Restart();
    }

    void Restart() {
        KeyIterator_ = IndexMap_.begin();
        PrevKeyIterator_ = KeyIterator_;
        HitIterator_ = KeyIterator_->second.begin();
        Progress_ = 0;
    }

    bool ReadKey(TKeyRef* keyRef, TKeyData* = NULL) {
        if (IndexMap_.end() == KeyIterator_) {
            return false;
        }
        PrevKeyIterator_ = KeyIterator_;
        ++KeyIterator_;
        HitIterator_ = PrevKeyIterator_->second.begin();
        *keyRef = PrevKeyIterator_->first;
        return true;
    }

    bool ReadHit(THit* hit) {
        if (PrevKeyIterator_->second.end() == HitIterator_) {
            return false;
        }
        *hit = *HitIterator_;
        ++HitIterator_;
        return true;
    }

    void Seek(const TKeyRef& keyRef) {
        Progress_ = 0; /* This breaks progress reporting, but we don't really care. */
        KeyIterator_ = IndexMap_.lower_bound(keyRef);
        PrevKeyIterator_ = KeyIterator_;
        if (IndexMap_.end() != KeyIterator_) {
            HitIterator_ = KeyIterator_->second.begin();
        }
    }

    void LowerBound(const TKeyRef&) {
        Y_VERIFY(false, "LowerBound unsupported, check HasLowerBound property.");
    }

    TProgress Progress() const {
        return TProgress(Progress_, IndexMap_.size());
    }

private:
    using TKeyIterator = typename TIndexMap::const_iterator;
    using THitIterator = typename TVector<Hit>::const_iterator;

private:
    const TIndexMap& IndexMap_;
    TKeyIterator KeyIterator_;
    TKeyIterator PrevKeyIterator_;
    THitIterator HitIterator_;
    ui64 Progress_ = 0;
};

} // namespace NDoom
