#pragma once
#include <util/generic/ptr.h>
#include <util/system/rwlock.h>
#include <array>


template <class TKeyExt, class TValueExt>
class TSimpleKVSnapshot {
public:
    using TKey = TKeyExt;
    using TValue = TValueExt;
private:
    TMap<TKey, TValue> Objects;
public:

    typename TMap<TKey, TValue>::const_iterator begin() const {
        return Objects.begin();
    }

    typename TMap<TKey, TValue>::const_iterator end() const {
        return Objects.end();
    }

    bool Add(const TKey& key, const TValue& value) {
        return Objects.emplace(key, value).second;
    }

    const TValue* GetPointer(const TKey& key) const {
        auto it = Objects.find(key);
        if (it != Objects.end()) {
            return &it->second;
        }
        return nullptr;
    }

    size_t Size() const {
        return Objects.size();
    }

    TValue Get(const TKey& key) const {
        auto value = GetPointer(key);
        if (value) {
            return *value;
        } else {
            return TValue();
        }
    }
};

template <class T, size_t SwitchSize>
class TSwitchSnapshotsCollection {
private:
    std::array<TAtomicSharedPtr<T>, SwitchSize> Objects;
    std::array<TRWMutex, SwitchSize> Guards;
    TAtomic IndexCurrent = 0;
public:
    void Switch(TAtomicSharedPtr<T> val) {
        const i64 idxGlobal = AtomicGet(IndexCurrent);
        const size_t idx = (idxGlobal + 1) % SwitchSize;
        TWriteGuard wg(Guards[idx]);
        Objects[idx] = std::move(val);
        AtomicIncrement(IndexCurrent);
    }

    TAtomicSharedPtr<T> Get() const {
        const i64 idxGlobal = AtomicGet(IndexCurrent);
        const size_t idx = (idxGlobal) % SwitchSize;
        TReadGuard wg(Guards[idx]);
        return Objects[idx];
    }
};

