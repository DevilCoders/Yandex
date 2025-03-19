#include "rus.h"

#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/rubbishdump.h>
#include <library/cpp/tokenizer/nlpparser.h>

NLemmer::TLllLevel TRusLanguage::LooksLikeLemma(const TYandexLemma& lemma) const {
    return NLanguageRubbishDump::EasternSlavicLooksLikeLemma(lemma);
}

TAutoPtr<NLemmer::TGrammarFiltr> TRusLanguage::LooksLikeLemmaExt(const TYandexLemma& lemma) const {
    return NLanguageRubbishDump::EasternSlavicLooksLikeLemmaExt(this, lemma);
}

namespace { // PostRecognize auxiliary
    static EGrammar GetBadGr(const char* gr) {
        using NTGrammarProcessing::GetGramOf;
        using NTGrammarProcessing::tg2ch;
        static const char bad_grammar1[] = {tg2ch(gParticiple), tg2ch(gGerund), tg2ch(gImperative), 0};
        static const char bad_grammar2[] = {tg2ch(gShort), 0};

        EGrammar ret = GetGramOf(gr, bad_grammar1);
        if (ret)
            return ret;
        return GetGramOf(gr, bad_grammar2);
    }

    static EGrammar GetBadGr(const char* const flexGram[], size_t gramNum) {
        if (!gramNum)
            return EGrammar(0);
        EGrammar ret = GetBadGr(flexGram[0]);
        for (size_t i = 1; i < gramNum; ++i) {
            if (!GetBadGr(flexGram[i]))
                return EGrammar(0);
        }
        return ret;
    }

    typedef ui8 TGoodBadMask;

    static const TGoodBadMask goodAny = 1;
    static const TGoodBadMask goodAdv = 2;
    static const TGoodBadMask goodA = 4;
    static const TGoodBadMask goodAC = 8;
    static const TGoodBadMask goodS = 16;

    static const TGoodBadMask badAny = 1;
    static const TGoodBadMask badPart = 2;
    static const TGoodBadMask badShort = 4;

    static TGoodBadMask GetBadMask(EGrammar badGr) {
        Y_ASSERT(badGr);
        switch (badGr) {
            case gParticiple:
                return badAny | badPart;
            case gShort:
                return badAny | badShort;
            default:
                return badAny;
        }
    }

    static TGoodBadMask GetGoodMask(const char* stemGram, const char* const* flexGram, size_t flexGramNum) {
        using NTGrammarProcessing::HasGram;
        using NTGrammarProcessing::ch2tg;
        switch (ch2tg(stemGram[0])) {
            case gSubstantive:
                return goodS | goodAny;
            case gAdverb:
                return goodAdv | goodAny;
            case gAdjective:
                if (HasGram(stemGram, gComparative) || HasGram(flexGram, flexGramNum, gComparative))
                    return goodA | goodAC | goodAny;
                return goodA | goodAny;
            default:
                return goodAny;
        }
    }

    static EGrammar GetLemmaInfo(const TYandexLemma& lm, TGoodBadMask& goodMask, TGoodBadMask& badMask) {
        if (lm.GetQuality() & TYandexLemma::QFoundling)
            return EGrammar(0);
        const char* stemGram = lm.GetStemGram();
        const char* const* flexGram = lm.GetFlexGram();
        size_t flexGramNum = lm.FlexGramNum();

        EGrammar badGr = GetBadGr(stemGram);
        if (!badGr)
            badGr = GetBadGr(flexGram, flexGramNum);

        if (badGr)
            badMask |= GetBadMask(badGr);
        else
            goodMask |= GetGoodMask(stemGram, flexGram, flexGramNum);

        return badGr;
    }

    static bool IsBad(EGrammar badGr, TGoodBadMask goodMask) {
        switch (badGr) {
            case gParticiple:
                return !(goodMask & goodA);
            case gShort:
                return !(goodMask & goodAdv);
            case gImperative:
                return !(goodMask & goodAC);
            default:
                return !!badGr;
        }
    }
}

namespace {
    void PostRecognize(TWLemmaArray& lemmas, const size_t begin, const size_t end, const TLanguage::TConvertResults& cr, size_t version) {
        static const EGrammar addGr[] = {gObsolete, EGrammar(0)};
        if (end <= begin)
            return;

        TGoodBadMask goodMask = 0;
        TGoodBadMask badMask = 0;
        EGrammar* badGr = static_cast<EGrammar*>(alloca((end - begin) * sizeof(EGrammar)));
        for (size_t i = begin; i < end; ++i) {
            if (cr.ConvertRet.IsChanged.Test(NLemmer::CnvAdvancedConvert))
                NLemmerAux::TYandexLemmaSetter(lemmas[i]).SetAdditGram(addGr);

            badGr[i - begin] = GetLemmaInfo(lemmas[i], goodMask, badMask);
        }

        if (!badMask || !goodMask)
            return;
        if (goodMask & goodS)
            goodMask = goodS;

        for (size_t i = begin; i < end; ++i) {
            if (IsBad(badGr[i - begin], goodMask)) {
                if (version == 0 || i > begin || lemmas[i].GetWeight() < 0.9) { // Do not set BadRequest on the first, most probable lemma
                    NLemmerAux::TYandexLemmaSetter(lemmas[i]).SetQuality(lemmas[i].GetQuality() | TYandexLemma::QBadRequest);
                }
            }
        }
        return;
    }
}

size_t TRusLanguage::PostRecognize(TWLemmaArray& lemmas, const size_t begin, const size_t end, const TConvertResults& cr, const NLemmer::TRecognizeOpt&) const {
    //if two morphologies are active for language, new lemmas should go first, then old
    size_t left = begin;
    size_t right = begin;
    for (; right < end; ++right) {
        if (lemmas[right].GetLanguageVersion() != lemmas[left].GetLanguageVersion()) {
            ::PostRecognize(lemmas, left, right, cr, lemmas[left].GetLanguageVersion());
            left = right;
        }
    }
    if (left < end) {
        ::PostRecognize(lemmas, left, right, cr, lemmas[left].GetLanguageVersion());
    }
    return end - begin;
}

bool TRusLanguage::CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t /*len2*/, bool isTranslit) const {
    if (!isTranslit)
        return true;
    static const TUtf16String setConsonants(u"bcdfghjklmnpqrstvwxz");

    if (pos1 + len1 + 1 != pos2 || !len1)
        return true;
    if (TNlpParser::GetCharClass(word[pos1 + len1]) != TNlpParser::CC_APOSTROPHE)
        return true;
    return setConsonants.find(word[pos1 + len1 - 1]) >= setConsonants.length();
}
