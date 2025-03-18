#pragma once

#include "istore.h"
#include <library/cpp/cache/cache.h>
#include <util/system/rwlock.h>
#include <util/system/spinlock.h>
#include <util/generic/maybe.h>

namespace NAapi {
namespace NStore {

class TCache {
public:
    struct TElement {  // Add element ttl?
        TAdaptiveLock Lock;

        // Key
        ui64 X;
        ui64 Y;
        ui32 Z;

        TString Data;

        TElement()
            : X(0)
            , Y(0)
            , Z(0)
        {
        }

        explicit TElement(const TString& key) {
            CopyKey(key);
        }

        void CopyKey(const TString& key) {
            memcpy(&X, key.data(), 8);
            memcpy(&Y, key.data() + 8, 8);
            memcpy(&Z, key.data() + 16, 4);
        }

        void RefData(const TString& data) {
            Data = data;
        }

        static ui64 GetX(const TString& key) {
            ui64 res;
            memcpy(&res, key.data(), 8);
            return res;
        }

        bool operator==(const TElement& rhs) const {
            return X == rhs.X && Y == rhs.Y && Z == rhs.Z;
        }
    };

    TCache(size_t size)
        : Cache(size)
    {
    }

    void Put(const TString& key, const TString& value) {
        const ui64 pos = TElement::GetX(key) % Cache.size();

        with_lock(Cache[pos].Lock) {
            Cache[pos].CopyKey(key);
            Cache[pos].RefData(value);
        }
    }

    bool Get(const TString& key, TString& value) {
        TElement element(key);
        const ui64 pos = element.X % Cache.size();

        with_lock(Cache[pos].Lock) {
            if (Cache[pos] == element) {
                value = Cache[pos].Data;
                return true;
            }
        }

        return false;
    }

private:
    TVector<TElement> Cache;
};

class TRamStore {
public:
    TRamStore(size_t maxSize)
        : Cache(maxSize)
    {
    }

    void Put(const TString& key, const TString& data) {
        TWriteGuard guard(Mutex);
        Cache.Insert(key, data);
    }

    bool Get(const TString& key, TString& data) {
        TReadGuard guard(Mutex);
        auto it = Cache.FindWithoutPromote(key);

        if (it == Cache.End()) {
            return false;
        }

        data = it.Value();
        return true;
    }

private:
    TLRUCache<TString, TString> Cache;
    TRWMutex Mutex;
};

using TRamStoreHolder = THolder<TRamStore>;

class TCompoundRamStore {
public:
    explicit TCompoundRamStore(size_t maxSize)
        : L1(maxSize)
    {
        const size_t size = maxSize / 256;

        for (size_t i = 0; i < 256; ++i) {
            L2.emplace_back(new TRamStore(size));
        }
    }

    void Put(const TString& key, const TString& data) {
        L1.Put(key, data);
        ChooseL2(key)->Put(key, data);
    }

    bool Get(const TString& key, TString& data) {
        if (L1.Get(key, data)) {
            return true;
        } else if (ChooseL2(key)->Get(key, data)) {
            L1.Put(key, data);
            return true;
        }

        return false;
    }

private:
    TCache L1;
    TVector<TRamStoreHolder> L2;

    TRamStore* ChooseL2(const TString& key) {
        return L2[static_cast<ui8>(key[0])].Get();
    }
};

}  // namespace NStore
}  // namespace NAapi
