#pragma once

#include <library/cpp/binsaver/bin_saver.h>
#include <util/generic/vector.h>

struct TSimpleHashTrie {
    struct TLeaf {
        int ParentId, Char;
        int WordId;

        TLeaf()
            : ParentId(-1)
            , Char(0)
            , WordId(-1)
        {
        }

        Y_SAVELOAD_DEFINE(ParentId, Char, WordId);
    };
    TVector<TLeaf> Leafs;
    TVector<int> LeafPtrs;

    SAVELOAD(Leafs, LeafPtrs);
    Y_SAVELOAD_DEFINE(Leafs, LeafPtrs); // Arcadia has two different serialization frameworks. Different users of this library need both. Such is life.

    int GetMaxWordId() const {
        int res = 0;
        for (size_t i = 0; i < Leafs.size(); ++i) {
            res = Max(res, Leafs[i].WordId);
        }
        return res;
    }
    static ui32 HashAddChar(ui32 h, int Char) {
        return 27214361 + h * 821345 + Char;
    }

    class TIterator {
        const TSimpleHashTrie& Trie;
        int LeafId;
        ui32 Hash;
        int WordId;

    public:
        TIterator(const TSimpleHashTrie& trie)
            : Trie(trie)
        {
            Reset();
        }
        void Reset() {
            LeafId = -1;
            Hash = 0;
            WordId = -1;
        }
        bool NextChar(int c) {
            ui32 hashVal = TSimpleHashTrie::HashAddChar(Hash, c);
            int bucket = hashVal & (Trie.LeafPtrs.size() - 2);
            for (int b = Trie.LeafPtrs[bucket], k = Trie.LeafPtrs[bucket + 1]; b < k; ++b) {
                const TSimpleHashTrie::TLeaf& curLeaf = Trie.Leafs[b];
                if (curLeaf.ParentId == LeafId && curLeaf.Char == c) {
                    LeafId = b;
                    Hash = hashVal;
                    WordId = curLeaf.WordId;
                    return true;
                }
            }
            return false;
        }
        int GetWordId() const {
            return WordId;
        }
    };
};

void BuildTrie(const TVector<TVector<int>>& words, TSimpleHashTrie* res);

void DecodeTrie(const TSimpleHashTrie& trie, TVector<TVector<int>>& words);
