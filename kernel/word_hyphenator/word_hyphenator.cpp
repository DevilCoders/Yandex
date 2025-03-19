#include "word_hyphenator.h"
#include <library/cpp/resource/resource.h>
#include <util/charset/wide.h>
#include <util/generic/list.h>

constexpr bool STRICK_MODE = false;
constexpr double HYPHENATE_DENSITY = 1.4;
constexpr size_t NO_POSITION = (size_t)-1;
THashSet<wchar16> TWordHyphenator::RuByAlphabet = InitializeSet("абвгдеёєжзиійклмнопрстуфхцчшщъыьэюя");
THashSet<wchar16> TWordHyphenator::RuByVowels = InitializeSet("аяоёуўюыиіэеє");
THashSet<wchar16> TWordHyphenator::RuByConsonants = InitializeSet("бвгджзйклмнпрстфхцчшщ");
THashSet<wchar16> TWordHyphenator::RuByVoicedConsonants = InitializeSet("бвгджзйлмнр");
THashSet<wchar16> TWordHyphenator::RuByDeafConsonants = InitializeSet("пфктшсхцчшщ");
THashSet<wchar16> TWordHyphenator::RuJointVowels = InitializeSet("ое");
THashSet<wchar16> TWordHyphenator::RuJointConsonants = InitializeSet("ъь");
TWordHyphenator::TTrie TWordHyphenator::RuWordParts = InitializeTrie();

THashSet<wchar16> TWordHyphenator::InitializeSet(TStringBuf arr) {
    TUtf16String initStr = TUtf16String::FromUtf8(arr);
    THashSet<wchar16> result(initStr.cbegin(), initStr.cend());

    return result;
}

TWordHyphenator::TTrie TWordHyphenator::InitializeTrie()
{
    TTrie result;
    result.Init(TBlob::FromString(NResource::Find("/word_parts_trie")));
    return result;
}

bool TWordHyphenator::HasSymbol(const THashSet<wchar16>& lettersSet, wchar16 symbol) {
    return lettersSet.contains(ToLower(symbol));
}

// if onlyForbidden is true then populate only positions where hyph is forbidden otherwise provide where hyph can be set as well
// we forbid soft hyph 1 letter before and after last letter for prefixes (example подставка по_д-с_тавка) and
// 1 letter before and after first letter in root. Cause after root we can meet ending and it's ok to split word (example прехорошая -> пр_е-х_о-ро-шая, пре - prefix, root хорош)
// prefix and root after it do the same work, but it's usefull for 2 consecutive roots.
inline void SetHyphenRecommendations(bool isRoot, size_t prevPartEnd, size_t curPartEnd, THashMap<size_t, bool>& hyphenRecommendations, bool onlyForbidden) {
    size_t position = isRoot ? prevPartEnd : curPartEnd;
    if (position != NO_POSITION) {
        hyphenRecommendations[position - 1] = false;
        if (!onlyForbidden) {
            hyphenRecommendations[position] = true;
        }
        hyphenRecommendations[position + 1] = false;
    }
}

namespace {

struct TSplit {
    double Weight = 0;
    int PrefixesCount = 0;
    int PrefixesLength = 0;
    bool HasPossibleJointVowel = false;
    bool HasConfirmedJointVowel = false;
    int RootsCount = 0;
    int RootsLength = 0;
    size_t EndOfPreviosPart = NO_POSITION;
    THashMap<size_t, bool> HyphenPositions;

    void AddPart(bool isRoot, size_t curPos, size_t startPos) {
        size_t newPartLen = curPos - (EndOfPreviosPart == NO_POSITION ? startPos : EndOfPreviosPart + 1) + 1;
        SetHyphenRecommendations(isRoot, EndOfPreviosPart, curPos, HyphenPositions, false);
        EndOfPreviosPart = curPos;

        if (isRoot) {
            if (HasPossibleJointVowel) {
                HasConfirmedJointVowel = true;
                ++RootsLength;
                HasPossibleJointVowel = false;
            }
            ++RootsCount;
            RootsLength += newPartLen;
        } else {
            ++PrefixesCount;
            PrefixesLength += newPartLen;
        }

        Weight = CalculateWeight();
    }

    // like синЕзелёный
    void AddJointVowel()
    {
        HasPossibleJointVowel = true;
        HyphenPositions.erase(EndOfPreviosPart - 1);
        HyphenPositions[EndOfPreviosPart] = false;
        HyphenPositions[EndOfPreviosPart + 1] = true;
        HyphenPositions[EndOfPreviosPart + 2] = false;
        ++EndOfPreviosPart;
    }

    // like подЪезд
    void AddJointConsonant()
    {
        if (RootsCount > 0) {
            ++RootsLength;
        } else {
            ++PrefixesLength;
        }
        HyphenPositions[EndOfPreviosPart - 1] = false;
        HyphenPositions[EndOfPreviosPart] = false;
        HyphenPositions[EndOfPreviosPart + 1] = true;
        HyphenPositions[EndOfPreviosPart + 2] = false;
        ++EndOfPreviosPart;
        Weight = CalculateWeight();
    }

    double CalculateWeight() const
    {
        double result = 0;

        int prefixWeight = PrefixesCount == 0 ? 1 : PrefixesCount;
        prefixWeight *= prefixWeight;
        result += ((double) PrefixesLength) / prefixWeight;

        int rootWeight = RootsCount <= 2 ? 1 : RootsCount;
        result += ((double) RootsLength) / rootWeight;

        result += HasConfirmedJointVowel ? 1 : 0;

        return result;
    }
};

bool operator< (const TSplit& split1, const TSplit& split2) {
    return split1.Weight < split2.Weight;
}

}

//result already contains all forbidden places for all possible splits. So if we find more probable split
//than we just swap result with this split
//strick mode determines if it's needed to provide places where soft hyph should be or just places where it shouldn't be
inline void ChooseAppropriateSplit(THashMap<size_t, bool>& result, TVector<TSplit>& splits, int wordLen) {
    double maxWeight = 0;
    TSplit* bestSplit = nullptr;
    int cnt = 0;
    for (TSplit& split : splits) {
        if (split.Weight <= 2 || wordLen - split.PrefixesLength < 2) {
            continue;
        }
        cnt++;

        if (maxWeight < split.Weight) {
            bestSplit = &split;
            maxWeight = split.Weight;
        }
    }
    if (bestSplit != nullptr && (cnt == 1 || cnt > 1 && bestSplit->Weight >= 4) ) {
        if(STRICK_MODE && bestSplit->RootsCount == 0) {
            TVector<size_t> exclude;
            for (const auto& pair :  bestSplit->HyphenPositions) {
                if(pair.second) {
                    exclude.push_back(pair.first);
                }
            }
            for(const auto& item : exclude) {
                bestSplit->HyphenPositions.erase(item);
            }
        }
        result.swap(bestSplit->HyphenPositions);
    } else if (cnt == 0) {
        result.clear();
    }
}

bool TWordHyphenator::RootFound(const TTrie& trie) {
    bool isRoot = trie.Get(nullptr, 0);
    return isRoot;
}

bool TWordHyphenator::WordPartFound(const TTrie& trie) {
    bool found = trie.Find(nullptr, 0);
    return found;
}

// How it works: using our trie of prefixes and roots (which is not full and can miss some) we find all possible splits. Split can contain one or more parts of word (prefix or root).
// Among all splits we choose the one which weight is the biggest and return hyph recommendation of this split. If all splits are not good enough, then
// we just collect only forbidden places for soft hyph in result from all possible splits.
void TWordHyphenator::RuGetHyphenRecommendationsBasedOnMorphAnalysis(const TUtf16String& str, const size_t startPos, THashMap<size_t, bool>& result) {
    TList<std::tuple<TTrie, TSplit>> currentSplitStates;
    TVector<TSplit> splits(1);
    currentSplitStates.emplace_back(RuWordParts, splits[0]);
    int wordLength = 0;

    for (size_t i = startPos; i < str.size() && HasSymbol(RuByAlphabet, str[i]); ++i, ++wordLength) {
        wchar16 symbol = ToLower(str[i]);
        bool canStartNewPart = false;
        TSplit newSplit;

        for (auto curSplitState = currentSplitStates.begin(); curSplitState != currentSplitStates.end(); ) {
            TTrie& trie = std::get<0>(*curSplitState);
            TSplit& curSplit = std::get<1>(*curSplitState);

            //if nodes still exist in tree (we still can find any part of word)
            if (trie.FindTails(symbol, trie)) {
                //if we find root or prefix and we haven't met root in current split
                if (WordPartFound(trie) && (curSplit.RootsCount == 0 || RootFound(trie))) {
                    bool isRoot = RootFound(trie);

                    SetHyphenRecommendations(isRoot, curSplit.EndOfPreviosPart, i, result, true);
                    TSplit& split =  splits.emplace_back(curSplit);
                    split.AddPart(isRoot, i, startPos);
                    //no need to start a lot of parts from here for a lot of dif split. lets choose the most possible split
                    //and start new part from here for this split.
                    canStartNewPart = true;
                    newSplit = std::max(split, newSplit);
                }

                currentSplitStates.emplace_front(trie, curSplit);
            }

            //if we meet joint vowel like 'о' 'е'
            if (curSplit.RootsCount > 0 && curSplit.EndOfPreviosPart == i - 1 && HasSymbol(RuJointVowels, symbol)) {
                canStartNewPart = true;
                TSplit& split = splits.emplace_back(curSplit);
                split.AddJointVowel();
                newSplit = std::max(split, newSplit);
            }

            //if we meet joint consonant like 'ъ' 'ь'
            if (curSplit.EndOfPreviosPart == i - 1 && HasSymbol(RuJointConsonants, symbol)) {
                canStartNewPart = true;
                TSplit& split = splits.emplace_back(curSplit);
                split.AddJointConsonant();
                newSplit = std::max(split, newSplit);
            }

            currentSplitStates.erase(curSplitState++);
        }

        if (canStartNewPart) {
            currentSplitStates.emplace_front(RuWordParts, newSplit);
        }
    }

    ChooseAppropriateSplit(result, splits, wordLength);
}

// Algorithm is based on 2 approaches:
//    1 - morphological analysis. Part which is responsible for recommendations where soft hyph can be set and where it can't be set. These
//        recommendations are based on splitting word on parts like prefix and root. Place between parts seems to be good place fo rsoft hyph.
//        Moving 1 letter from root to prefix and vice versa is not good idea.
//    2 - Other places fall under set of simple rules like: 2 the same consonants can be separated with soft hyph, soft hyph is set before last consonant
//        among 2 and more consecutive consonants and so on.
// return true if word has been processed otherwise false is returned
bool TWordHyphenator::RuByHyphenate(const TUtf16String& str, size_t& curPos, TUtf16String& hyphenatedStr, const THyphenateParams& params) {
    bool processed = false;
    bool metVowel = false;
    bool metConsonant = false;
    bool firstPart = true;
    size_t startPos = curPos;
    bool isAbbreviation = false;
    THashMap<size_t, bool> hyphenRecommendations;
    RuGetHyphenRecommendationsBasedOnMorphAnalysis(str, curPos, hyphenRecommendations);
    TUtf16String strWithoutNewHyphenation = hyphenatedStr;

    for (; curPos < str.size(); ++curPos) {
        wchar16 symbol = str[curPos];

        if (!HasSymbol(RuByAlphabet,symbol)) {
            break;
        }

        hyphenatedStr.append(symbol);
        strWithoutNewHyphenation.append(symbol);

        if (curPos + 2 >= str.size()) {
            continue;
        }

        wchar16 nextSymbol = str[curPos + 1];
        wchar16 afterNextSymbol = str[curPos + 2];

        if (startPos != curPos && (symbol == ToUpper(symbol) || nextSymbol == ToUpper(nextSymbol))) {
            isAbbreviation = true;
        }

        if (isAbbreviation) {
            continue;
        }

        processed = true;
        metVowel = metVowel || HasSymbol(RuByVowels, symbol);
        metConsonant = metConsonant || HasSymbol(RuByConsonants, symbol);

        if (!HasSymbol(RuByAlphabet, nextSymbol) || !HasSymbol(RuByAlphabet, afterNextSymbol)){
            continue;
        }

        if (!metVowel || !metConsonant) {
            continue;
        }

        auto curPosIt = hyphenRecommendations.find(curPos);
        if (curPosIt != hyphenRecommendations.end()) {
            if (curPosIt->second) {
                if (curPos - startPos + 1 > params.MinSymbolsBeforeHyphenation) {
                    hyphenatedStr.append(SOFT_HYPHEN);
                }
                metVowel = false;
                metConsonant = false;
                firstPart = true;
            }

            continue;
        }

        // example обЪ-ЯВление или парА-ОЛимпийский
        bool notConsonantVowelConsonant = !HasSymbol(RuByConsonants, symbol) && HasSymbol(RuByVowels, nextSymbol) && HasSymbol(RuByConsonants, afterNextSymbol);
        // example множествеН-Ный
        bool sameConsonants = HasSymbol(RuByConsonants, symbol) && symbol == nextSymbol;
        // example ма-МА
        bool nextConsonantVowel = HasSymbol(RuByConsonants, nextSymbol) && HasSymbol(RuByVowels, afterNextSymbol);
        // example каса-ТЬСЯ
        bool nextConsonantJointConsonantVowel = HasSymbol(RuByConsonants, nextSymbol) && HasSymbol(RuJointConsonants, afterNextSymbol) && curPos + 3 < str.size() && HasSymbol(RuByVowels, str[curPos + 3]);
        bool voicedDeafVoiced = firstPart && HasSymbol(RuByVoicedConsonants, symbol) && HasSymbol(RuByDeafConsonants, nextSymbol) && HasSymbol(RuByVoicedConsonants, afterNextSymbol);

        if (notConsonantVowelConsonant ||
            sameConsonants ||
            nextConsonantVowel ||
            nextConsonantJointConsonantVowel ||
            voicedDeafVoiced)
        {
            if (curPos - startPos + 1 > params.MinSymbolsBeforeHyphenation) {
                hyphenatedStr.append(SOFT_HYPHEN);
            }
            firstPart = false;
            metVowel = false;
            metConsonant = false;
        }
    }

    if (curPos - startPos + 1 < params.MinWordLengthForHyphenation) {
        hyphenatedStr = strWithoutNewHyphenation;
    }
    return processed;
}

TUtf16String TWordHyphenator::Hyphenate(const TUtf16String& str, const THyphenateParams& params) const {
    TUtf16String result;
    size_t originalSize = str.size();
    result.reserve((size_t)(HYPHENATE_DENSITY * originalSize));
    size_t curPos = 0;
    while (curPos < str.size()) {
        bool handled = true;
        while (handled) {
            handled = false;
            for (const HyphenateHandler& handler : HyphenateHandlers) {
                handled |= handler(str, curPos, result, params);
            }
        }

        if (curPos < str.size()) {
            result.append(str[curPos++]);
        }
    }
    return result;
}

TString TWordHyphenator::Hyphenate(const TString& str, const THyphenateParams& params) const {
    return WideToUTF8(Hyphenate(UTF8ToWide(str), params));
}
