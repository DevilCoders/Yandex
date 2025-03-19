#pragma once

#include "trie_complex_key_iterator.h"

#include <array>

namespace NSaasTrie {
    struct TUrlMaskHostIterator {
        // magic number from https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/infected_masks/infected_masks.cpp?rev=3176868#L204
        static const ui32 MaxHosts = 5;

        bool SetUrl(TStringBuf url);

        bool AtEnd() const {
            return Index >= MaxHosts;
        }
        TStringBuf CurrentHost() const {
            return TStringBuf{FullHost.data() + Heads[Index], FullHost.size() - Heads[Index]};
        }
        bool CheckOwner();
        bool Next() {
            return ++Index < MaxHosts;
        }

    private:
        TStringBuf Url;
        TStringBuf FullHost;
        std::array<ui32, MaxHosts> Heads;
        ui32 Index = 0;
        ui32 OwnerLimit = 0;
    };

    struct TUrlMaskPathIterator {
        // magic number from https://a.yandex-team.ru/arc/trunk/arcadia/library/cpp/infected_masks/infected_masks.cpp?rev=3176868#L258
        static const ui32 MaxPaths = 6;

        void SetUrl(TStringBuf url);

        void Reset() {
            Index = 0;
        }
        bool AtEnd() const {
            return Index >= End;
        }
        TStringBuf CurrentPath() const {
            return Paths[Index];
        }
        bool EndsWithSlash() const {
            return Index == SlashIndex;
        }
        bool Next() {
            return ++Index < End;
        }

    private:
        TStringBuf FullPath;
        std::array<TStringBuf, MaxPaths> Paths;
        ui32 Index = 0;
        ui32 End = 0;
        ui32 SlashIndex = MaxPaths;
    };

    THolder<ITrieStorageIterator> CreateTrieUrlMaskIterator(const ITrieStorageReader& trie,
                                                            const TComplexKeyPreprocessed& key);
}
