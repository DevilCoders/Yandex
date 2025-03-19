#include "fio_inflector_core.h"

#include "fio_exceptions.h"

#include <kernel/lemmer/dictlib/tgrammar_processing.h>


namespace {
    TVector<char> GenerateRequiredGrammems(const TGramBitSet& required) {
        using NTGrammarProcessing::tg2ch;

        TVector<char> requiredGrammemsList;

        for (EGrammar gr : required) {
            requiredGrammemsList.push_back(tg2ch(gr));
        }

        requiredGrammemsList.push_back(0);
        return requiredGrammemsList;
    }

    bool HasGram(const TWLemmaArray& lemmas, EGrammar gram) {
        bool hasGram = false;
        for (const auto& yl : lemmas) {
            if (hasGram) {
                break;
            }
            hasGram |= yl.HasGram(gram);
        }
        return hasGram;
    }
}


namespace NFioInflector {

EGrammar TFioInflectorCore::GuessGender(const TUtf16String& s, const TGramBitSet& hint) const {
    TGramBitSet hintBitSet = hint | TGramBitSet(gSingular, gNominative);
    const auto& requiredGrammar = GenerateRequiredGrammems(hintBitSet);
    try {
        auto results = MyAnalyze(s, requiredGrammar.data());
        bool hasMasculine = HasGram(results, gMasculine);
        bool hasFeminine = HasGram(results, gFeminine);

        if (hasMasculine && hasFeminine)
            return gMasFem;
        if (hasMasculine)
            return gMasculine;
        if (hasFeminine)
            return gFeminine;
    } catch (const TAnalyzeException& err) {
    }
    return gMasFem;
}

EGrammar TFioInflectorCore::GuessGender(const TUtf16String& s, EGrammar proper) const {
    TGramBitSet hint = TGramBitSet(proper);
    return GuessGender(s, hint);
}

EGrammar TFioInflectorCore::GuessGender(const TUtf16String& name, const TUtf16String& surname) const {
    EGrammar nameGender = GuessGender(name, gFirstName);
    EGrammar surnameGender = GuessGender(surname, gSurname);
    if (nameGender == surnameGender || nameGender != gMasFem)
        return nameGender;
    return surnameGender;
}

EGrammar TFioInflectorCore::GuessGender(const TUtf16String& name) const {
    return GuessGender(name, TGramBitSet());
}

TYandexLemma TFioInflectorCore::AnalyzeChunk(const TUtf16String& s,
                                         const char * requiredGrammar,
                                         NLemmer::EAccept acc) const
{
    // Solve renuxa problem (STARTREK-6134)
    TLangMask mask = NLemmer::ClassifyLanguageAlpha(s.c_str(), s.length(), true);
    if (!mask.SafeTest(Lang->Id))
        throw TAnalyzeException() << "Unsupported language for: " << s;

    NLemmer::TRecognizeOpt opt(acc, requiredGrammar);
    opt.SkipAdvancedNormaliation = Options.SkipAdvancedNormaliation;
    //opt.GenerateAllBastards = true;
    TVector<TYandexLemma> results;
    Lang->Recognize(s.c_str(), s.size(), results, opt);
    if (results.empty())
       throw TAnalyzeException() << "Cannot analyze: " << s;
    return results.back();
}

TWLemmaArray TFioInflectorCore::MyAnalyze(const TUtf16String& s, const char * requiredGrammar) const {
    size_t pos = s.find(L'-');
    if (pos != TUtf16String::npos) {
        try {
            TYandexLemma res = AnalyzeChunk(s, requiredGrammar, NLemmer::AccDictionary);
            return TWLemmaArray(1, res);
        } catch (TAnalyzeException&) {
            TWLemmaArray result;
            result.push_back(AnalyzeChunk(s.substr(0, pos), requiredGrammar));
            result.push_back(AnalyzeChunk(s.substr(pos + 1), requiredGrammar));
            return result;
        }
    } else {
        return TWLemmaArray(1, AnalyzeChunk(s, requiredGrammar));
    }
}

TWLemmaArray TFioInflectorCore::Analyze(const TUtf16String& name, const TGramBitSet& hint) const {
    TGramBitSet hintBitSet = hint | TGramBitSet(gSingular, gNominative);
    const auto& requiredGrammar = GenerateRequiredGrammems(hintBitSet);
    return MyAnalyze(name, requiredGrammar.data());
}

TWLemmaArray TFioInflectorCore::Analyze(const TUtf16String& name, EGrammar gender, EGrammar proper) const {
    return Analyze(name, TGramBitSet(gender, proper));
}

TWLemmaArray TFioInflectorCore::Analyze(const TUtf16String& name, EGrammar gender) const {
    return Analyze(name, TGramBitSet(gender));
}

TUtf16String TFioInflectorCore::Generate(const TYandexLemma& lemma, EGrammar gCase) const {
    const auto& gram = GenerateRequiredGrammems(TGramBitSet(gSingular, gCase));
    TWordformArray forms;
    size_t formsNum = Lang->Generate(lemma, forms, gram.data());
    if (formsNum == 0)
        throw TGenerateException() << "Cannot generate forms for: " << lemma.GetText();

    static const auto& badGrammar = GenerateRequiredGrammems(TGramBitSet(gReserved, gObsolete));
    using NTGrammarProcessing::HasGram;
    for (size_t i = 0; i != formsNum; ++i)
        for (size_t j = 0; j != forms[i].FlexGramNum(); ++j)
            if (!HasGram(forms[i].GetFlexGram()[j], badGrammar.data()))
                return forms[i].GetText();
    return forms[0].GetText();
}

TUtf16String TFioInflectorCore::Generate(const TWLemmaArray& lemmas, EGrammar gCase) const {
    TUtf16String result;
    for (size_t i = 0; i != lemmas.size(); ++i) {
        if (!result.empty())
            result += u"-";
        result += Generate(lemmas[i], gCase);
    }
    return result;
}

EGrammar TFioInflectorCore::GuessProper(const TUtf16String& name, const TGramBitSet& hint) const {
    // don't raise exception on guessing stage
    try {
        auto result = Analyze(name, hint);
        if (!result.empty()) {
            const auto& lemma = result[0];
            if (!lemma.IsBastard()) {
                if (lemma.HasGram(gFirstName)) {
                    return gFirstName;
                }
                if (lemma.HasGram(gPatr)) {
                    return gPatr;
                }
            }
        }
    } catch (const TAnalyzeException&) {
    }

    return gSurname;
}

} // namespace NFioInflector

