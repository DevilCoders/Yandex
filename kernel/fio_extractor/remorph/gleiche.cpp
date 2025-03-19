#include "gleiche.h"

#include <library/cpp/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/iterator.h>
#include <util/stream/trace.h>
#include <util/stream/str.h>

using namespace NSymbol;

namespace {

    typedef TVector<TGramBitSet> TGrammarBunch;

    struct TFioHomonym {
        static const size_t EMPTY;
        static const size_t USE_ORIGINAL_TEXT;
        size_t FirstNameHomonym, SecondNameHomonym, SurnameHomonym;
        TGrammarBunch Grammems;

        TFioHomonym()
            : FirstNameHomonym(EMPTY), SecondNameHomonym(EMPTY), SurnameHomonym(EMPTY)
        { }

        static bool IsLemma(size_t hom) {
            return hom != EMPTY && hom != USE_ORIGINAL_TEXT;
        }


        static TWtringBuf GetText(size_t homonym, const TInputSymbol* symbol) {
            if (homonym == TFioHomonym::EMPTY) {
                return TWtringBuf();
            } else if (homonym == TFioHomonym::USE_ORIGINAL_TEXT) {
                return symbol->GetNormalizedText();
            }
            return symbol->GetLemmas().GetLemmaText(homonym);
        }
    };

    const size_t TFioHomonym::EMPTY = Max<size_t>();
    const size_t TFioHomonym::USE_ORIGINAL_TEXT = TFioHomonym::EMPTY - 1;

    struct TCompareHomonymsByText {
        size_t TFioHomonym::* Field;
        const TInputSymbol* Symbol;

        TCompareHomonymsByText(size_t TFioHomonym::* field, const TInputSymbol* symbol)
            : Field(field), Symbol(symbol)
        { }

        bool operator()(const TFioHomonym& lhs, const TFioHomonym& rhs) const {
            return lhs.*Field != rhs.*Field && GetText(lhs) < GetText(rhs);
        }

        bool Equals(const TFioHomonym& lhs, const TFioHomonym& rhs) const {
            return lhs.*Field == rhs.*Field || GetText(lhs) == GetText(rhs);
        }

        TWtringBuf GetText(const TFioHomonym& hom) const {
            Y_ASSERT(hom.*Field != TFioHomonym::EMPTY);
            return TFioHomonym::GetText(hom.*Field, Symbol);
        }
    };

    void AbsorbHomonym(TFioHomonym& where, const TFioHomonym& what) {
        if (where.FirstNameHomonym == TFioHomonym::EMPTY)
            where.FirstNameHomonym = what.FirstNameHomonym;
        if (where.SecondNameHomonym == TFioHomonym::EMPTY)
            where.SecondNameHomonym = what.SecondNameHomonym;
        if (where.SurnameHomonym == TFioHomonym::EMPTY)
            where.SurnameHomonym = what.SurnameHomonym;
    }

    typedef TVector<TFioHomonym> TFioHomonyms;

    bool FioMembersGrammarCheck(const TGramBitSet& g1, const TGramBitSet& g2, TGramBitSet& res)
    {
        static const TGramBitSet name(gFirstName, gPatr, gSurname);
        static const TGramBitSet all_gnc = TGramBitSet(gSingular) | name | NSpike::AllCases | NSpike::AllGenders;
        static const TGramBitSet groups[] = { NSpike::AllCases, NSpike::AllGenders, NSpike::AllNumbers };
        static const size_t groupsCount = sizeof(groups) / sizeof(groups[0]);

        TGramBitSet common = ::NGleiche::NormalizeAll(g1) & ::NGleiche::NormalizeAll(g2);
        TGramBitSet all = ::NGleiche::NormalizeAll(g1) | ::NGleiche::NormalizeAll(g2);

        for (size_t i = 0; i < groupsCount; ++i) {
            if (g1.HasAny(groups[i]) != g2.HasAny(groups[i])) {
                // one of the fio memebers doesn't has any grammar from the groups[i] (e.g. adverb),
                // so keep it in fio
                common |= all & groups[i];
            } else if (!common.HasAny(groups[i]) && (g1.HasAny(groups[i]) || g2.HasAny(groups[i]))) {
                return false;
            }
        }

        res = all_gnc & ((all & name) | common);
        return true;

    }

    TGrammarBunch FilterGrammars(const TGrammarBunch& g1, const TGrammarBunch& g2)
    {
        TGrammarBunch result;
        // comparing all possible pairs
        for (TGrammarBunch::const_iterator it1 = g1.begin(); it1 != g1.end(); ++it1)
            for (TGrammarBunch::const_iterator it2 = g2.begin(); it2 != g2.end(); ++it2) {
                TGramBitSet grammems;
                if (FioMembersGrammarCheck(*it1, *it2, grammems))
                    result.push_back(grammems);
            }
        return result;
    }

    template<class TGrammarBunch>
    TString ToStroku(const TGrammarBunch& set, const char* delim = " | ") {
        TStringStream result;
        for (typename TGrammarBunch::const_iterator i = set.begin(); i != set.end(); ++i)
            result << delim << i->ToString();
        return result.Str();
    }

#if !defined(Y_ENABLE_TRACE)
    Y_DECLARE_UNUSED
#endif
    TString ToStroku(const TFioHomonyms& homonyms, const TInputSymbol* symbol, size_t TFioHomonym::* field) {
        TStringStream result;
        for (size_t i = 0; i < homonyms.size(); ++i) {
            if (i)
            result << Endl;
            result << "  Homonym " << i << ": " << TFioHomonym::GetText(homonyms[i].*field, symbol);
            result << ToStroku(homonyms[i].Grammems, "\n    Grammar: ");
        }
        return result.Str();
    }

    void GetSpecialGrammarForms(TFioHomonyms& homonyms, const ILemmas& lemmas, size_t TFioHomonym::* field) {
        if (field != &TFioHomonym::SurnameHomonym)
            return;
        const TGramBitSet specialCase(gGenitive, gPlural);
        for (size_t i = 0; i < lemmas.GetLemmaCount(); ++i)
            if (lemmas.GetLemmaLang(i) == LANG_RUS) {
                for (size_t count = lemmas.GetFlexGramCount(i), j = 0; j < count; ++j)
                    if (lemmas.GetFlexGram(i, j).HasAll(specialCase)) {
                        TFioHomonym hom;
                        hom.SurnameHomonym = TFioHomonym::USE_ORIGINAL_TEXT;
                        hom.Grammems.push_back(lemmas.GetStemGram(i) | NSpike::AllCases
                                               | TGramBitSet(gSingular, gMasculine, gFeminine));
                        homonyms.push_back(hom);
                        return;
                    }
            }
    }

    struct TFilterLemma {
        bool operator()(const TGramBitSet& grammems) {
            if (Lang == LANG_RUS) {
                if ((grammems & NSpike::AllGenders) == TGramBitSet(gNeuter))
                    return true;
                if ((grammems & NSpike::AllNumbers) == TGramBitSet(gPlural))
                    return true;
            }
            return false;
        }

        TFilterLemma(ELanguage lang)
            : Lang(lang)
        {
        }

        ELanguage Lang;
    };

    TFioHomonyms GetFioMemberHomonyms(const TInputSymbol* symbol,
                                      size_t TFioHomonym::* field)
    {
        TFioHomonyms homonyms;
        Y_DBGTRACE(VERBOSE, "Fio member #" << (size_t)&(((TFioHomonym*)NULL)->*field) << " is initializing");
        if (symbol) {
            const ILemmas& lemmas = symbol->GetLemmas();
            for (size_t i = 0; i < lemmas.GetLemmaCount(); ++i) {
                TFioHomonym homonym;
                homonym.*field = i;
                const NSpike::TGrammarBunch& bunch = symbol->GetAllGram(i);
                RemoveCopyIf(bunch.begin(), bunch.end(),
                       std::back_inserter(homonym.Grammems),
                       TFilterLemma(lemmas.GetLemmaLang(i)));
                if (!homonym.Grammems.empty()) {
                    homonyms.push_back(homonym);
                } else {
                    Y_DBGTRACE(VERBOSE, "    Homonym " << lemmas.GetLemmaText(i) << "[" << ToStroku(bunch) << "] is rejected");
                }
            }
            GetSpecialGrammarForms(homonyms, symbol->GetLemmas(), field);
            if (homonyms.size() > 1) {
                TCompareHomonymsByText cmp(field, symbol);
                Sort(homonyms.begin(), homonyms.end(), cmp);
                size_t i = 0;
                for (size_t j = 1; j < homonyms.size(); ++j) {
                    if (cmp.Equals(homonyms[i], homonyms[j])) {
                        homonyms[i].Grammems.insert(homonyms[i].Grammems.end(),
                                                    homonyms[j].Grammems.begin(),
                                                    homonyms[j].Grammems.end());
                    } else if (++i != j) {
                        homonyms[i] = homonyms[j];
                    }
                }
                homonyms.erase(homonyms.begin() + i + 1, homonyms.end());
            }
            Y_DBGTRACE(VERBOSE, ToStroku(homonyms, symbol, field));
        } else {
            Y_DBGTRACE(VERBOSE, "    fio member is empty");
            homonyms.resize(1);
            homonyms[0].*field = TFioHomonym::EMPTY;
            homonyms[0].Grammems.push_back(TGramBitSet());
        }
        return homonyms;
    }

    TFioHomonyms AgreeHomonyms(const TFioHomonyms& firstSet,
                               const TFioHomonyms& secondSet)
    {
        TFioHomonyms result;
        for (size_t i = 0; i < firstSet.size(); ++i) {
            for (size_t j = 0; j < secondSet.size(); ++j) {
                TGrammarBunch bunch = FilterGrammars(
                        firstSet[i].Grammems, secondSet[j].Grammems);
                if (bunch.empty())
                    continue;
                TFioHomonym consistentPair = firstSet[i];
                consistentPair.Grammems = bunch;
                AbsorbHomonym(consistentPair, secondSet[j]);
                result.push_back(consistentPair);
            }
        }
        if (result.empty()) { // fail,
            if (secondSet.size() == 1) { // if we have only one variant in secondSet take it
                for (size_t i = 0; i < firstSet.size(); ++i) {
                    TFioHomonym consistentPair = firstSet[i];
                    AbsorbHomonym(consistentPair, secondSet[0]);
                    result.push_back(consistentPair);
                }
            } else if (firstSet.size() == 1) { // if we have only one variant in secondSet take it
                for (size_t i = 0; i < secondSet.size(); ++i) {
                    TFioHomonym consistentPair = secondSet[i];
                    AbsorbHomonym(consistentPair, firstSet[0]);
                    result.push_back(consistentPair);
                }
            }
        }
        return result;
    }

    TFioHomonyms FindConsistentHomonyms(const TInputSymbol* firstName,
                                        const TInputSymbol* secondName,
                                        const TInputSymbol* surname)
    {
        TFioHomonyms firstNames = GetFioMemberHomonyms(firstName, &TFioHomonym::FirstNameHomonym);
        TFioHomonyms secondNames = GetFioMemberHomonyms(secondName, &TFioHomonym::SecondNameHomonym);
        TFioHomonyms surnames = GetFioMemberHomonyms(surname, &TFioHomonym::SurnameHomonym);
        return AgreeHomonyms(AgreeHomonyms(firstNames, secondNames), surnames);
    }

    const TInputSymbol* GetFioMemberSymbol(
            const TVector<NSymbol::TInputSymbol*>& inputSymbols,
            const TFIOOccurenceInText& fioOcc,
            ENameType fioMember)
    {
        const TInputSymbol* symbol = nullptr;
        if (fioOcc.NameMembers[fioMember].IsValidWordNum())
            symbol = inputSymbols[fioOcc.NameMembers[fioMember].WordNum];
        return symbol;
    }

    void SetResultFio(const TInputSymbol* symbol, size_t homonym, int& externHomonym, TString& externLemma) {
        externHomonym = TFioHomonym::IsLemma(homonym) ? homonym : -1;
        if (homonym != TFioHomonym::EMPTY)
            externLemma = WideToChar(TFioHomonym::GetText(homonym, symbol), CODES_YANDEX);
    }

} // namespace

namespace NFioExtractor {

    bool GleicheFio(const TFullFIO& fullFio, const TFIOOccurenceInText& fioOcc,
                    TVector<TFullFIO>& fullFios2, TVector<TFIOOccurenceInText>& fioOccs2,
                    const TVector<NSymbol::TInputSymbol*>& inputSymbols)
    {
        Y_DBGTRACE(VERBOSE, "GleicheFio for '" << CharToWide(fullFio.ToString(), csYandex) << "' started");

        const TInputSymbol* firstName = GetFioMemberSymbol(inputSymbols, fioOcc, FirstName);
        const TInputSymbol* secondName = GetFioMemberSymbol(inputSymbols, fioOcc, MiddleName);
        const TInputSymbol* surname = GetFioMemberSymbol(inputSymbols, fioOcc, Surname);

        TVector<TFioHomonym> fioHomonyms = FindConsistentHomonyms(firstName, secondName, surname);

        Y_DBGTRACE(VERBOSE, "GleicheResult:");

        for (TFioHomonyms::const_iterator hom = fioHomonyms.begin();
                hom != fioHomonyms.end(); ++hom)
        {
            TFullFIO fullFioNew = fullFio;
            TFIOOccurenceInText fioOccNew = fioOcc;
            fioOccNew.Grammems = TGramBitSet::UniteGrammems(hom->Grammems);
            SetResultFio(firstName, hom->FirstNameHomonym, fioOccNew.NameMembers[FirstName].HomNum, fullFioNew.Name);
            SetResultFio(secondName, hom->SecondNameHomonym, fioOccNew.NameMembers[MiddleName].HomNum, fullFioNew.Patronymic);
            SetResultFio(surname, hom->SurnameHomonym, fioOccNew.NameMembers[Surname].HomNum, fullFioNew.Surname);
            if (TFioHomonym::IsLemma(hom->FirstNameHomonym))
                fullFioNew.FirstNameFromMorph = firstName->GetLemmas().GetStemGram(hom->FirstNameHomonym).Test(gFirstName);
            fullFioNew.SurnameLemma = TFioHomonym::IsLemma(hom->SurnameHomonym);
            fullFios2.push_back(fullFioNew);
            fioOccs2.push_back(fioOccNew);
            Y_DBGTRACE(VERBOSE, "  [" << CharToWide(fullFioNew.ToString(), csYandex) << "]");
        }
        return !fioHomonyms.empty();
    }

} // NFioExtractor
