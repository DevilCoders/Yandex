#include "disk_trie.h"

#include "config.h"
#include "disk_io.h"

#include <library/cpp/containers/comptrie/comptrie_trie.h>

#include <util/generic/maybe.h>
#include <util/system/madvise.h>
#include <util/system/mlock.h>

namespace NSaasTrie {
    using TTrie = TCompactTrie<char, ui64>;
    using TTrieIterator = TTrie::TConstIterator;

    struct TDiskTrieIterator : ITrieStorageIterator {
        TDiskTrieIterator(TTrie trie)
            : Trie(std::move(trie))
            , Iterator(Trie.begin())
            , End(Trie.end())
        {
        }

        bool AtEnd() const override {
            return Iterator == End;
        }

        TString GetKey() const override {
            return Iterator.GetKey();
        }

        ui64 GetValue() const override {
            return Iterator.GetValue();
        }

        bool Next() override {
            return ++Iterator != End;
        }

    private:
        TTrie Trie;
        TTrieIterator Iterator;
        TTrieIterator End;
    };

    struct TTrieWrapper : ITrieStorage {
        explicit TTrieWrapper(TTrie trie)
            : Trie(std::move(trie))
        {
        }

        explicit TTrieWrapper(const TBlob& blob, bool lockMemory = false)
            : Trie(blob)
        {
            if (lockMemory) {
                ::LockMemory(blob.Data(), blob.Length());
                LockedBlob.ConstructInPlace(blob);
            } else {
                MadviseRandomAccess(blob.Data(), blob.Length());
            }
        }

        ~TTrieWrapper() {
            if (LockedBlob.Defined()) {
                ::UnlockMemory(LockedBlob->Data(), LockedBlob->Length());
            }
        }

        bool Get(TStringBuf key, ui64& value) const override {
            return Trie.Find(key, &value);
        }

        ui64 GetSize() const override {
            return Trie.Size();
        }

        bool IsEmpty() const override {
            return Trie.IsEmpty();
        }

        THolder<ITrieStorageIterator> CreateIterator() const override {
            return MakeHolder<TDiskTrieIterator>(Trie);
        }

        THolder<ITrieStorageIterator> CreatePrefixIterator(TStringBuf prefix) const override {
            return MakeHolder<TDiskTrieIterator>(Trie.FindTails(prefix));
        }

        THolder<ITrieStorageReader> GetSubTree(TStringBuf prefix) const override {
            return MakeHolder<TTrieWrapper>(Trie.FindTails(prefix));
        }

        // stubs for ITrieStorageWriter interface
        bool IsReadOnly() const override {
            return true;
        }
        bool Put(TStringBuf /*key*/, ui64 /*value*/) override {
            return false;
        }
        bool Delete(TStringBuf /*key*/) override {
            return false;
        }
        bool DeleteIfEqual(TStringBuf /*key*/, ui64 /*value*/) override {
            return false;
        }
        void Discard() override {
        }

    private:
        TTrie Trie;
        TMaybe<TBlob> LockedBlob;
    };

    TBlob GetDiskTrieBlob(const TString& path, const IDiskIO& disk) {
        auto blob = disk.Map(path);
        if (blob.IsNull()) {
            return blob;
        }
        const size_t headerSize = sizeof(TrieFileHeader) + sizeof(TrieVersion) + sizeof(ui64);
        if (blob.Size() < headerSize) {
            return {};
        }
        ui32 version = *reinterpret_cast<const ui32*>(blob.AsCharPtr() + sizeof(TrieFileHeader));
        ui64 trieSize = *reinterpret_cast<const ui64*>(blob.AsCharPtr() + sizeof(TrieFileHeader) + sizeof(ui32));
        if (version != TrieVersion || blob.Size() != headerSize + trieSize) {
            return {};
        }
        return blob.SubBlob(headerSize, headerSize + trieSize);
    }

    TAtomicSharedPtr<ITrieStorage> OpenDiskTrie(const TString& path, const IDiskIO& disk, bool lockMemory) {
        auto trieBlob = GetDiskTrieBlob(path, disk);
        if (trieBlob.IsNull()) {
            return {};
        }
        return MakeAtomicShared<TTrieWrapper>(trieBlob, lockMemory);
    }

    bool CheckDiskTrieIsValid(const TString& path, const IDiskIO& disk) {
        return !GetDiskTrieBlob(path, disk).IsNull();
    }
}
