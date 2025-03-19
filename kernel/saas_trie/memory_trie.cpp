#include "memory_trie.h"

#include <util/generic/deque.h>
#include <util/generic/map.h>
#include <util/system/mutex.h>

#include <functional>

namespace NSaasTrie {
    using TMapType = TMap<TString, ui64>;
    using TMapIterator = TMapType::const_iterator;
    using TEndCallback = std::function<bool(const TMapIterator&)>;

    struct IUnlockable {
        virtual ~IUnlockable() = default;
        virtual void Unlock() const = 0;
    };

    struct TMemoryTrieIterator : ITrieStorageIterator {
        TMemoryTrieIterator(const IUnlockable& guard, TMapIterator iterator, TMapIterator endIterator)
            : Guard(guard)
            , Iterator(iterator)
            , EndIterator(endIterator)
        {
        }

        ~TMemoryTrieIterator() {
            Guard.Unlock();
        }

        bool AtEnd() const override {
            return Iterator == EndIterator;
        }

        TString GetKey() const override {
            return Iterator->first;
        }

        ui64 GetValue() const override {
            return Iterator->second;
        }

        bool Next() override {
            ++Iterator;
            return !AtEnd();
        }

    protected:
        const IUnlockable& Guard;
        TMapIterator Iterator;
        TMapIterator EndIterator;
    };

    struct TMemoryTriePrefixIterator : TMemoryTrieIterator {
        TMemoryTriePrefixIterator(const IUnlockable& guard, TMapIterator iterator, TMapIterator endIterator, TString prefix)
            : TMemoryTrieIterator(guard, iterator, endIterator)
            , Prefix(std::move(prefix))
        {
        }

        bool AtEnd() const override {
            return Iterator == EndIterator || !Iterator->first.StartsWith(Prefix);
        }

        TString GetKey() const override {
            return Iterator->first.substr(Prefix.size());
        }

    protected:
        TString Prefix;
    };

    struct TSubTreeDecorator : ITrieStorageReader {
        TSubTreeDecorator(const ITrieStorageReader& parent, TString prefix)
            : Parent(parent)
            , Prefix(std::move(prefix))
        {
        }

        bool Get(TStringBuf key, ui64& value) const override {
            return Parent.Get(Prefix + key, value);
        }
        ui64 GetSize() const override {
            auto iterator = Parent.CreatePrefixIterator(Prefix);
            ui64 size = 0;
            if (!iterator->AtEnd()) {
                do {
                    ++size;
                } while (iterator->Next());
            }
            return size;
        }
        bool IsEmpty() const override {
            auto iterator = Parent.CreatePrefixIterator(Prefix);
            return iterator->AtEnd();
        }
        THolder<ITrieStorageIterator> CreateIterator() const override {
            return Parent.CreatePrefixIterator(Prefix);
        }
        THolder<ITrieStorageIterator> CreatePrefixIterator(TStringBuf prefix) const override {
            return Parent.CreatePrefixIterator(Prefix + prefix);
        }
        THolder<ITrieStorageReader> GetSubTree(TStringBuf prefix) const override {
            return Parent.GetSubTree(Prefix + prefix);
        }

    private:
        const ITrieStorageReader& Parent;
        TString Prefix;
    };

    struct TMemoryStorage : ITrieStorage, IUnlockable {
        bool Get(TStringBuf key, ui64& value) const override {
            TGuard<TMutex> lock(Mutex);
            auto it = Storage.find(key);
            if (it == Storage.end()) {
                return false;
            }
            value = it->second;
            return true;
        }

        ui64 GetSize() const override {
            return Size;
        }

        bool IsEmpty() const override {
            return Size == 0;
        }

        THolder<ITrieStorageIterator> CreateIterator() const override {
            Mutex.Acquire();
            return MakeHolder<TMemoryTrieIterator>(*this, Storage.cbegin(), Storage.cend());
        }

        THolder<ITrieStorageIterator> CreatePrefixIterator(TStringBuf prefix) const override {
            Mutex.Acquire();
            auto first = Storage.lower_bound(prefix);
            return MakeHolder<TMemoryTriePrefixIterator>(*this, first, Storage.cend(), TString{prefix});
        }

        THolder<ITrieStorageReader> GetSubTree(TStringBuf prefix) const override {
            return MakeHolder<TSubTreeDecorator>(*this, TString{prefix});
        }

        bool IsReadOnly() const override {
            return false;
        }

        bool Put(TStringBuf key, ui64 value) override {
            TGuard<TMutex> lock(Mutex);
            auto result = Storage.emplace(key, value);
            if (result.second) {
                ++Size;
            } else {
                result.first->second = value;
            }
            return true;
        }

        bool Delete(TStringBuf key) override {
            TGuard<TMutex> lock(Mutex);
            auto removedCount = Storage.erase(TString(key));
            Size -= removedCount;
            return removedCount > 0;
        }

        bool DeleteIfEqual(TStringBuf key, ui64 value) override {
            TGuard<TMutex> lock(Mutex);
            auto it = Storage.find(key);
            if (it == Storage.end() || it->second != value) {
                return false;
            }
            Storage.erase(it);
            --Size;
            return true;
        }

        void Discard() override {
            TGuard<TMutex> lock(Mutex);
            Storage.clear();
        }

        void Unlock() const override {
            Mutex.Release();
        }

    private:
        mutable TMutex Mutex;
        TMapType Storage;
        volatile ui64 Size = 0;
    };

    TAtomicSharedPtr<ITrieStorage> CreateMemoryTrie() {
        return MakeAtomicShared<TMemoryStorage>();
    }

    ui64 RemoveValuesFromTrie(ITrieStorage& storage, THashSet<ui64> valuesToRemove) {
        auto iterator = storage.CreateIterator();
        if (iterator->AtEnd()) {
            return 0;
        }
        TDeque<std::pair<TString, ui64>> toRemove;
        do {
            auto value = iterator->GetValue();
            auto it = valuesToRemove.find(value);
            if (it != valuesToRemove.end()) {
                toRemove.emplace_back(iterator->GetKey(), value);
                valuesToRemove.erase(it);
            }
        } while (iterator->Next());
        iterator.Reset(); // release mutex in memory trie
        ui64 deleteCount = 0;
        for (auto& item : toRemove) {
            if (Y_LIKELY(storage.DeleteIfEqual(item.first, item.second))) {
                ++deleteCount;
            }
        }
        return deleteCount;
    }
}
