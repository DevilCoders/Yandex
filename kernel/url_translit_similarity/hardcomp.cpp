#include "hardcomp.h"
#include <util/string/split.h>

namespace NUrlTranslitSimilarity {
    struct TSearchNode {
        TrieNodeConstPtr DstPrefix; // приложившаяся часть результирующего слова
        double Weight;

        TSearchNode() {
            DstPrefix = nullptr;
            Weight = 0.0;
        }
        TSearchNode(TrieNodeConstPtr prefix, double eval)
            : DstPrefix(prefix)
            , Weight(eval)
        {}
        bool operator < (const TSearchNode& sn) const {
            return Weight < sn.Weight;
        }
    };

    void InsertSearchNode(TSearchPath& searchPath, TrieNodeConstPtr prefix, double pathEval) {
        TSearchPath::iterator it = searchPath.find(prefix);
        if (it == searchPath.end()) {
            searchPath[prefix] = pathEval;
            return;
        }
        if (it->second > pathEval) {
            it->second = pathEval;
        }
    }

    void TLettersDecode::DoDecode(const TString& srcString, TrieNodeConstPtr langModel,
        TVector<TSearchPath>& searchBeam) const
    {
        size_t srcStrLength = srcString.length();
        searchBeam.resize(srcStrLength + 1);
        for (auto& ts : searchBeam) {
            ts.clear();
        }
        InsertSearchNode(searchBeam[0], langModel, 0.0);
        TVector<std::pair<TrieNodeConstPtr, int>> triePath;

        // цикл по буквам исходного слова
        for (size_t si = 0; si < srcStrLength; si++) {
            const TSearchPath& sp = searchBeam[si];
            if (sp.empty()) {
                continue;
            }
            TVector<TSearchNode> dstWindow;
            for (const auto& tsl : sp) {
                dstWindow.push_back(TSearchNode(tsl.first, tsl.second));
            }
            if (dstWindow.size() > MaxSearchWidth) {
                std::partial_sort(dstWindow.begin(), dstWindow.begin() + MaxSearchWidth, dstWindow.end());
                dstWindow.resize(MaxSearchWidth);
            }

            // исходная часть фразы
            SrcPhr.Search(srcString, triePath, si);
            for (size_t ti = 0; ti < triePath.size(); ti++) {
                TrieNodeConstPtr tn = triePath[ti].first;
                int nextPos = triePath[ti].second + 1;
                int tv = tn->Value;
                Y_ENSURE(tv >= 0);
                const TVector<TPtRec>& dstPhrases = PhraseModel[tv];
                // цикл по коридору поиска
                for (const TSearchNode& dstPath : dstWindow) {
                    // цикл по вариантам перевода исходной части фразы
                    for (const TPtRec& dstPhr : dstPhrases) {
                        // результирующая часть фразы
                        TrieNodeConstPtr dstPathPos = dstPath.DstPrefix->Search(dstPhr.Dst);
                        if (dstPathPos != nullptr) {
                            InsertSearchNode(searchBeam[nextPos], dstPathPos,
                                dstPhr.Weight + dstPath.Weight);
                        }
                    }
                }
            }
        }
    }

    TDecodeResult TLettersDecode::SearchWords(const TString& srcWord, const TVector<TString>& dstWords) const {
        TrieNode LangModel;
        for (size_t wi = 0; wi < dstWords.size(); wi++) {
            TString td = "{" + dstWords[wi] + "}";
            LangModel.InsertNGram(td, wi);
        }

        TVector<TSearchPath> searchBeam;
        DoDecode("{" + srcWord + "}", &LangModel, searchBeam);
        int maxValue = -1;
        double maxWeight = 0.0;
        for (const auto& tsl : searchBeam.back()) {
            int v = tsl.first->Value;
            if (v < 0) {
                continue;
            }
            if ((maxValue < 0) || (maxWeight > tsl.second)) {
                maxValue = tsl.first->Value;
                maxWeight = tsl.second;
            }
        }
        return TDecodeResult(maxValue, maxWeight);
    }

    int TLettersDecode::LoadPhraseModel(IInputStream* inputStream,
                                        bool invertTable,
                                        double maxWeight,
                                        size_t maxSearchWidth)
    {
        MaxSearchWidth = maxSearchWidth;
        TrieNode& root = SrcPhr;
        TPtRecVec& ptRecVec = PhraseModel;
        int line = 0;
        int outline = 0;
        TString entry;
        TPtRecMap ptRecMap;

        while (inputStream->ReadLine(entry)) {
            TVector<TString> tparts;
            StringSplitter(entry).Split('\t').AddTo(&tparts);
            Y_ENSURE(tparts.size() == 3);

            double weight = FromString<double>(tparts[2]);
            double dlength0 = (double)PosUtf8Length(tparts[0]);

            if (weight / (dlength0 + 2.0) < maxWeight) {
                ptRecMap[tparts[0]].push_back(TPtRec(tparts[1], weight));
                outline++;
            }

            if (invertTable) {
                double dlength1 = (double)PosUtf8Length(tparts[1]);
                if (weight / (dlength1 + 2.0) < maxWeight) {
                    ptRecMap[tparts[1]].push_back(TPtRec(tparts[0], weight));
                    outline++;
                }
            }
            line++;
        }
        for (auto& tp : ptRecMap) {
            root.Insert(tp.first, ptRecVec.ysize());
            ptRecVec.push_back(TVector<TPtRec>());
            ptRecVec.back().swap(tp.second);
        }
        return line;
    }
}
