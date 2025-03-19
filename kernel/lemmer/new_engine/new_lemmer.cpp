#include "new_lemmer.h"
#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <util/generic/algorithm.h>
#include <util/generic/ylimits.h>
#include <util/memory/blob.h>

using TRecognizeOpt = NLemmer::TRecognizeOpt;
using TDiacriticsMap = NLemmer::TDiacriticsMap;

namespace {
    inline bool AnasWeightComparer(const TYandexLemma& x, const TYandexLemma& y) {
        if (x.GetWeight() == y.GetWeight()) {
            return NLemmerAux::GetRuleId(x) < NLemmerAux::GetRuleId(y);
        }
        return x.GetWeight() > y.GetWeight();
    }

    static bool HasFlexWithGrammar(const TBinaryGrammarString& neededGram, const IScheme& scheme, const IBlock& block) {
        if (!neededGram.IsValid())
            return true;

        using NTGrammarProcessing::HasGram;
        using NTGrammarProcessing::ch2tg;

        bool notStemGram[256];
        TBinaryGrammarString stemGram = scheme.GetGrammarString();
        int i = 0;
        for (TGrammarIterator it = neededGram.begin(); it.IsValid(); it.Next()) {
            Y_ASSERT(i < 256);
            notStemGram[i] = !stemGram.HasGram(*it);
            i++;
        }
        for (IGrammarStringIteratorPtr it1 = block.GetGrammarStringIterator(); it1->IsValid(); it1->Next()) {
            bool goodFlex = true;
            const TBinaryGrammarString flexGram = it1->Get();
            i = 0;
            for (TGrammarIterator it2 = neededGram.begin(); it2.IsValid(); it2.Next()) {
                if (notStemGram[i] && !flexGram.HasGram(*it2)) {
                    goodFlex = false;
                    break;
                }
                i++;
            }
            if (goodFlex) {
                return true;
            }
        }

        return false;
    }

    static bool HasScheme(TWLemmaArray::const_iterator begin, TWLemmaArray::const_iterator end, size_t schemeId) {
        for (; begin != end; ++begin) {
            if (schemeId == begin->GetParadigmId())
                return true;
        }
        return false;
    }

    static size_t CountDistortions(const wchar16* a, const wchar16* b, size_t len) {
        size_t distortions = 0;
        while (len-- > 0 && *a && *b) {
            if (*a++ != *b++)
                ++distortions;
        }
        return distortions;
    }
}

TNewLemmer::TNewLemmer(const char* data, size_t size, const NLemmer::TAlphabetWordNormalizer* alphaRules)
    : AlphaRules(alphaRules)
{
    Data = MakeHolder<TBinaryDictData>(data, size);
}

const wchar16 TNewLemmer::AffixDelimiter = L'$';
const wchar16 TNewLemmer::WordStartSymbol = L'_';
const wchar16 TNewLemmer::EmptyFlex[] = {0};

class TAnalyzer {
private:
    const TNewLemmer& Lemmer;

    TUtf16String Word;
    TUtf16String Rev;
    const NLemmer::TRecognizeOpt& Opt;
    const TDiacriticsMap* const Dm;

    TUtf16String Spec;
    size_t EndLen;
    size_t FlexLen;
    size_t StemLen;
    size_t PrefixLen;
    size_t Distortions;

    TWLemmaArray& Out;
    size_t InitialOutSize;
    bool AllBastards;
    size_t MinDistortions;
    size_t MinDistortionsEndLen;

public:
    TAnalyzer(const TNewLemmer& lemmer, const TWtringBuf& word, const NLemmer::TRecognizeOpt& opt, const TDiacriticsMap* const dm, TWLemmaArray& out)
        : Lemmer(lemmer)
        , Word(word)
        , Rev(Word)
        , Opt(opt)
        , Dm(dm)
        , Out(out)
        , InitialOutSize(out.size())
        , AllBastards(opt.GenerateAllBastards)
        , MinDistortions(Max<decltype(MinDistortions)>())
        , MinDistortionsEndLen(0)
    {
        Reverse(Rev.begin(), Rev.begin() + Rev.size());
        Rev.push_back(TNewLemmer::WordStartSymbol);
    }

private:
    inline void ComputeDistortions(size_t diaMask) {
        if (Dm && diaMask != 0) {
            size_t pos = EndLen > Word.size() ? 0 : Word.size() - EndLen;
            size_t len = Word.size() - FlexLen - pos;
            wchar16 genBuf[MAXWORD_BUF];
            wchar16 specBuf[MAXWORD_BUF];
            Dm->Generalize(Word.c_str() + pos, len, genBuf);
            Dm->SpecializeBackward(genBuf, len, specBuf, diaMask >> 1);
            Spec = Word.substr(0, pos) + TWtringBuf(specBuf, StemLen);
            Distortions = CountDistortions(Word.c_str() + pos, specBuf, len);
        } else {
            Spec = Word;
            Distortions = 0;
        }
    }

    inline bool ComputeMinDistortions(const IPattern& pattern, const IBlock& block) {
        if (!Dm)
            return true;
        size_t pos = Word.size() - FlexLen;
        Distortions += CountDistortions(Word.c_str() + pos, block.GetFormFlex(), FlexLen);
        if (Distortions < MinDistortions)
            Out.resize(InitialOutSize);
        else if (Distortions > MinDistortions)
            return false;
        else if (Distortions == MinDistortions && EndLen < MinDistortionsEndLen)
            return false;
        if (!pattern.IsFinal())
            AllBastards = true;
        MinDistortions = Distortions;
        MinDistortionsEndLen = EndLen;
        return true;
    }

    inline size_t GetBlockId(const IPattern& pattern) {
        PrefixLen = 0;
        if (Y_UNLIKELY(!pattern.HasFlexTrie()))
            return 0;
        const auto& flexTrie = pattern.GetFlexTrie();
        ui32 blockId = 0;
        bool flexFound;
        if (pattern.HasPrefix()) {
            TUtf16String flexSample = Word.substr(StemLen);
            flexSample.push_back(TNewLemmer::AffixDelimiter);
            flexSample += Word;
            size_t flexFoundLen;
            flexFound = flexTrie.FindLongestPrefix(flexSample, &flexFoundLen, &blockId);
            if (flexFoundLen <= FlexLen)
                return size_t(-1);
            PrefixLen = flexFoundLen - FlexLen - 1;
        } else
            flexFound = flexTrie.Find(Word.c_str() + StemLen, FlexLen, &blockId);
        Y_ASSERT(flexFound || blockId == 0);
        return blockId;
    }

    inline bool Append(const IPattern& pattern, const IBlock& block) {
        const ISchemePtr& scheme = pattern.GetScheme();
        TWtringBuf stem(Spec.c_str() + PrefixLen, StemLen - PrefixLen);
        TWtringBuf lemmaFlex(scheme->GetLemmaFlex());
        if (Y_UNLIKELY(stem.size() + lemmaFlex.size() >= MAXWORD_BUF))
            return false;
        Out.push_back(TYandexLemma());
        NLemmerAux::TYandexLemmaSetter set(Out.back());
        size_t lemmaPrefixPos = lemmaFlex.find(TNewLemmer::AffixDelimiter);
        if (Y_LIKELY(lemmaPrefixPos == TWtringBuf::npos)) {
            set.SetLemmaText(stem.data(), stem.size());
            set.AppendLemmaText(lemmaFlex.data(), lemmaFlex.size());
            set.SetLemmaInfo(scheme->GetId(), PrefixLen, FlexLen, scheme->GetGrammarString().AsCharPtr(), scheme->GetId());
        } else {
            set.SetLemmaText(lemmaFlex.data(), lemmaPrefixPos);
            set.AppendLemmaText(stem.data(), stem.size());
            set.AppendLemmaText(lemmaFlex.data() + lemmaPrefixPos + 1, lemmaFlex.size() - lemmaPrefixPos - 1);
            set.SetLemmaInfo(scheme->GetId(), PrefixLen, FlexLen, scheme->GetGrammarString().AsCharPtr(), scheme->GetId(), lemmaPrefixPos);
        }
        set.SetQuality(pattern.IsFinal() ? TYandexLemma::QDictionary : TYandexLemma::QBastard);
        double schemeFreq = scheme->GetFrequency() + 1;
        double stemFreq = pattern.GetFrequency();
        double endFreq = block.GetFrequency();
        set.SetWeight(stemFreq * endFreq / schemeFreq);
        for (IGrammarStringIteratorPtr it = block.GetGrammarStringIterator(); it->IsValid(); it->Next()) {
            set.AddFlexGr(it->Get().AsCharPtr(), 0);
        }
        if (Dm && Distortions > 0) {
            static const EGrammar distGr[] = {gDistort, EGrammar(0)};
            set.SetAdditGram(distGr);
        }
        return true;
    }

    inline bool IsValid(const IPattern& pattern) const {
        if (pattern.IsFinal())
            return true;
        if (pattern.IsUseAlways())
            return true;
        if (Word.size() + 1 < pattern.GetRest() + EndLen)
            return false;
        if (!Lemmer.GetAlphaRules())
            return false;
        size_t flexLen = pattern.GetOldFlexLen();
        for (size_t k = 0; k + flexLen < Word.size(); ++k) {
            if (Lemmer.GetAlphaRules()->GetCharClass(Word[k]) & NLemmer::TAlphabet::CharAlphaRequired)
                return true;
        }
        return false;
    }

    inline bool Process(const IPattern& pattern) { // returns true if this iteration should be finished
        FlexLen = EndLen - pattern.GetStemLen();
        StemLen = Word.size() - FlexLen;
        if (!IsValid(pattern))
            return false;
        ComputeDistortions(pattern.GetDiaMask());
        const ISchemePtr scheme = pattern.GetScheme();
        size_t blockId = GetBlockId(pattern);
        if (blockId == size_t(-1))
            return false;
        const IBlockPtr block = pattern.GetScheme()->GetBlock(blockId);
        if (!HasFlexWithGrammar(TBinaryGrammarString(Opt.RequiredGrammar), *scheme, *block))
            return false;
        if (!ComputeMinDistortions(pattern, *block))
            return false;
        if (AllBastards && HasScheme(Out.begin() + InitialOutSize, Out.end(), scheme->GetId()))
            return false;
        Append(pattern, *block);
        if (!AllBastards || pattern.IsFinal())
            return true;
        return false;
    }

public:
    void Analyze() {
        bool finish = false;
        TWtringBuf revWord(Rev);
        while (true) {
            TPatternMatchResult patternMatch = Lemmer.GetData().GetMatchedPatterns(revWord);
            EndLen = patternMatch.MatchedLength;
            for (IPatternIteratorPtr& it = patternMatch.PatternIterator; it->IsValid(); it->Next()) {
                const IPatternPtr pattern = it->Get();
                if (!pattern->IsFinal() && !Opt.Accept.Test(NLemmer::AccBastard))
                    return;
                finish |= Process(*pattern);
            }
            if (finish || EndLen == 0)
                break;
            revWord = revWord.substr(0, EndLen - 1);
        }
    }

    inline void SortAndNormalize(size_t maxLemmas) {
        Sort(Out.begin() + InitialOutSize, Out.end(), AnasWeightComparer);

        if (Out.size() - InitialOutSize > maxLemmas)
            Out.resize(InitialOutSize + maxLemmas);

        double z = 0.0;
        for (size_t i = InitialOutSize; i != Out.size(); ++i)
            z += Out[i].GetWeight();

        if (z > 0.0) {
            for (size_t i = InitialOutSize; i != Out.size(); ++i) {
                NLemmerAux::TYandexLemmaSetter(Out[i]).SetWeight(Out[i].GetWeight() / z);
                if (Out[i].GetWeight() < Opt.MinAllowedProbability) {
                    Out.resize(i);
                    break;
                }
            }
        }
    }

    inline size_t ResultsCount() const {
        return Out.size() - InitialOutSize;
    }
};

size_t TNewLemmer::Analyze(const TBuffer& word, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    if (!opt.Accept.Test(NLemmer::AccDictionary))
        return 0;

    const TDiacriticsMap* const dm = AlphaRules ? AlphaRules->GetDiacriticsMap() : nullptr;

    TAnalyzer analyzer(*this, word, opt, dm, out);
    analyzer.Analyze();
    analyzer.SortAndNormalize(opt.MaxLemmasToReturn);

    return analyzer.ResultsCount();
}
