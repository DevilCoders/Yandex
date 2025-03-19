#pragma once

#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include "utils.h"

namespace NUrlTranslitSimilarity {
    struct TPtRec {
        TString Dst;
        double Weight = 0.0;

        TPtRec() = default;
        TPtRec(const TString& str, double w)
            : Dst(str)
            , Weight(w)
        {}
    };

    inline bool operator < (const TPtRec& sn1, const TPtRec& sn2) {
        return sn1.Dst < sn2.Dst;
    }

    using TPtRecMap = TMap<TString, TVector<TPtRec>>;
    struct TrieNode;
    using TTrieMap = TMap<char, TrieNode>;

    using TrieNodePtr = TrieNode *;
    using TrieNodeConstPtr = const TrieNode *;

    struct TrieNode {
        TTrieMap Children;
        int Value;

        TrieNode() {
            Clear();
        }

        void Clear() {
            Children.clear();
            Value = -1;
        }

        void Insert(const TString& word, int value) {
            TrieNodePtr current = this;
            for (ui32 i = 0; i < word.size(); i++) {
                current = &(current->Children[word[i]]);
            }
            current->Value = value;
        }

        void InsertNGram(const TString& word, int value) {
            TrieNodePtr current = this;
            for (ui32 i = 0; i < word.size(); i++) {
                if (word[i] != '~') {
                    current = &(current->Children[word[i]]);
                }
            }
            current->Value = value;
        }

        TrieNodeConstPtr Search(const TString& word, int pos0 = 0) const {
            TrieNodeConstPtr current = this;
            for (ui32 i = pos0; i < word.size(); i++) {
                TTrieMap::const_iterator it = current->Children.find(word[i]);
                if (it == current->Children.end()) {
                    return nullptr;
                }
                current = &it->second;
            }
            return current;
        }

        void Search(const TString& word, TVector<std::pair<TrieNodeConstPtr, int>>& path, int pos0 = 0) const {
            TrieNodeConstPtr current = this;
            path.clear();
            for (ui32 i = pos0; i < word.size(); i++) {
                TTrieMap::const_iterator it = current->Children.find(word[i]);
                if (it == current->Children.end()) {
                    return;
                }
                current = &it->second;
                if (current->Value >= 0) {
                    path.push_back(std::pair<TrieNodeConstPtr, int>(current, i));
                }
            }
        }
    };

    using TPtRecVec = TVector<TVector<TPtRec>>;

    struct TDecodeResult {
        int WordNum = 0;
        double Weight = 0.0;
        TDecodeResult() = default;
        TDecodeResult(int dstWordNum, double weight)
            : WordNum(dstWordNum)
            , Weight(weight)
        {}
    };

    using TSearchPath = TMap<TrieNodeConstPtr, double>;

    class TLettersDecode {
    public:
        void DoDecode(const TString& srcString, TrieNodeConstPtr langModel,
            TVector<TSearchPath>& searchBeam) const;
        TDecodeResult SearchWords(const TString& srcWord, const TVector<TString>& dstWords) const;
        int LoadPhraseModel(IInputStream* inputStream, bool invertTable,
            double maxWeight, size_t maxSearchWidth);
    private:
        TrieNode SrcPhr;
        TPtRecVec PhraseModel;
        size_t MaxSearchWidth = 0;
    };
}
