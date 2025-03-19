#include "trie_complex_key_iterator.h"

#include "config.h"

#include "trie_iterator_chain.h"
#include "trie_url_mask_iterator.h"

#include <kernel/saas_trie/idl/saas_trie.pb.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/string/join.h>

namespace NSaasTrie {
    namespace {
        template<typename T>
        struct TValueReader;

        template<>
        struct TValueReader<ui64> {
            ui64 GetValue() const {
                return Value;
            }
            bool TryRead(const TString& key) {
                return Trie->Get(key, Value);
            }
            void ResetTrie(const ITrieStorageReader* trie) {
                Trie = trie;
            }
            bool IsPrefixLevel() const {
                return false;
            }
            void SetPrefixLevel(bool) {
            }
        private:
            const ITrieStorageReader* Trie = nullptr;
            ui64 Value = 0;
        };

        template<>
        struct TValueReader<ITrieStorageReader> {
            const ITrieStorageReader* GetValue() const {
                return Value.Get();
            }
            bool TryRead(const TString& key) {
                Value = Trie->GetSubTree(key);
                return !Value->IsEmpty();
            }
            void ResetTrie(const ITrieStorageReader* trie) {
                Trie = trie;
                Value.Reset();
            }
            bool IsPrefixLevel() const {
                return PrefixLevel;
            }
            void SetPrefixLevel(bool prefixLevel) {
                PrefixLevel = prefixLevel;
            }
        private:
            const ITrieStorageReader* Trie = nullptr;
            THolder<ITrieStorageReader> Value;
            bool PrefixLevel = false;
        };

        template<typename TValue>
        struct TTrieKeysIterator {
            void SetKeys(const TVector<TString>& keys) {
                Keys = keys.data();
                End = keys.size();
            }
            void SetKeys(const TString* keys, size_t nKeys) {
                Keys = keys;
                End = nKeys;
            }
            bool Reset(const ITrieStorageReader* trie) {
                Reader.ResetTrie(trie);
                Index = 0;
                return FoundExistentKey();
            }
            void Close() {
                Index = End;
            }
            bool AtEnd() const {
                return Index >= End;
            }
            bool Next() {
                ++Index;
                return FoundExistentKey();
            }
            const TString& GetKey() const {
                return Keys[Index];
            }
            auto GetValue() const {
                return Reader.GetValue();
            }
            bool IsPrefixLevel() const {
                return Reader.IsPrefixLevel();
            }
            void SetPrefixLevel(bool prefixLevel) {
                Reader.SetPrefixLevel(prefixLevel);
            }

        private:
            bool FoundExistentKey() {
                for (; Index < End; ++Index) {
                    if (Reader.TryRead(Keys[Index])) {
                        return true;
                    }
                }
                return false;
            }

            TValueReader<TValue> Reader;
            const TString* Keys = nullptr;
            size_t Index = 0;
            size_t End = 0;
        };

        const TRealm& FindRealm(const TComplexKey& key, const TString& realmName) {
            size_t nRealms = key.AllRealmsSize();
            for (size_t i = 0; i < nRealms; ++i) {
                auto& realm = key.GetAllRealms(i);
                if (realm.GetName() == realmName) {
                    return realm;
                }
            }
            ythrow yexception() << "Can not find realm \"" << realmName << "\" in TComplexKey";
        }

        void CopySortedRealm(const TRealm& input, TVector<TString>& output, bool needSort) {
            size_t realmSize = input.KeySize();
            output.resize(realmSize);
            for (size_t i = 0; i < realmSize; ++i) {
                output[i] = input.GetKey(i);
            }
            if (needSort) {
                Sort(output);
            }
        }
    }

    struct TSingleRealmIterator : ITrieStorageIterator {
        TSingleRealmIterator(const ITrieStorageReader& trie, const TComplexKeyPreprocessed& key)
            : ComplexKey(key)
        {
            auto nLevels = ComplexKey.SortedRealms.size();
            Y_ENSURE(nLevels < 2);
            if (nLevels == 0) {
                Iterator.SetKeys(&ComplexKey.Prefix, 1);
                Iterator.Reset(&trie);
            } else {
                MainSubTrie = trie.GetSubTree(ComplexKey.Prefix);
                if (!MainSubTrie->IsEmpty()) {
                    Iterator.SetKeys(ComplexKey.SortedRealms[0]);
                    Iterator.Reset(MainSubTrie.Get());
                }
            }
        }
        bool AtEnd() const override {
            return Iterator.AtEnd();
        }
        bool Next() override {
            return Iterator.Next();
        }
        TString GetKey() const override {
            return MainSubTrie ? ComplexKey.Prefix + Iterator.GetKey() : Iterator.GetKey();
        }
        ui64 GetValue() const override {
            return Iterator.GetValue();
        }

    private:
        const TComplexKeyPreprocessed& ComplexKey;
        THolder<ITrieStorageReader> MainSubTrie;
        TTrieKeysIterator<ui64> Iterator;
    };

    struct TMultiRealmIterator : ITrieStorageIterator {
        TMultiRealmIterator(const ITrieStorageReader& trie, const TComplexKeyPreprocessed& key)
            : ComplexKey(key)
        {
            auto nLevels = ComplexKey.SortedRealms.size();
            Y_ENSURE(nLevels >= 2 && nLevels < MaxNumberOfRealms);
            MainSubTrie = trie.GetSubTree(ComplexKey.Prefix);
            if (!MainSubTrie->IsEmpty()) {
                MaxLevel = static_cast<ui32>(nLevels - 1);
                Iterator.SetKeys(ComplexKey.SortedRealms[MaxLevel]);
                Levels.resize(MaxLevel);
                for (ui32 i = 0; i < MaxLevel; ++i) {
                    Levels[i].SetKeys(ComplexKey.SortedRealms[i]);
                    if (key.RealmPrefixSet[i]) {
                        Levels[i].SetPrefixLevel(true);
                    }
                }
                Levels[0].Reset(MainSubTrie.Get());
                UpdateLevelIterators();
            }
            KeyBuffer.reserve(nLevels + 1);
        }
        bool AtEnd() const override {
            return CurrentLevel == MaxLevel && Iterator.AtEnd();
        }
        bool Next() override {
            if (CurrentLevel < MaxLevel) {
                return StepInto();
            }
            if (Iterator.Next()) {
                Value = Iterator.GetValue();
                return true;
            }
            CurrentLevel = MaxLevel - 1;
            Levels[CurrentLevel].Next();
            return UpdateLevelIterators();
        }
        TString GetKey() const override {
            KeyBuffer.clear();
            if (MainSubTrie) {
                KeyBuffer.emplace_back(ComplexKey.Prefix);
            }
            for (ui32 i = 0; i < CurrentLevel; ++i) {
                KeyBuffer.emplace_back(Levels[i].GetKey());
            }
            if (CurrentLevel < MaxLevel) {
                KeyBuffer.emplace_back(Levels[CurrentLevel].GetKey());
            } else {
                KeyBuffer.emplace_back(Iterator.GetKey());
            }
            return JoinSeq(TStringBuf{}, KeyBuffer);
        }
        ui64 GetValue() const override {
            return Value;
        }

    private:
        bool StepInto() {
            if (CurrentLevel + 1 == MaxLevel) {
                if (Iterator.Reset(Levels[CurrentLevel].GetValue())) {
                    Value = Iterator.GetValue();
                    CurrentLevel = MaxLevel;
                    return true;
                }
                Levels[CurrentLevel].Next();
            } else {
                Levels[CurrentLevel + 1].Reset(Levels[CurrentLevel].GetValue());
                ++CurrentLevel;
            }
            return UpdateLevelIterators();
        }

        bool UpdateLevelIterators() {
            for (;;) {
                if (Levels[CurrentLevel].AtEnd()) {
                    do {
                        if (CurrentLevel == 0) {
                            CurrentLevel = MaxLevel;
                            Iterator.Close();
                            return false;
                        }
                        --CurrentLevel;
                    } while (!Levels[CurrentLevel].Next());
                }
                if (Levels[CurrentLevel].IsPrefixLevel() && Levels[CurrentLevel].GetValue()->Get({}, Value)) {
                    return true;
                }
                if (CurrentLevel + 1 < MaxLevel) {
                    Levels[CurrentLevel + 1].Reset(Levels[CurrentLevel].GetValue());
                    ++CurrentLevel;
                } else {
                    if (Iterator.Reset(Levels[CurrentLevel].GetValue())) {
                        CurrentLevel = MaxLevel;
                        Value = Iterator.GetValue();
                        return true;
                    }
                    Levels[CurrentLevel].Next();
                }
            }
        }

        const TComplexKeyPreprocessed& ComplexKey;
        THolder<ITrieStorageReader> MainSubTrie;
        TVector<TTrieKeysIterator<ITrieStorageReader>> Levels;
        TTrieKeysIterator<ui64> Iterator;
        ui64 Value = 0;
        ui32 CurrentLevel = 0;
        ui32 MaxLevel = 0;
        mutable TVector<TStringBuf> KeyBuffer;
    };

    TComplexKeyPreprocessed PreprocessComplexKey(const TComplexKey& key, bool needSort, const TString& uniqueDimension) {
        TComplexKeyPreprocessed outKey;
        ui64 kps = key.HasKeyPrefix() ? key.GetKeyPrefix() : 0u;
        if (!key.HasUrlMaskPrefix()) {
            TString mainKey = key.HasMainKey() ? key.GetMainKey() : TString{};
            outKey.Prefix = GetTrieKey(kps, mainKey);
        } else if (!key.HasMainKey()) {
            outKey.UrlMaskPrefix = GetTrieKey(kps, key.GetUrlMaskPrefix());
        } else {
            outKey.Prefix = GetTrieKey(kps, key.GetMainKey());
            outKey.UrlMaskPrefix = GetTrieKey(kps, key.GetUrlMaskPrefix());
        }
        outKey.RealmPrefixSet = ReadRealmPrefixSet(key);

        size_t nRealms = key.KeyRealmsSize();
        outKey.SortedRealms.resize(nRealms);
        for (size_t i = 0; i < nRealms; ++i) {
            CopySortedRealm(FindRealm(key, key.GetKeyRealms(i)), outKey.SortedRealms[i], needSort);
        }

        if (key.HasLastRealmUnique()) {
            outKey.LastDimensionUnique = key.GetLastRealmUnique();
        } else {
            outKey.LastDimensionUnique = nRealms > 0 && key.GetKeyRealms(nRealms - 1) == uniqueDimension;
        }

        return outKey;
    }

    inline THolder<ITrieStorageIterator> CreateRealmIterator(const ITrieStorageReader& trie,
                                                             const TComplexKeyPreprocessed& key) {
        if (key.SortedRealms.size() < 2) {
            return MakeHolder<TSingleRealmIterator>(trie, key);
        }
        return MakeHolder<TMultiRealmIterator>(trie, key);
    }

    THolder<ITrieStorageIterator> CreateTrieComplexKeyIterator(const ITrieStorageReader& trie,
                                                               const TComplexKeyPreprocessed& key) {
        if (key.UrlMaskPrefix.empty()) {
            return CreateRealmIterator(trie, key);
        }
        if (key.Prefix.empty()) {
            return CreateTrieUrlMaskIterator(trie, key);
        }
        return CreateTrieIteratorChain(
            CreateRealmIterator(trie, key),
            CreateTrieUrlMaskIterator(trie, key)
        );
    }
}
