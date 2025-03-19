#include <util/charset/wide.h>
#include <util/system/yassert.h>
#include <util/generic/string.h>

#include <kernel/lemmer/core/language.h>

#include "trie/trie.h"
#include "ngrams/ngrams.h"

#include "untranslit.h"

const double TUntransliter::DefaultQualityRange = 2.7182818459045;
static const size_t MaxNumberOfVariants = 8192;

using NLemmer::NDetail::TAlphaMap;
using NLemmer::NDetail::TNGramData;

static void MakeCapt(TCharCategory capt, TUtf16String& word) {
    if (word.empty())
        return;
    if (capt & CC_UPPERCASE)
        word.to_upper();
    else if (capt & CC_TITLECASE)
        word.to_title();
}

size_t TUntransliter::WordPart::AddPart(const wchar16*& trRes, size_t num) {
    Y_ASSERT(trRes && *trRes && *trRes != WeightDelimiter);
    while (*trRes && *trRes != WeightDelimiter)
        Word += *(trRes++);
    Y_ASSERT(*trRes == WeightDelimiter);
    Weight += TUntransliter::DecodeWeight(*++trRes);
    Ind += num;
    ++trRes;
    return num;
}

void TUntransliter::WordPart::MakeCapt(TCharCategory capt) {
    ::MakeCapt(capt, Word);
}

void TUntransliter::WordPart::AddWeight(ui32 weight) {
    Weight += weight;
}

size_t TUntransliter::WordPart::GetInd() const {
    return Ind;
}

bool operator<(const TUntransliter::WordPart& w1, const TUntransliter::WordPart& w2) {
    if (w1.Weight != w2.Weight)
        return w1.Weight > w2.Weight;
    if (w1.Ind != w2.Ind)
        return w1.Ind < w2.Ind;
    return w1.GetWord() > w2.GetWord();
}

class TUntransliter::TNgrChecker {
private:
    const TNGramData& NGram;
    const TAlphaMap& AlphaMap;

public:
    TNgrChecker(const TNGramData& nGram, const TAlphaMap& alphaMap)
        : NGram(nGram)
        , AlphaMap(alphaMap)
    {
    }

    ui32 Weight(const TUtf16String& str, size_t diff, bool ending) {
        if (NGram.IsEmpty())
            return 0;
        size_t len = str.length();
        ui32 num = 0;
        num += CheckNGrInt(str.c_str(), len, ending);
        for (size_t i = 1; i < diff; ++i)
            num += CheckNGrInt(str.c_str(), --len, false);
        return num;
    }

private:
    ui32 CheckNGrInt(const wchar16* beg, size_t length, bool ending) {
        if (length < TNGramData::NgrSize - 1) {
            if (length > 0 && ending) {
                wchar16 buf[TNGramData::NgrSize + 1];
                for (size_t i = 0; i < TNGramData::NgrSize; ++i)
                    buf[i] = ' ';
                memcpy(buf + 1, beg, length * sizeof(wchar16));
                buf[TNGramData::NgrSize] = 0;
                return NGram.LookForNGr(buf, AlphaMap);
            }
            return 0;
        }
        const wchar16* end = beg + length;
        wchar16 buf[TNGramData::NgrSize + 1];
        ui32 num = 0;
        if (length == TNGramData::NgrSize - 1) {
            buf[0] = ' ';
            memcpy(buf + 1, beg, (TNGramData::NgrSize - 1) * sizeof(wchar16));
            buf[TNGramData::NgrSize] = 0;
            num += NGram.LookForNGr(buf, AlphaMap);
        }
        if (ending) {
            memcpy(buf, end - TNGramData::NgrSize + 1, (TNGramData::NgrSize - 1) * sizeof(wchar16));
            buf[TNGramData::NgrSize - 1] = ' ';
            buf[TNGramData::NgrSize] = 0;
            num += NGram.LookForNGr(buf, AlphaMap);
        }
        if (length >= TNGramData::NgrSize) {
            memcpy(buf, end - TNGramData::NgrSize, TNGramData::NgrSize * sizeof(wchar16));
            buf[TNGramData::NgrSize] = 0;
            num += NGram.LookForNGr(buf, AlphaMap);
        }
        return num;
    }
};

TUntransliter::TUntransliter(const TTrieNode<wchar16>* trie, const wchar16** trieData, const TNGramData& nGram, const TAlphaMap& alphaMap)
    : Trie(trie)
    , TrieData(trieData)
    , NGr(new TNgrChecker(nGram, alphaMap))
{
}

TUntransliter::~TUntransliter() {
}

void TUntransliter::Init(const TWtringBuf& word) {
    while (!Que.empty())
        Que.pop();
    Word.resize(word.size());
    ToLower(word.data(), word.size(), Word.begin());
    Capt = NLemmer::ClassifyCase(word.data(), word.size()) & (CC_UPPERCASE | CC_TITLECASE);
    if (Word.length() > MaxUntransliterableLength)
        return;
    TVector<TWtringTrieFinderRet> pars;
    NextLetter(WordPart(), pars);
}

bool TUntransliter::Unacceptable(const TUtf16String&, size_t) const {
    return false;
}

void TUntransliter::NextLetter(const WordPart& wp, TVector<TWtringTrieFinderRet>& pars, ui32 worstQuality) {
    LookFor(Trie, TrieData, Word.c_str() + wp.GetInd(), pars);
    for (size_t i = 0; i < pars.size(); ++i) {
        while (pars[i].Val[0]) {
            WordPart r = wp;
            if (!r.AddPart(pars[i].Val, pars[i].Depth))
                continue;
            size_t diff = r.GetWord().length() - wp.GetWord().length();
            if (Unacceptable(r.GetWord(), diff))
                continue;
            r.AddWeight(NGr->Weight(r.GetWord(), diff, !Word[r.GetInd()]));
            if (worstQuality && r.Quality() > worstQuality)
                continue;
            Que.push(r);
        }
    }
}

//#define TEST_UNTRANSLIT
TUntransliter::WordPart TUntransliter::GetNextAnswer(ui32 worstQuality) {
    TVector<TWtringTrieFinderRet> pars;
    while (!Que.empty() && Que.size() <= MaxNumberOfVariants) {
        WordPart wp = Que.top();
        Que.pop();
        if (worstQuality && wp.Quality() > worstQuality)
            break;
        if (!Word[wp.GetInd()]) {
            wp.MakeCapt(Capt);
#ifdef TEST_UNTRANSLIT
            Cerr << "Untransliter::GetNextAnswer: " << WideToUTF8(wp.GetWord())
                 << " Capt: " << Capt
                 << ", word index: " << wp.GetInd() << ", word quality: " << wp.Quality()
                 << ", variants: " << Que.size() << Endl;
#endif
            return wp;
        }
        NextLetter(wp, pars, worstQuality);
    }
    return WordPart();
}
