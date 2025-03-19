#include "pluralize.h"

#include <kernel/inflectorlib/phrase/complexword.h>
#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <kernel/lemmer/dictlib/grammar_enum.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <util/charset/wide.h>


namespace {
    struct TPluralizationParams {
        EGrammar Case = gNominative;
        EGrammar Number = gSingular;
        bool PluralizeInflect = false; // pluralization mode in inflector: replace sg with pl for adjectives
    };

    TUtf16String PluralizeDefault(const TUtf16String& text, ui64 targetNumber, ELanguage lang) {
        NInfl::TSimpleInflector inflector(NameByLanguage(lang));
        if (targetNumber == 1) {
            return inflector.Inflect(text, "sg");
        } else {
            return inflector.Inflect(text, "pl");
        }
    }

    bool IsAcceptedCaseGrammar(EGrammar grammar) {
        return NSpike::AllMajorCases.Has(grammar);
    }

    TPluralizationParams GetPluralizationParams(ui64 targetNumber, EGrammar targetCase, bool isAnimated) {
        TPluralizationParams res;

        const ui64 mod10 = targetNumber % 10;
        const ui64 mod100 = targetNumber % 100;
        if (0 == targetNumber) {
            // им 0 черных котов
            // рд 0-я черных котов
            // дт 0-ю черных котов
            // вн 0 черных котов
            //
            res.Case = gGenitive;
            res.Number = gPlural;
            res.PluralizeInflect = false;
        } else if (0 == mod10
            || (mod10 >= 5 && mod10 <= 9)
            || (mod100 >= 11 && mod100 <= 14))
        {
            // им 10 черных котов
            // рд 10-и черных котов
            // дт 10-и черным котам
            // вн 10 черных котов
            //
            res.Case = (gNominative == targetCase || gAccusative == targetCase)
                ? gGenitive
                : targetCase;

            res.Number = gPlural;
            res.PluralizeInflect = false;
        } else if (1 == mod10) {
            // им 1 черный кот
            // рд 1-го черного кота
            // дт 1-му черному коту
            // вн 1-го черного кота
            //
            res.Case = targetCase;
            res.Number = gSingular;
            res.PluralizeInflect = false;
        } else {
            Y_ASSERT(mod10 >= 2 && mod10 <= 4);
            Y_ASSERT(mod100 < 10 || mod100 > 20);
            if (isAnimated) {
                // им 2 черных кота
                // рд 2-х черных котов
                // дт 2-м черным котам
                // вн 2-х черных котов
                //
                if (gNominative == targetCase) {
                    res.PluralizeInflect = true;
                } else if (gAccusative == targetCase) {
                    if (targetNumber < 10) {
                        res.Case = gGenitive;
                        res.Number = gPlural;
                        res.PluralizeInflect = false;
                    } else {
                        Y_ASSERT(targetNumber > 20);
                        res.PluralizeInflect = true;
                    }
                } else {
                    res.Case = targetCase;
                    res.Number = gPlural;
                    res.PluralizeInflect = false;
                }
            } else {
                if (gNominative == targetCase || gAccusative == targetCase) {
                    res.PluralizeInflect = true;
                } else {
                    res.Case = targetCase;
                    res.Number = gPlural;
                    res.PluralizeInflect = false;
                }
            }
        }
        return res;
    }

    TUtf16String PluralizeSlavic(
        const TUtf16String& text,
        ui64 targetNumber,
        EGrammar targetCase,
        ELanguage lang,
        bool isAnimated)
    {
        Y_ENSURE_EX(
            IsAcceptedCaseGrammar(targetCase),
            yexception{}
                << "unexpected grammar for target case: "
                << TGrammarIndex::GetName(targetCase, true));

        TPluralizationParams params = GetPluralizationParams(targetNumber, targetCase, isAnimated);

        NInfl::TSimpleInflector inflector(NameByLanguage(lang));
        NInfl::TSimpleResultInfo res;
        TUtf16String ret;
        TWtringBuf wordMain(text);

        if (!text.Contains(' ') && text.Contains('-')) {
            // split the 1st part and check if it is animated
            // if not, do not inflect it
            // им 2 веб-раработчика
            TWtringBuf prefix = wordMain.Before('-');
            NInfl::TComplexWord word(LI_CYRILLIC_LANGUAGES, prefix);
            const auto isAnim = [](const TYandexLemma& lemma) { return !lemma.IsBastard() && lemma.HasStemGram(gAnimated); };
            if (std::none_of(word.GetCandidates().begin(), word.GetCandidates().end(), isAnim)) {
                ret = ToWtring(prefix).append('-');
                wordMain = wordMain.After('-');
            }
        }

        return ret + inflector.Inflect(
            ToWtring(wordMain),
            TGramBitSet{params.Case, params.Number}.ToString(",", true),
            &res,
            params.PluralizeInflect);
    }

    TUtf16String SingularizeDefault(const TUtf16String& text, ui64 sourceNumber, ELanguage lang) {
        NInfl::TSimpleInflector inflector(NameByLanguage(lang));
        if (sourceNumber == 1) {
            return text;
        } else {
            return inflector.Inflect(text + u"{g=pl}", "sg");
        }
    }

    TUtf16String SingularizeSlavic(const TUtf16String& text, ui64 sourceNumber, ELanguage lang) {
        NInfl::TSimpleInflector inflector(NameByLanguage(lang));
        auto last1 = sourceNumber % 10;
        auto last2 = sourceNumber % 100;
        if (last1 == 0 || 5 <= last1 && last1 <= 9 || 11 <= last2 && last2 <= 19) {
            return inflector.Inflect(text + u"{g=gen,pl}", "nom,sg");
        } else if (last1 == 1) {
            return text;
        } else { // 2 <= last1 && last1 <= 4
            auto plural = inflector.Inflect(text + u"{g=gen}", "gen,pl");
            return inflector.Inflect(plural + u"{g=gen,pl}", "nom,sg");
        }
    }
} // namespace

namespace NInfl {
    TUtf16String Pluralize(
        const TUtf16String& text,
        ui64 targetNumber,
        ELanguage lang)
    {
        return Pluralize(text, targetNumber, gNominative, lang);
    }

    TUtf16String Pluralize(
        const TUtf16String& text,
        ui64 targetNumber,
        EGrammar targetCase,
        ELanguage lang,
        bool isAnimated)
    {
        switch (lang) {
            case LANG_RUS:
            case LANG_UKR:
            case LANG_BEL:
                return PluralizeSlavic(text, targetNumber, targetCase, lang, isAnimated);
            case LANG_TUR:
                return text;
            default:
                return PluralizeDefault(text, targetNumber, lang);
        }
    }


    TUtf16String Singularize(
        const TUtf16String& text,
        ui64 sourceNumber,
        ELanguage lang)
    {
        switch (lang) {
            case LANG_RUS:
            case LANG_UKR:
            case LANG_BEL:
                return SingularizeSlavic(text, sourceNumber, lang);
            case LANG_TUR:
                return text;
            default:
                return SingularizeDefault(text, sourceNumber, lang);
        }
    }
} // NInfl
