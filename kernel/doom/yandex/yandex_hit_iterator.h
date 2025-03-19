#pragma once

#include <kernel/keyinv/hitlist/invsearch.h>
#include <kernel/keyinv/hitlist/positerator.h>

namespace NDoom {


template <class Hit>
class TYandexHitIterator {
public:
    using THit = Hit;

    bool ReadHit(THit* hit) {
        if (!PosIterator_.Valid()) {
            return false;
        }
        *hit = PosIterator_.Current();
        PosIterator_.Next();
        return true;
    }

    template <class Consumer>
    bool ReadHits(const Consumer& consumer) {
        while (PosIterator_.Valid()) {
            if (!consumer(PosIterator_.Current())) {
                return false;
            }
            PosIterator_.Next();
        }
        return false;
    }

private:
    template <class OtherHit>
    friend class TYandexHitSearcher;

    TRequestContext RequestContext_;
    TPosIterator<> PosIterator_;
};


} // namespace NDoom
