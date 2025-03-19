#pragma once

#include <kernel/keyinv/hitlist/invsearch.h>

#include "yandex_key_data.h"
#include "yandex_key_iterator.h"

namespace NDoom {


class TYandexKeySearcher {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = TYandexKeyData;
    using TIterator = TYandexKeyIterator;

    TYandexKeySearcher(const IKeysAndPositions* index)
        : Index_(index)
        , Size_(Index_->KeyCount())
    {
    }

    bool LowerBound(const TKeyRef& key, TKeyRef* firstKey, TKeyData* firstData, TIterator* iterator) const {
        i32 position = Index_->LowerBound(key.data(), iterator->RequestContext_);
        if (position < 0 || static_cast<ui32>(position) >= Size_) {
            return false;
        }

        iterator->Index_ = Index_;
        iterator->Position_ = static_cast<ui32>(position);
        iterator->Size_ = Size_;

        const YxRecord* entry = EntryByNumber(Index_, iterator->RequestContext_, iterator->Position_);

        *firstKey = entry->TextPointer;

        firstData->SetHitCount(entry->Counter);
        firstData->SetStart(iterator->Position_);
        firstData->SetEnd(iterator->Position_ + 1);

        return true;
    }

private:
    const IKeysAndPositions* Index_ = nullptr;
    const ui32 Size_ = 0;
};


} // namespace NDoom
