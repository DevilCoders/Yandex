#include <util/charset/wide.h>
#include <util/generic/set.h>
#include <util/generic/singleton.h>

#include "rubbishdump.h"

#include <kernel/lemmer/dictlib/tgrammar_processing.h>
#include <library/cpp/tokenizer/nlpparser.h>

namespace NLanguageRubbishDump {
    static NLemmer::TLllLevel EasternSlavicLooksLikeLemmaInt(const char* stemGram, const char* const flexGram[], size_t gramNum, char* gramForGenerator, size_t bufsLen) {
        using NTGrammarProcessing::GetGramIndex;
        using NTGrammarProcessing::HasGram;
        using NTGrammarProcessing::ch2tg;
        using NTGrammarProcessing::tg2ch;

        static const char FiltrGrNom[] = {
            tg2ch(gFeminine), tg2ch(gNeuter),   // род - кроме мужского
            tg2ch(gPlural),                     // число - только множественное
            tg2ch(gParticiple),                 // причастие
            tg2ch(gPerfect), tg2ch(gImperfect), // вид глагола - для причастия
            tg2ch(gPassive), tg2ch(gActive),    // залог - для причастия
            0};
        static const char FiltrGrInf[] = {0};

        const char* filtrGramm = "";

        const char* flexGr = nullptr;
        if ((flexGr = HasGram(flexGram, gramNum, gNominative)) != nullptr)
            filtrGramm = FiltrGrNom;
        else if ((flexGr = HasGram(flexGram, gramNum, gInfinitive)) != nullptr)
            filtrGramm = FiltrGrInf;
        else
            return NLemmer::LllNo;
        NLemmer::TLllLevel r = NLemmer::Lll;
        if (HasGram(stemGram, gSubstantive) && (HasGram(stemGram, gSingular) || HasGram(stemGram, gPlural))) {
            r = NLemmer::LllTantum;
        }

        if (!bufsLen)
            return r;

        size_t k = 0;
        bufsLen--;
        while (*flexGr) {
            size_t n = GetGramIndex(flexGr, filtrGramm);
            if (flexGr[n] && k < bufsLen) {
                gramForGenerator[k++] = flexGr[n];
                flexGr += (n + 1);
            } else
                break;
        }
        gramForGenerator[k] = 0;

        return r;
    }

    NLemmer::TLllLevel EasternSlavicLooksLikeLemma(const TYandexLemma& lemma, char* gramForGenerator, size_t bufsLen) {
        if (bufsLen)
            gramForGenerator[0] = 0;
        NLemmer::TLllLevel r = DefaultLooksLikeLemma(lemma);

        NLemmer::TLllLevel rt = EasternSlavicLooksLikeLemmaInt(lemma.GetStemGram(), lemma.GetFlexGram(), lemma.FlexGramNum(),
                                                               gramForGenerator, bufsLen);
        if (rt > r)
            r = rt;
        return r;
    }

    static bool LemmaEqForm(const TYandexLemma& lemma) {
        return lemma.GetTextLength() == lemma.GetNormalizedFormLength() && !memcmp(lemma.GetText(), lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength() * sizeof(lemma.GetNormalizedForm()[0]));
    }

    static EGrammar PartOfSpeech(const TYandexLemma& lemma) {
        if (!lemma.GetStemGram())
            return (EGrammar)0;
        return NTGrammarProcessing::ch2tg(lemma.GetStemGram()[0]);
    }

    NLemmer::TLllLevel DefaultLooksLikeLemma(const TYandexLemma& lemma) {
        using namespace NLemmer;
        if (lemma.GetQuality() & (TYandexLemma::QFoundling | TYandexLemma::QOverrode))
            return LllNo;
        switch (PartOfSpeech(lemma)) {
            case gSubstantive:
            case gVerb:
            case gAdjective:
            case gAdverb:
                //        case gAdjNumeral:
                //        case gNumeral:
                break;
            default:
                return LllNo;
        }
        if (LemmaEqForm(lemma))
            return LllSameText;
        return LllNo;
    }

    wchar16 ToLowerTurkish(wchar16 ch) {
        switch (ch) {
            case static_cast<wchar16>(0x130):
                return static_cast<wchar16>('i');
            case static_cast<wchar16>('I'):
                return static_cast<wchar16>(0x131);
        }
        return static_cast<wchar16>(ToLower(ch));
    }

    wchar16 ToUpperTurkish(wchar16 ch) {
        switch (ch) {
            case static_cast<wchar16>('i'):
                return static_cast<wchar16>(0x130);
            case static_cast<wchar16>(0x131):
                return static_cast<wchar16>('I');
        }
        return static_cast<wchar16>(ToUpper(ch));
    }

    class TEnglishCanBreakConsts {
    public:
        typedef TSet<TUtf16String> TSetType;

    public:
        const wchar16 chrT;
        const wchar16 chrS;
        const wchar16 chrN;
        const TSetType BreakSet;
        TEnglishCanBreakConsts()
            : chrT('t')
            , chrS('s')
            , chrN('n')
            , BreakSet(GetSet())
        {
        }

    private:
        TSetType GetSet() {
            struct TLocSet: public TSetType {
                TLocSet(const char* const* arr) {
                    for (; (*arr)[0]; ++arr)
                        insert(ASCIIToWide(*arr));
                }
            };
            static const char* const englishUnbreakableTokens[] = {
                "isn", "aren", "wasn", "weren",
                "don", "doesn", "didn",
                "can", "couldn",
                "shan", "shouldn", "won", "wouldn",
                "haven", "hasn", "hadn",
                "needn", "daren",
                "mayn", "mightn",
                "mustn", "oughtn",
                "usedn", /* don't ask me about this one :-) */
                ""};
            return TLocSet(englishUnbreakableTokens);
        }
    };

    bool EnglishCanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2) {
        const TEnglishCanBreakConsts* consts = Singleton<TEnglishCanBreakConsts>();

        if (pos1 + len1 + 1 != pos2 || len2 != 1 || !len1)
            return true;
        if (TNlpParser::GetCharClass(word[pos2 - 1]) != TNlpParser::CC_APOSTROPHE)
            return true;
        if (word[pos2] == consts->chrS)
            return false;
        // Accelerate: check common assumptions
        if (word[pos2] != consts->chrT)
            return true;
        if (word[pos2 - 2] != consts->chrN)
            return true;

        TUtf16String s(word + pos1, len1);
        return consts->BreakSet.find(s) == consts->BreakSet.end();
    }

} //namespace NLanguageRubbishDump
