#pragma once

#include <kernel/keyinv/hitlist/invsearch.h>

#include "yandex_hit_iterator.h"

namespace NDoom {


template <class Hit>
class TYandexHitSearcher {
public:
    using THit = Hit;
    using TPosition = ui32;
    using TIterator = TYandexHitIterator<Hit>;

    TYandexHitSearcher(const IKeysAndPositions* index)
        : Index_(index)
        , Size_(Index_->KeyCount())
    {
    }

    bool Seek(ui32 start, ui32 end, TIterator* iterator) const {
        Y_VERIFY(start + 1 == end);
        if (start >= Size_) {
            return false;
        }
        const YxRecord* entry = EntryByNumber(Index_, iterator->RequestContext_, start);
        iterator->PosIterator_.Init(*Index_, entry->Offset, entry->Length, entry->Counter, RH_DEFAULT);
        return true;
    }

private:
    const IKeysAndPositions* Index_ = nullptr;
    const ui32 Size_ = 0;
};


} // namespace NDoom
