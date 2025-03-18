#pragma once

#include "version.h"
#include "ngrammers.h"
#include "vectorizers.h"
#include "algorithms.h"
#include "randoms.h"
#include "simhash_calculator.h"
#include "common.h"
#include "stopwords.h"

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/numerator/numerate.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/charset/wide.h>
#include <util/digest/city.h>
#include <utility>
#include <library/cpp/digest/md5/md5.h>
#include <util/generic/ymath.h>
#include <util/stream/output.h>

class TSimhashHash: private TNonCopyable {
public:
    void Reset() {
        WordHashIndexMap.clear();
        WordStorage.clear();
        WordCount.clear();
        BigramCount.clear();
        Words.clear();
        SimhashHash = 0;
    }

    void OnBegin() {
        static const TUtf16String s = u"^";
        OnWord(s);
    }

    void OnWord(const TUtf16String& word) {
        size_t wordIndex = GetWordIndex(word);
        UpdateContent(wordIndex);
    }

    void OnEnd() {
        static const TUtf16String s = u"&";
        OnWord(s);
        UpdateSimhashHash();
    }

    ui64 GetSimhashHash() const {
        return SimhashHash;
    }

private:
    bool IsNumber(const TUtf16String& s) noexcept {
        if (s.empty())
            return false;

        for (size_t i = 0; i < s.length(); ++i) {
            if (s[i] < L'0' || s[i] > L'9')
                return false;
        }
        return true;
    }

    size_t GetWordIndex(const TUtf16String& word) {
        size_t hash = ComputeHash(word);
        if (WordHashIndexMap.contains(hash)) {
            return WordHashIndexMap[hash];
        } else {
            size_t index = WordStorage.size();
            WordStorage.push_back(word);
            WordHashIndexMap[hash] = index;
            return index;
        }
    }

    void UpdateContent(size_t index) {
        Words.push_back(index);
        if (index < WordCount.size()) {
            ++WordCount[index];
        } else if (index == WordCount.size()) {
            WordCount.push_back(1);
        } else {
            ythrow yexception() << "Wrong internal state. Your program is wrong";
        }
        if (Words.size() >= 2) {
            BigramCount[std::make_pair(Words[Words.size() - 2], index)] += 1;
        }
    }

    void UpdateSimhashHash() {
        if (SimhashHash != 0) {
            return;
        }
        TVector<std::pair<std::pair<size_t, size_t>, ui32>> bigrams(BigramCount.begin(), BigramCount.end());
        Sort(bigrams.begin(), bigrams.end(), TCompareStringPairs<true>(WordStorage, WordCount));
        size_t startIndex = 0;
        {
            ui32 rankSum = 0;
            ui32 rank = 1;
            ui32 prevCount = 0;
            for (size_t i = 0; i < bigrams.size(); ++i) {
                if (i > 0) {
                    if (bigrams[i].second != prevCount) {
                        ++rank;
                        prevCount = bigrams[i].second;
                    }
                } else {
                    prevCount = bigrams[i].second;
                }
                bigrams[i].second = rank;
                rankSum += rank;
            }
            ui32 neededRank = rankSum - SmartFloor(rankSum);
            if (neededRank > rankSum) {
                neededRank = 0;
            }
            ui32 partialRankSum = 0;
            for (size_t i = 0; i < bigrams.size(); ++i) {
                partialRankSum += bigrams[i].second;
                if (partialRankSum >= neededRank) {
                    startIndex = i;
                    break;
                }
            }
        }
        {
            MD5 md5;
            md5.Init();
            for (size_t i = startIndex; i < bigrams.size(); ++i) {
                //Cerr << bigrams[i].second << ": ";
                for (ui32 j = 0; j < bigrams[i].second; ++j) {
                    md5.Update(WordStorage[bigrams[i].first.first].data(),
                               WordStorage[bigrams[i].first.first].length() * sizeof(wchar16));
                    md5.Update(" ", 1);

                    md5.Update(WordStorage[bigrams[i].first.second].data(),
                               WordStorage[bigrams[i].first.second].length() * sizeof(wchar16));
                    md5.Update(" ", 1);

                    if (j == 0) {
                        //Cerr << "[" << WordStorage[bigrams[i].first.first] << " " <<
                        //        WordStorage[bigrams[i].first.second] << "] ";
                    }
                }
                //Cerr << Endl;
            }
            //Cerr << Endl << Endl;
            md5.Final(signature);
            SimhashHash = CityHash64((const char*)signature, 16);
        }
    }

    static double Log(double x, double base) {
        return log(x) / log(base);
    }

    static ui32 SmartFloor(ui32 x) {
        if (x <= 9) {
            return x;
        }
        static double e = 1.0 / 0.9;
        return (ui32)ceil(pow(e, floor(Log(x, e))));
    }

    template <bool LengthFirst>
    struct TCompareStringPairs {
        TCompareStringPairs(
            TVector<TUtf16String>& wordStorage,
            TVector<ui32>& wordCount)
            : WordStorage(wordStorage)
            , WordCount(wordCount)
        {
        }

        bool operator()(const std::pair<std::pair<size_t, size_t>, ui32>& left,
                        const std::pair<std::pair<size_t, size_t>, ui32>& right) {
            bool LeftIsBad = (WordStorage[left.first.first].length() +
                                  WordStorage[left.first.second].length() ==
                              2);
            bool RightIsBad = (WordStorage[right.first.first].length() +
                                   WordStorage[right.first.second].length() ==
                               2);

            if (LeftIsBad && !RightIsBad) {
                return true;
            }
            if (!LeftIsBad && RightIsBad) {
                return false;
            }

            if (left.second < right.second) {
                return true;
            } else if (left.second > right.second) {
                return false;
            } else {
                if (LengthFirst) {
                    const size_t lenLeft = WordStorage[left.first.first].length() +
                                           WordStorage[left.first.second].length();
                    const size_t lenRight = WordStorage[right.first.first].length() +
                                            WordStorage[right.first.second].length();
                    if (lenLeft < lenRight) {
                        return true;
                    } else if (lenLeft > lenRight) {
                        return false;
                    } else {
                        if (Min(WordCount[left.first.first],
                                WordCount[left.first.second]) <
                            Min(WordCount[right.first.first],
                                WordCount[right.first.second]))
                        {
                            return true;
                        } else if (Min(WordCount[left.first.first],
                                       WordCount[left.first.second]) >
                                   Min(WordCount[right.first.first],
                                       WordCount[right.first.second]))
 {
                            return false;
                        } else {
                            return left.first < right.first;
                        }
                    }
                } else {
                    if (Min(WordCount[left.first.first],
                            WordCount[left.first.second]) <
                        Min(WordCount[right.first.first],
                            WordCount[right.first.second]))
                    {
                        return true;
                    } else if (Min(WordCount[left.first.first],
                                   WordCount[left.first.second]) >
                               Min(WordCount[right.first.first],
                                   WordCount[right.first.second]))
 {
                        return false;
                    } else {
                        const size_t lenLeft = WordStorage[left.first.first].length() +
                                               WordStorage[left.first.second].length();
                        const size_t lenRight = WordStorage[right.first.first].length() +
                                                WordStorage[right.first.second].length();
                        if (lenLeft < lenRight) {
                            return true;
                        } else if (lenLeft > lenRight) {
                            return false;
                        } else {
                            return left.first < right.first;
                        }
                    }
                }
            }
        }

        TVector<TUtf16String>& WordStorage;
        TVector<ui32>& WordCount;
    };

private:
    THashMap<size_t, size_t> WordHashIndexMap;
    TVector<TUtf16String> WordStorage;
    TVector<ui32> WordCount;
    THashMap<std::pair<size_t, size_t>, ui32> BigramCount;
    TVector<size_t> Words;
    ui64 SimhashHash;
    unsigned char signature[16];
};

class TSimHashNumeratorHandler: public INumeratorHandler, private TNonCopyable {
private:
    typedef TSimhashCalculator<ui64, TBigrammer, TDummyVectorizer4, TInnerProductSimhash> TSimHash;

public:
    TSimHashNumeratorHandler()
        : SimhashVersion()
        , SimHashBuilder()
        , DocLength(0)
        , TitleHash(0)
        , InTitle(false)
        , Word()
    {
        SimhashVersion.NGrammVersion = TBigrammer<ui64>::GetVersion();
        SimhashVersion.VectorizerVersion = TDummyVectorizer4<ui64>::GetVersion();
        SimhashVersion.MethodVersion = TInnerProductSimhash::GetVersion();
        SimhashVersion.RandomVersion = 61;

        SimHashBuilder.Reset(new TSimHash(SimhashVersion));

        Reset();
    }

    void Reset() {
        SimHashBuilder->Input().OnStart();
        DocLength = 0;
        TitleHash = 0 ^ 0xFFFFFFFF;
        InTitle = false;
        Word.clear();
    }

    void OnWord(const TChar* word, size_t len) {
        Word.assign(word, len);
        Word.to_lower();
        if (!IsStopWord(Word)) {
            SimHashBuilder->Input().OnValue(WordCrc());
            ++DocLength;
        }
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat& /*stat*/) override {
        if (DetectNLPType(tok.SubTokens) == NLP_WORD) {
            for (const auto& sub : tok.SubTokens) {
                OnWord(tok.Token + sub.Pos, sub.Len);
            }
        } else {
            OnWord(tok.Token, tok.Leng);
        }
    }

    void OnTextEnd(const IParsedDocProperties* ps, const TNumerStat& /*stat*/) override {
        const char* prop = nullptr;
        if (ps) {
            ps->GetProperty("_ogtitle", &prop);
        }

        if (prop) {
            size_t len = strlen(prop);
            TCharTemp tempBuf(len);
            unsigned lenbuf = HtEntDecodeToChar(ps->GetCharset(), prop, len, tempBuf.Data());
            Word.assign(tempBuf.Data(), (size_t)lenbuf);
            Word.to_lower();

            UpdateTitleCrc((const char*)Word.data(), Word.size() * sizeof(wchar16));
            static const char line_end = '\n';
            UpdateTitleCrc(&line_end, 1);
        }
    }

    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat& /*stat*/) override {
        if (zone && !zone->OnlyAttrs && zone->Name) {
            if (zone->Name == TStringBuf{"title"}) {
                InTitle = zone->IsOpen;
            }
        }
    }

    ui32 GetVersion() const {
        return MakeVersion(SimHashBuilder->GetVersion());
    }

    ui64 GetSimhash() {
        SimHashBuilder->Input().OnEnd();

        SimHashBuilder->Calculate();

        return SimhashFromVector<ui64>(SimHashBuilder->Output());
    }

    ui32 GetDocLength() const {
        return DocLength;
    }

    ui32 GetTitleHash() const {
        return TitleHash ^ 0xFFFFFFFF;
    }

    const TVector<i32>& GetRawVector() const {
        return SimHashBuilder->VectorizerOutput();
    }

    ui32 GetVectorizerOutputSize() const {
        return SimHashBuilder->GetVectorizerOutputSize();
    }

private:
    inline ui64 WordCrc() {
        if (InTitle) {
            UpdateTitleCrc((const char*)Word.data(), Word.size() * sizeof(wchar16));
            static const char line_end = '\n';
            UpdateTitleCrc(&line_end, 1);
        }
        return CityHash64WithSeed((const char*)Word.data(), Word.size() * sizeof(wchar16), 0);
    }

    inline void UpdateTitleCrc(const char* chars, ui32 length) {
        extern const ui32* crctab32;
        for (ui32 i = 0; i < length; ++i) {
            TitleHash = (TitleHash >> 8) ^ crctab32[(ui8)((TitleHash ^ chars[i]) & 0xFF)];
        }
    }

private:
    TSimHashVersion SimhashVersion;
    THolder<TSimHash> SimHashBuilder;

    ui32 DocLength;
    ui32 TitleHash;

    bool InTitle;
    TUtf16String Word;
};
