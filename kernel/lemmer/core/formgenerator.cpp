#include "formgenerator.h"
#include "lemmeraux.h"

namespace NLemmer {
    TFormGenerator::TFormGenerator(const TYandexLemma& lemma, const TGrammarFiltr* filter)
        : Filter(filter ? filter->Clone() : new NLemmer::NDetail::TDummyGrammarFiltr)
    {
        Filter->SetLemma(lemma);
        if (Filter->IsProperStemGr()) {
            ItPtr.Reset(new TDefaultWordformIterator(lemma));
            FilterNext();
        }
    }

    bool TFormGenerator::GenerateNext(TYandexWordform& form) {
        if (ItPtr.Get() == nullptr)
            return false;

        IWordformIterator& it = *ItPtr;
        if (!it.IsValid())
            return false;

        it->ConstructForm(form);
        if (it->GetWeight() != 0)
            NLemmerAux::TYandexWordformSetter(form).SetWeight(it->GetWeight());

        TWordformKit orig = *it;
        operator++();

        // append all following forms with same text to @Form as additional flex-grammars
        for (; it.IsValid() && it->HasSameParts(orig); operator++())
            NLemmerAux::TYandexWordformSetter(form).AddFlexGr(it->GetFlexGram());

        return true;
    }

    namespace NDetail {
        static bool InitFlexGrammar(const char* stemGr, const char* needGram, TString& buf) {
            assert(needGram);
            if (!stemGr)
                stemGr = "";
            buf = "";
            for (; *needGram; needGram++) {
                const TGramClass clNeed = GetClass(*needGram);
                const char* gr = stemGr;
                bool bAdd = true;
                for (; *gr; gr++) {
                    const TGramClass cl = GetClass(*gr);
                    if (cl == clNeed) {
                        if (*gr == *needGram) {
                            bAdd = false;
                            break;
                        } else if (cl)
                            return false;
                    }
                }
                if (bAdd)
                    buf += *needGram;
            }
            return true;
        }

        TClueGrammarFiltr::TClueGrammarFiltr(const TString neededGr[]) {
            for (const TString* ps = neededGr; !ps->empty(); ++ps)
                Grammar.push_back(*ps);
            if (Grammar.empty())
                Grammar.push_back("");
        };

        void TClueGrammarFiltr::SetLemma(const TYandexLemma& lemma) {
            FlexGram.clear();
            for (size_t i = 0; i < Grammar.size(); ++i) {
                TString buf;
                if (InitFlexGrammar(lemma.GetStemGram(), Grammar[i].c_str(), buf))
                    FlexGram.push_back(buf);
            }
        }

        bool TClueGrammarFiltr::CheckFlexGr(const char* flexGram) const {
            if (!IsProperStemGr())
                return false;
            for (size_t i = 0; i < FlexGram.size(); ++i) {
                if (!flexGram || !*flexGram) {
                    if (FlexGram[i].empty())
                        return true;
                } else if (NTGrammarProcessing::HasAllGrams(flexGram, FlexGram[i].c_str()))
                    return true;
            }
            return false;
        }

    }
}
