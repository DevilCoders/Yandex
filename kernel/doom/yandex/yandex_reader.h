#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

#include <kernel/keyinv/indexfile/searchfile.h>   /* For TYndex4Searching. */
#include <kernel/keyinv/hitlist/invsearch.h>      /* For TRequestContext. */
#include <kernel/keyinv/hitlist/positerator.h>    /* For TPosIterator. */

#include <kernel/doom/progress/progress.h>

#include "yandex_key_data.h"

namespace NDoom {


template<class Hit>
class TYandexReader: private TNonCopyable {
public:
    using THit = Hit;
    using TKey = TString;
    using TKeyRef = TStringBuf;
    using TKeyData = TYandexKeyData;

    enum {
        HasLowerBound = true,
        HitLayers = 1
    };

    TYandexReader(const TString& path, READ_HITS_TYPE hitsReadMode = RH_DEFAULT)
    {
        IndexHolder_ = MakeHolder<TYndex4Searching>();
        IndexHolder_->InitSearch(path, hitsReadMode);

        Index_ = IndexHolder_.Get();
        Size_ = Index_->KeyCount();
    }

    TYandexReader(const IKeysAndPositions* index, ui32 position = 0)
        : Index_(index)
        , Position_(position)
        , Size_(Index_->KeyCount())
    {
    }

    bool ReadKey(TKeyRef* key, TKeyData* data = nullptr) {
        if (Position_ >= Size_)
            return false;

        Record_ = EntryByNumber(Index_, RequestContext_, Position_);
        *key = Record_->TextPointer;
        IteratorInitialized_ = false;

        if(data) {
            data->SetHitCount(Record_->Counter);
            data->SetStart(Position_);
            data->SetEnd(Position_ + 1);
        }

        Position_++;
        return true;
    }

    bool ReadHit(THit* hit) {
        bool valid = Iterator_.Valid() & IteratorInitialized_;
        if (!valid) {
            if (!IteratorInitialized_) {
                Iterator_.Init(*Index_, Record_->Offset, Record_->Length, Record_->Counter, RH_DEFAULT);
                IteratorInitialized_ = true;
                if (!Iterator_.Valid())
                    return false;
            } else {
                return false;
            }
        }

        CopyHitInternal(Iterator_.Current(), hit);
        Iterator_.Next();
        return true;
    }

    void LowerBound(const TKeyRef& key) {
        i32 position = Index_->LowerBound(key.data(), RequestContext_);
        Position_ = position < 0 ? Size_ : static_cast<ui32>(position);
    }

    void Restart() {
        Position_ = 0;
    }

    TProgress Progress() const {
        return {Position_, Size_};
    }

private:
    static void CopyHitInternal(const SUPERLONG& src, SUPERLONG* dst) {
        *dst = src;
    }

    template<class OtherHit>
    static void CopyHitInternal(const SUPERLONG& src, OtherHit* dst) {
        *dst = OtherHit::FromSuperLong(src);
    }

private:
    const IKeysAndPositions* Index_;
    THolder<TYndex4Searching> IndexHolder_;
    TRequestContext RequestContext_;
    const YxRecord* Record_ = nullptr;
    ui32 Position_ = 0;
    ui32 Size_ = 0;

    TPosIterator<> Iterator_;
    bool IteratorInitialized_ = false;
};


} // namespace NDoom
