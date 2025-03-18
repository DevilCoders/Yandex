#pragma once

#include <library/cpp/offroad/utility/tagged.h>

namespace NOffroad {
    template <class HitReader, class KeyReader>
    class TKeyInvIterator: private NPrivate::TTaggedBase {
    public:
        using THit = typename HitReader::THit;

        TKeyInvIterator() {
        }

        Y_FORCE_INLINE bool ReadHit(THit* hit) {
            return HitReader_.ReadHit(hit);
        }

        template <class Consumer>
        Y_FORCE_INLINE bool ReadHits(const Consumer& consumer) {
            return HitReader_.ReadHits(consumer);
        }

        Y_FORCE_INLINE bool LowerBound(const THit& prefix, THit* first) {
            return HitReader_.LowerBound(prefix, first);
        }

    private:
        template <class KeyDataFactory, class OtherHitReader, class OtherKeyReader, class KeySeeker>
        friend class TKeyInvSearcher;

    private:
        HitReader HitReader_;
        KeyReader KeyReader_;
    };

}
