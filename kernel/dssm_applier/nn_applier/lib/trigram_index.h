#pragma once
#include "saveload_utils.h"
#include "blob_array.h"
#include <library/cpp/containers/comptrie/comptrie.h>

namespace NNeuralNetApplier {

class TTrigramsIndex {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    size_t NumBins = 0;
    size_t Shift = 0;
    TBlobArray<ui8> WcharMap;
    TBlobArray<ui32> Remap;

    TStringStream BufferWcharMap;
    TStringStream RemapBuffer;
    TString TrieBuffer;

public:
    TTrigramsIndex() = default;
    TTrigramsIndex(const TTermToIndex& mapping, size_t numBins = 128)
        : NumBins(numBins)
        , Shift(GetNumBits(NumBins - 1))
    {
        TrieBuffer = HashMapToTrieString(mapping);
        TStringStream stream(TrieBuffer);
        Trie.Init(TBlob::FromStream(stream));

        const auto wcharMap = BuildWcharMap(Trie);
        ::Save(&BufferWcharMap, wcharMap.size());
        ::SaveArray(&BufferWcharMap, wcharMap.data(), wcharMap.size());
        WcharMap.Load(TBlob::FromStream(BufferWcharMap));

        size_t stride = 1 << Shift;
        TVector<ui32> remap(stride * stride * stride, Max<ui32>());

        for (const auto& info : Trie) {
            remap[GetRawId(info.first)] = info.second;
        }

        for (size_t i = 0; i < stride; ++i) {
            for (size_t j = 0; j < stride; ++j) {
                for (size_t k = 0; k < stride; ++k) {
                    if (i == (stride - 1) || j == (stride - 1) || k == (stride - 1)) {
                        remap[(i << (2 * Shift)) + (j << Shift) + k] = Max<ui32>();
                    }
                }
            }
        }

        ::Save(&RemapBuffer, remap.size() * sizeof(ui32));
        ::SaveArray(&RemapBuffer, remap.data(), remap.size());
        Remap.Load(TBlob::FromStream(RemapBuffer));
    }

    void Init(const TBlob& blob) {
        TBlob curBlob = blob;
        TBlob trieBlob = ReadBlob(curBlob);
        Trie.Init(trieBlob);
        NumBins = ReadSize(curBlob);
        Shift = GetNumBits(NumBins - 1);
        size_t wcharMapSize = WcharMap.Load(curBlob);
        curBlob = curBlob.SubBlob(wcharMapSize, curBlob.Size());
        size_t remapSize = Remap.Load(curBlob);
        curBlob = curBlob.SubBlob(remapSize, curBlob.Size());
        Y_ENSURE(curBlob.Empty(), "wrong read from curBlob!");
    }

    void Save(IOutputStream* s) const {
        const auto trieString = TrieToString(Trie);
        ::Save(s, trieString.size());
        *s << trieString;
        ::Save(s, NumBins);
        WcharMap.Save(s);
        Remap.Save(s);
    }

    bool Find(const TWtringBuf& w, size_t* idx) const {
        *idx = Remap[GetRawId(w)];
        if (*idx != Max<ui32>()) {
            return true;
        }
        return Trie.Find(w, idx);
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const {
        return Trie;
    }

private:
    ui32 GetRawId(const TWtringBuf& w) const {
        return (WcharMap[w[2]] << (2 * Shift)) + (WcharMap[w[1]] << Shift) + WcharMap[w[0]];
    }

    size_t GetNumBits(ui32 v, size_t bit = 0) {
        return (v >> bit) == 0 ? bit : GetNumBits(v, bit + 1);
    }

    TVector<ui8> BuildWcharMap(const TCompactTrie<TUtf16String::char_type, size_t>& trie) const {
        TVector<std::pair<wchar16, size_t>> counts(Max<wchar16>());
        for (const auto& info : trie) {
            Y_ENSURE(info.first.size() == 3, "trigrams should have len == 3");
            for (auto c : info.first) {
                ++counts[c].second;
                counts[c].first = c;
            }
        }
        Sort(counts, [](const auto& left, const auto& right) {return left.second > right.second; });

        TVector<ui8> toReturn(Max<wchar16>() + 1, (1 << Shift) - 1);
        for (size_t i = 0; i < Min(NumBins - 1, counts.size()); ++i) {
            toReturn[counts[i].first] = i;
        }
        return toReturn;
    }
};

}
