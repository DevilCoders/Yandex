#pragma once

#include <kernel/keyinv/hitlist/invsearch.h>

#include "yandex_key_data.h"

namespace NDoom {


class TYandexKeyIterator {
public:
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = TYandexKeyData;

    bool ReadKey(TKeyRef* key, TKeyData* data) {
        if (Position_ >= Size_) {
            return false;
        }
        const YxRecord* entry = EntryByNumber(Index_, RequestContext_, Position_);

        *key = entry->TextPointer;

        data->SetHitCount(entry->Counter);
        data->SetStart(Position_);
        data->SetEnd(Position_ + 1);

        ++Position_;

        return true;
    }

private:
    friend class TYandexKeySearcher;

    TRequestContext RequestContext_;
    const IKeysAndPositions* Index_ = nullptr;
    ui32 Position_ = 0;
    ui32 Size_ = 0;
};


} // namespace NDoom
