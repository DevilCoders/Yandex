#include "hash_trie.h"

struct TCmpWords {
    const TVector<TVector<int>>& Words;

    TCmpWords(const TVector<TVector<int>>& words)
        : Words(words)
    {
    }
    bool operator()(int n1, int n2) const {
        const TVector<int>& a = Words[n1];
        const TVector<int>& b = Words[n2];
        size_t pos = 0;
        for (; pos < a.size() && pos < b.size(); ++pos) {
            if (a[pos] < b[pos])
                return true;
            if (a[pos] > b[pos])
                return false;
        }
        if (b.size() > pos)
            return true;
        return false; // equal is also false
    }
};

static void BuildTrie(const TVector<TVector<int>>& words,
                      size_t pos,
                      const TVector<int>& sortedList, int beg, int kon, int parentId, ui32 parentHash,
                      int bucketCount,
                      THashMap<ui32, TVector<int>>* hash2leafs,
                      TVector<TSimpleHashTrie::TLeaf>* resLeafs) {
    for (int i = beg; i < kon;) {
        int letterStart = i;
        TSimpleHashTrie::TLeaf leaf;
        {
            int idx = sortedList[letterStart];
            const TVector<int>& seq = words[idx];
            Y_ASSERT(seq.size() > pos);
            leaf.Char = seq[pos];
            leaf.ParentId = parentId;
            if (seq.size() == pos + 1) {
                leaf.WordId = idx;
                ++letterStart;
            }
        }
        for (++i; i < kon; ++i) {
            int idx = sortedList[i];
            const TVector<int>& seq = words[idx];
            if (seq[pos] != leaf.Char)
                break;
        }
        int leafId = resLeafs->size();
        resLeafs->push_back(leaf);
        ui32 hashVal = TSimpleHashTrie::HashAddChar(parentHash, leaf.Char);
        (*hash2leafs)[hashVal % bucketCount].push_back(leafId);
        BuildTrie(words, pos + 1,
                  sortedList, letterStart, i, leafId, hashVal,
                  bucketCount, hash2leafs, resLeafs);
    }
}

void BuildTrie(const TVector<TVector<int>>& words, TSimpleHashTrie* res) {
    TVector<int> sortedList;
    for (size_t i = 0; i < words.size(); ++i)
        sortedList.push_back(i);
    std::sort(sortedList.begin(), sortedList.end(), TCmpWords(words));

    size_t bucketCount = 1;
    while (words.size() * 2 > bucketCount)
        bucketCount *= 2;

    THashMap<ui32, TVector<int>> hash2leafs;
    TVector<TSimpleHashTrie::TLeaf> leafs;
    BuildTrie(words, 0, sortedList, 0, sortedList.size(), -1, 0, bucketCount, &hash2leafs, &leafs);

    TVector<int> leafRename;
    leafRename.resize(leafs.size(), -1);

    res->LeafPtrs.push_back(0);
    for (size_t bucket = 0; bucket < bucketCount; ++bucket) {
        const TVector<int>& bl = hash2leafs[bucket];
        for (size_t i = 0; i < bl.size(); ++i) {
            int idx = bl[i];
            leafRename[idx] = res->Leafs.size();
            res->Leafs.push_back(leafs[idx]);
        }
        res->LeafPtrs.push_back(res->Leafs.size());
    }
    for (size_t i = 0; i < res->Leafs.size(); ++i) {
        TSimpleHashTrie::TLeaf& dst = res->Leafs[i];
        if (dst.ParentId >= 0)
            dst.ParentId = leafRename[dst.ParentId];
    }
}

void DecodeTrie(const TSimpleHashTrie& trie, TVector<TVector<int>>& words) {
    for (auto leaf : trie.Leafs) {
        if (leaf.WordId < 0)
            continue;
        TVector<int> word;
        word.push_back(leaf.Char);
        auto cur = leaf;
        while (cur.ParentId >= 0) {
            cur = trie.Leafs[cur.ParentId];
            word.push_back(cur.Char);
        }
        Reverse(word.begin(), word.end());
        words.push_back(word);
    }
}
