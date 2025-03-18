#pragma once

#include <string.h>

#include <util/system/filemap.h>

//This is LRU cache of fixed-size key and values
//TKey should be integer with random distribution such as hash of strings
//TValue should be fixed size class
//Memory is preallocated in Init and not allocated or freed during work

template <class TKey, class TValue>
class TLRUCache {
private:
    class TEntry {
    public:
        TEntry* Next;
        TEntry* Mru;
        TEntry* Lru;
        TKey Key;
        TValue Value;
    };

private:
    TMappedArray<TEntry> Storage;
    TMappedArray<TEntry*> Pairs;
    TEntry* Free = nullptr;
    TEntry* Mru = nullptr;
    ui32 Mask;
    bool Inited = false;

public:
    void Init(ui32 pairBits, ui32 cacheSize) {
        if (Inited)
            return;
        Mask = (1 << pairBits) - 1;
        Storage.Create(cacheSize);
        Pairs.Create(1 << pairBits);
        for (ui32 i = 1; i < cacheSize; i++) {
            Storage[i - 1].Next = &Storage[i];
        }
        Storage[cacheSize - 1].Next = nullptr;
        Free = &Storage[0];
        memset(&Pairs[0], 0, (1 << pairBits) * sizeof(TEntry*));
        Mru = nullptr;
        Inited = true;
    }

private:
    TEntry* GetFreePair() {
        TEntry* result;
        if (Free == nullptr) {
            Y_ASSERT(Mru);
            Y_ASSERT(Mru->Lru);
            TEntry* lru = Mru->Lru;
            lru->Mru->Lru = lru->Lru;
            lru->Lru->Mru = lru->Mru;
            Erase(lru->Key);
            result = lru;
        } else {
            result = Free;
            Free = Free->Next;
        }
        return result;
    }

    TEntry* operator[](TKey key) {
        TEntry** firstPair = &Pairs[key & Mask];
        TEntry* pair = *firstPair;
        while (pair) {
            if (pair->Key == key)
                return pair;
            pair = pair->Next;
        }
        TEntry* newPair = GetFreePair();
        newPair->Next = *firstPair;
        *firstPair = newPair;
        newPair->Mru = nullptr;
        newPair->Key = key;
        return newPair;
    }

    void Erase(ui64 key) {
        TEntry** lastPair = &Pairs[key & Mask];
        TEntry* pair;
        while ((pair = *lastPair)) {
            if (pair->Key == key) {
                *lastPair = pair->Next;
                return;
            }
            lastPair = &pair->Next;
        }
    }

    void UpdateMru(TEntry* entry) {
        if (Mru) {
            entry->Mru = Mru;
            entry->Lru = Mru->Lru;
            Mru->Lru->Mru = entry;
            Mru->Lru = entry;
        } else {
            entry->Mru = entry;
            entry->Lru = entry;
        }
        Mru = entry;
    }

public:
    void FindOrCalc(TKey key, TValue& value, std::function<void(TKey, TValue&)> func) {
        TEntry* la = (*this)[key];
        if (la->Mru == nullptr) {
            func(key, value);
            la->Value = value;
            UpdateMru(la);
            return;
        }
        value = la->Value;
        if (la != Mru) {
            la->Mru->Lru = la->Lru;
            la->Lru->Mru = la->Mru;
            UpdateMru(la);
        }
    }

    TValue* FindOrCalc(TKey key, std::function<void(TKey, TValue&)> func) {
        TEntry* la = (*this)[key];
        if (la->Mru == nullptr) {
            func(key, la->Value);
            UpdateMru(la);
            return &la->Value;
        }
        if (la != Mru) {
            la->Mru->Lru = la->Lru;
            la->Lru->Mru = la->Mru;
            UpdateMru(la);
        }
        return &la->Value;
    }
};
