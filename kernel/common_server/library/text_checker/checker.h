#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/generic/vector.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/map.h>
#include <util/string/split.h>
#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <library/cpp/charset/wide.h>
#include <util/generic/set.h>
#include <kernel/common_server/library/scheme/scheme.h>

namespace NCS {
    namespace NTextChecker {
        class TRequestFeatures {
        private:
            CS_ACCESS(TRequestFeatures, ui32, UnorderedWordsMax, 0);
            CS_ACCESS(TRequestFeatures, ui32, NearWordsDistanceMax, 0);
            CS_ACCESS(TRequestFeatures, ui32, MisspellsCountMax, 0);
            CS_ACCESS(TRequestFeatures, bool, UseWholeTextSearch, false);
        public:
            static NCS::NScheme::TScheme GetScheme();

            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                result.InsertValue("use_whole_word_search", UseWholeTextSearch);
                result.InsertValue("misspells_count_max", MisspellsCountMax);
                result.InsertValue("unordered_words_max", UnorderedWordsMax);
                result.InsertValue("near_words_distance_max", NearWordsDistanceMax);
                return result;
            }

            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        };

        template <class T>
        TVector<ui32> InitLongestSubsequenceGrowing(const TVector<T>& sequence) {
            TVector<T> d;
            TVector<i32> pos;
            TVector<i32> prev;
            d.resize(sequence.size() + 1, T::Max());
            d.front() = T::Min();
            pos.resize(sequence.size() + 1, -1);
            prev.resize(sequence.size() + 1, -1);
            ui32 length = 0;
            for (ui32 i = 0; i < sequence.size(); ++i) {
                auto it = std::upper_bound(d.begin(), d.end(), sequence[i]);
                CHECK_WITH_LOG(it != d.end());
                if (sequence[i] == *(it - 1)) {
                    continue;
                }
                const ui32 currentLength = it - d.begin();
                d[currentLength] = sequence[i];
                pos[currentLength] = i;
                prev[i] = pos[currentLength - 1];
                if (length < currentLength) {
                    length = currentLength;
                }
            }
            TVector<ui32> result;
            result.reserve(length);
            i32 idx = pos[length];
            while (idx != -1 && result.size() < length) {
                result.emplace_back(idx);
                idx = prev[idx];
            }
            std::reverse(result.begin(), result.end());
            return result;
        }

        class TPosition {
        private:
            CS_ACCESS(TPosition, ui32, DocNumber, 0);
            CS_ACCESS(TPosition, ui32, WordsCount, 0);

            CS_ACCESS(TPosition, ui32, WordNumber, 0);
            CS_ACCESS(TPosition, ui32, WordSize, 0);

            CS_ACCESS(TPosition, ui32, NGrammIdx, 0);
            CS_ACCESS(TPosition, ui32, NGrammsCount, 0);
        public:
            bool operator!() const {
                return !WordSize && !NGrammsCount && !WordsCount;
            }

            bool operator<(const TPosition& item) const {
                return std::tie(DocNumber, WordNumber, NGrammIdx) > std::tie(item.DocNumber, item.WordNumber, item.NGrammIdx);
            }
            bool IsWholeWord() const {
                return NGrammsCount == 1;
            }
        };

        class TCompositePosition {
            CS_ACCESS(TCompositePosition, i32, RequestNGrammIdx, 0);
            CSA_DEFAULT(TCompositePosition, TPosition, OriginalWordPosition);
        public:
            TCompositePosition() = default;

            bool operator==(const TCompositePosition& wpc) const {
                return RequestNGrammIdx == wpc.RequestNGrammIdx;
            }

            bool operator<(const TCompositePosition& item) const {
                return RequestNGrammIdx < item.RequestNGrammIdx;
            }

            static TCompositePosition Max() {
                return TCompositePosition().SetRequestNGrammIdx(::Max<i32>());
            }

            static TCompositePosition Min() {
                return TCompositePosition().SetRequestNGrammIdx(::Min<i32>());
            }
        };

        class TPositionMatchInfo {
        private:
            CS_ACCESS(TPositionMatchInfo, ui32, DocNumber, Max<ui32>());
            CS_ACCESS(TPositionMatchInfo, ui32, WordNumber, 0);
            CS_ACCESS(TPositionMatchInfo, ui32, WordSize, 0);
            CS_ACCESS(TPositionMatchInfo, ui32, WordsCount, 0);
            CSA_MUTABLE_DEF(TPositionMatchInfo, TVector<TCompositePosition>, ExpectedIdxs);
            CS_ACCESS(TPositionMatchInfo, ui32, ReqWordNGrammsCount, 0);
        public:
            TPositionMatchInfo() = default;

            TPositionMatchInfo(const TPosition& pos, const ui32 reqWordNGrammsCount)
                : DocNumber(pos.GetDocNumber())
                , WordNumber(pos.GetWordNumber())
                , WordSize(pos.GetWordSize())
                , WordsCount(pos.GetWordsCount())
                , ReqWordNGrammsCount(reqWordNGrammsCount)
            {

            }

            bool operator<(const TPositionMatchInfo& item) const {
                return std::tie(DocNumber, WordNumber) > std::tie(item.DocNumber, item.WordNumber);
            }

            bool IsValid() const {
                return DocNumber != Max<ui32>();
            }

            bool IsMatch(const TRequestFeatures& rFeatures, const ui32 nGrammSize, const bool wordsSplitting) const {
                if (!IsValid() || ExpectedIdxs.empty()) {
                    return false;
                }
                {
                    TVector<ui32> selection = InitLongestSubsequenceGrowing(ExpectedIdxs);
                    TVector<TCompositePosition> cVector;
                    for (auto&& i : selection) {
                        if (cVector.empty() || ExpectedIdxs[i].GetOriginalWordPosition() < cVector.back().GetOriginalWordPosition()) {
                            cVector.emplace_back(ExpectedIdxs[i]);
                        }
                    }
                    std::swap(ExpectedIdxs, cVector);
                }

                TMap<i32, i32> charIdx;
                i32 nReqWordSize = ReqWordNGrammsCount - nGrammSize + 1;
                for (ui32 i = 0; i < ExpectedIdxs.size(); ++i) {
                    for (i32 iChar = 0; iChar < (i32)nGrammSize; ++iChar) {
                        const TCompositePosition& cp = ExpectedIdxs[i];
                        if (cp.GetRequestNGrammIdx() - iChar >= 0 && cp.GetRequestNGrammIdx() - iChar < nReqWordSize) {
                            if ((i32)cp.GetOriginalWordPosition().GetNGrammIdx() - iChar >= 0 && (i32)cp.GetOriginalWordPosition().GetNGrammIdx() - iChar < (i32)WordSize) {
                                charIdx.emplace((i32)cp.GetRequestNGrammIdx() - iChar, cp.GetOriginalWordPosition().GetNGrammIdx() - iChar);
                            }
                        }
                    }
                }
                if (wordsSplitting) {
                    return (Abs<i32>((i32)charIdx.size() - (i32)WordSize) <= (i32)rFeatures.GetMisspellsCountMax()) &&
                           (Abs<i32>((i32)charIdx.size() - (i32)(ReqWordNGrammsCount - nGrammSize + 1)) <= (i32)rFeatures.GetMisspellsCountMax());
                } else {
                    TMaybe<i32> predIdxSecond;
                    TMaybe<i32> predIdxFirst;
                    ui32 incorrectCharacters = 0;
                    for (auto&& i : charIdx) {
                        if (!!predIdxSecond) {
                            if (i.second == *predIdxSecond) {
                                incorrectCharacters += i.first - *predIdxFirst;
                            } else {
                                incorrectCharacters += Max(i.second - 1 - *predIdxSecond, i.first - 1 - *predIdxFirst);
                            }
                        } else {
                            incorrectCharacters += i.second;
                        }
                        predIdxSecond = i.second;
                        predIdxFirst = i.first;
                    }
                    if (predIdxSecond) {
                        incorrectCharacters += (WordSize - 1 - *predIdxSecond);
                    } else {
                        return false;
                    }
                    return incorrectCharacters <= rFeatures.GetMisspellsCountMax();
                }
            }

            bool TakePosition(const TPosition& pos, const ui32 requestWordNGrammIdx) {
                TCompositePosition cp;
                cp.SetOriginalWordPosition(pos).SetRequestNGrammIdx(requestWordNGrammIdx);
                ExpectedIdxs.emplace_back(std::move(cp));
                return true;
            }

            bool SameWord(const TPosition& pos) const {
                return DocNumber == pos.GetDocNumber() && WordNumber == pos.GetWordNumber();
            }
        };

        class TWordPositionComposite {
        private:
            CS_ACCESS(TWordPositionComposite, ui32, OriginalWordPosition, 0);
            CSA_DEFAULT(TWordPositionComposite, TVector<ui32>, ReqWordPositions);
        };

        class TLineMatchInfo {
        private:
            CS_ACCESS(TLineMatchInfo, ui32, DocNumber, Max<ui32>());
            CSA_MUTABLE_DEF(TLineMatchInfo, TVector<TWordPositionComposite>, ExpectedIdxs);
            CSA_DEFAULT(TLineMatchInfo, TSet<ui32>, WordPositions);
            CSA_DEFAULT(TLineMatchInfo, TSet<ui32>, RequestWordIndexes);
            CS_ACCESS(TLineMatchInfo, ui32, LineWordsCount, 0);
        public:
            TLineMatchInfo() = default;

            TLineMatchInfo(const TPositionMatchInfo& pos)
                : DocNumber(pos.GetDocNumber())
                , LineWordsCount(pos.GetWordsCount())
            {

            }

            bool IsValid() const {
                return DocNumber != Max<ui32>();
            }

            bool IsMatch(const TRequestFeatures& /*rFeatures*/) const {
                if (!IsValid()) {
                    return false;
                }

                TSet<ui32> wIndexes;
                for (ui32 i = 0; i < ExpectedIdxs.size(); ++i) {
                    for (auto&& w : ExpectedIdxs[i].GetReqWordPositions()) {
                        if (wIndexes.emplace(w).second) {
                            break;
                        }
                    }
                }

                if (wIndexes.size() >= LineWordsCount) {
                    return true;
                }
                return false;
            }

            bool TakePosition(const TPositionMatchInfo& pos, const ui32 requestWordIndex) {
                if (ExpectedIdxs.empty() || ExpectedIdxs.back().GetOriginalWordPosition() != pos.GetWordNumber()) {
                    TWordPositionComposite wpc;
                    wpc.SetOriginalWordPosition(pos.GetWordNumber()).SetReqWordPositions({ requestWordIndex });
                    ExpectedIdxs.emplace_back(std::move(wpc));
                } else {
                    ExpectedIdxs.back().MutableReqWordPositions().emplace_back(requestWordIndex);
                }
                return true;
            }

            bool SameLine(const TPositionMatchInfo& pos) const {
                return DocNumber == pos.GetDocNumber();
            }
        };

        class TWordsIterator {
        private:
            TVector<TPositionMatchInfo>::const_iterator CurrentIterator;
            TVector<TPositionMatchInfo>::const_iterator Finish;
            CSA_READONLY(ui32, RequestWordIdx, 0);
        public:
            TWordsIterator(const TVector<TPositionMatchInfo>& v, const ui32 wordIdx)
                : CurrentIterator(v.begin())
                , Finish(v.end())
                , RequestWordIdx(wordIdx)
            {

            }

            bool operator<(const TWordsIterator& item) const {
                if (*CurrentIterator < *item.CurrentIterator) {
                    return true;
                } else if (*item.CurrentIterator < *CurrentIterator) {
                    return false;
                } else {
                    return RequestWordIdx > item.RequestWordIdx;
                }
            }

            bool IsValid() const {
                return CurrentIterator != Finish;
            }
            const TPositionMatchInfo& GetPosition() const {
                ASSERT_WITH_LOG(IsValid());
                return *CurrentIterator;
            }

            bool Next() {
                return (++CurrentIterator) != Finish;
            }
        };

        class TNGrammsIterator {
        private:
            TVector<TPosition>::const_iterator CurrentIterator;
            TVector<TPosition>::const_iterator Finish;
            CSA_READONLY(ui32, CurrentNGrammIdx, 0);
        public:
            TNGrammsIterator(const TVector<TPosition>& positions, const ui32 currentNGrammIdx)
                : CurrentIterator(positions.begin())
                , Finish(positions.end())
                , CurrentNGrammIdx(currentNGrammIdx)
            {

            }

            bool operator<(const TNGrammsIterator& item) const {
                if (*CurrentIterator < *item.CurrentIterator) {
                    return true;
                } else if (*item.CurrentIterator < *CurrentIterator) {
                    return false;
                } else {
                    return CurrentNGrammIdx > item.CurrentNGrammIdx;
                }
            }

            bool IsValid() const {
                return CurrentIterator != Finish;
            }
            const TPosition& GetPosition() const {
                ASSERT_WITH_LOG(IsValid());
                return *CurrentIterator;
            }

            bool Next() {
                return (++CurrentIterator) != Finish;
            }
        };

        class TTextChecker: public TNonCopyable {
        private:
            TMap<TUtf16String, TVector<TPosition>> KeyInv;
            TVector<TUtf16String> OriginalLines;
            const ui32 NGrammBase = 3;
            const bool SplitWords = true;
            const bool UseNormalization = false;
            const ui32 MinWordSize = 2;
            const TUtf16String DelimitersSet;
            static const TUtf16String SpaceChar;

            TVector<TUtf16String> BuildNGramms(const TWtringBuf w) const;

            TUtf16String Normalize(const TString& line) const;
        public:
            TTextChecker(const ui32 nGrammBase, const bool splitWords, const TString& delimitersSet, const bool useNormalization = false, const ui32 minWordSize = 2)
                : NGrammBase(nGrammBase)
                , SplitWords(splitWords)
                , UseNormalization(useNormalization)
                , MinWordSize(minWordSize)
                , DelimitersSet(UTF8ToWide(delimitersSet))
            {

            }

            void Compile(const TVector<TString>& originalLines) {
                for (auto&& line : originalLines) {
                    TUtf16String nLine = Normalize(line);
                    if (!nLine) {
                        continue;
                    }
                    OriginalLines.emplace_back(std::move(nLine));
                }

                ui32 lineIdx = 0;
                for (auto&& line : OriginalLines) {
                    TWtringBuf text(line.data(), line.size());
                    TWtringBuf l;
                    TWtringBuf r;
                    if (!text.TrySplit('\n', l, r)) {
                        l = text;
                    }
                    if (l.empty()) {
                        text = r;
                        continue;
                    }
                    ui32 wNumber = 0;
                    TVector<TWtringBuf> words;
                    if (SplitWords) {
                        words = StringSplitter(l).SplitBySet(DelimitersSet.data()).SkipEmpty().ToList<TWtringBuf>();
                    } else {
                        words.emplace_back(l);
                    }
                    TPosition pos;
                    pos.SetDocNumber(lineIdx++).SetWordsCount(words.size());
                    for (auto&& w : words) {
                        pos.SetWordNumber(wNumber++).SetWordSize(w.size());
                        const TVector<TUtf16String> nGramms = BuildNGramms(w);
                        ui32 idx = 0;
                        for (auto&& i : nGramms) {
                            pos.SetNGrammsCount(nGramms.size());
                            pos.SetNGrammIdx(idx++);
                            KeyInv[i].emplace_back(pos);
                        }
                    }
                    text = r;
                }
            }

            bool CheckSubstrings(const TString& baseStringExt, const TRequestFeatures& rFeatures = Default<TRequestFeatures>(), TString* matchResult = nullptr) const {
                const TUtf16String baseString = Normalize(baseStringExt);
                TVector<TVector<TPositionMatchInfo>> wordPositions;
                TVector<TWtringBuf> reqWords;
                if (SplitWords) {
                    reqWords = StringSplitter(baseString).SplitBySet(DelimitersSet.data()).SkipEmpty().ToList<TWtringBuf>();
                } else {
                    if (rFeatures.GetUseWholeTextSearch()) {
                        if (KeyInv.contains(baseString)) {
                            if (matchResult) {
                                *matchResult = baseStringExt;
                            }
                            return true;
                        }
                    }
                    reqWords.emplace_back(baseString.data());
                }
                for (auto&& wl : reqWords) {
                    wordPositions.emplace_back(TVector<TPositionMatchInfo>());
                    TVector<TPositionMatchInfo>& wPositions = wordPositions.back();
                    TVector<TNGrammsIterator> itGramms;
                    const TVector<TUtf16String> nGramms = BuildNGramms(wl);
                    ui32 idx = 0;
                    for (auto&& i : nGramms) {
                        auto it = KeyInv.find(i);
                        if (it == KeyInv.end()) {
                            idx++;
                            continue;
                        }
                        TNGrammsIterator ngIterator(it->second, idx++);
                        if (ngIterator.IsValid()) {
                            itGramms.emplace_back(std::move(ngIterator));
                        }
                    }
                    MakeHeap(itGramms.begin(), itGramms.end());
                    const ui32 nGrammsCount = nGramms.size();
                    TPositionMatchInfo matchInfo;
                    while (itGramms.size()) {
                        const TPosition& currentMin = itGramms.front().GetPosition();
                        if (!matchInfo.SameWord(currentMin)) {
                            if (matchInfo.IsMatch(rFeatures, NGrammBase, SplitWords)) {
                                wPositions.emplace_back(matchInfo);
                            }
                            matchInfo = TPositionMatchInfo(currentMin, nGrammsCount);
                        }
                        matchInfo.TakePosition(currentMin, itGramms.front().GetCurrentNGrammIdx());
                        PopHeap(itGramms.begin(), itGramms.end());
                        if (itGramms.back().Next()) {
                            PushHeap(itGramms.begin(), itGramms.end());
                        } else {
                            itGramms.pop_back();
                        }
                    }
                    if (matchInfo.IsMatch(rFeatures, NGrammBase, SplitWords)) {
                        wPositions.emplace_back(matchInfo);
                    }
                }

                TVector<TLineMatchInfo> LinesMatch;
                TVector<TWordsIterator> itWords;
                {
                    ui32 idx = 0;
                    for (auto&& i : wordPositions) {
                        if (i.size()) {
                            itWords.emplace_back(TWordsIterator(i, idx));
                        }
                        ++idx;
                    }
                }
                MakeHeap(itWords.begin(), itWords.end());
                TLineMatchInfo lineMatchInfo;
                while (itWords.size()) {
                    const TPositionMatchInfo& positionsMatching = itWords.front().GetPosition();
                    if (!lineMatchInfo.SameLine(itWords.front().GetPosition())) {
                        if (lineMatchInfo.IsMatch(rFeatures)) {
                            if (matchResult) {
                                *matchResult = WideToUTF8(OriginalLines[lineMatchInfo.GetDocNumber()]);
                                return true;
                            }
                            LinesMatch.emplace_back(lineMatchInfo);
                        }
                        lineMatchInfo = TLineMatchInfo(positionsMatching);
                    }
                    lineMatchInfo.TakePosition(positionsMatching, itWords.front().GetRequestWordIdx());
                    PopHeap(itWords.begin(), itWords.end());
                    if (itWords.back().Next()) {
                        PushHeap(itWords.begin(), itWords.end());
                    } else {
                        itWords.pop_back();
                    }
                }
                if (lineMatchInfo.IsMatch(rFeatures)) {
                    if (matchResult) {
                        *matchResult = WideToUTF8(OriginalLines[lineMatchInfo.GetDocNumber()]);
                        return true;
                    }
                    LinesMatch.emplace_back(lineMatchInfo);
                }
                return LinesMatch.size();
            }
        };
    }
}
