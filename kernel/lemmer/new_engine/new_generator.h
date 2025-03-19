#pragma once

#include "new_lemmer.h"
#include <kernel/lemmer/core/wordform.h>
#include <kernel/lemmer/core/formgenerator.h>
#include <util/generic/noncopyable.h>

class TNewWordformKit: public TWordformKit {
private:
    void SetStem(const TYandexLemma& lemma) {
        Lemma = &lemma;
        size_t stemLen = lemma.GetConvertedFormLength() - lemma.GetPrefLen() - lemma.GetFlexLen();
        StemBase = TWtringBuf(lemma.GetText() + lemma.GetLemmaPrefLen(), stemLen);
        StemFlexion = TWtringBuf();
        Prefix = TWtringBuf();
        InvertedFlexion = TWtringBuf();
        DirectFlexion = TWtringBuf();
        Postfix = TWtringBuf(lemma.GetText() + (lemma.GetTextLength() - lemma.GetSuffixLength()), lemma.GetSuffixLength());
        FlexGrammar = nullptr;
    }

    void SetFlex(const TWtringBuf& flex, const char* flexGrammar) {
        size_t pos = flex.find(TNewLemmer::AffixDelimiter);
        if (Y_LIKELY(pos == TWtringBuf::npos)) {
            Prefix = TWtringBuf();
            DirectFlexion = flex;
        } else {
            Prefix = TWtringBuf(flex.data() + pos + 1, flex.size() - pos - 1);
            DirectFlexion = TWtringBuf(flex.data(), pos);
        }
        InvertedFlexion = TWtringBuf();
        FlexGrammar = flexGrammar;
    }

    void SetFlexGrammar(const char* flexGrammar) {
        FlexGrammar = flexGrammar;
    }

    void SetWeight(ui32 weight) {
        Weight = weight;
    }

    friend class TNewWordformIterator;
};

class TNewWordformIterator: public IWordformIterator, TNonCopyable {
public:
    TNewWordformIterator(const TNewLemmer& lemmer, const TYandexLemma& lemma)
        : Scheme(lemmer.GetData().GetScheme(lemma.GetParadigmId()))
        , CurrentFlex(0)
    {
        CurrentForm.SetStem(lemma);
        const IBlockPtr block = Scheme->GetBlock(CurrentFlex);
        GrammarIterator = block->GetGrammarStringIterator();
        CurrentForm.SetFlex(block->GetFormFlex(), GrammarIterator->Get().AsCharPtr());
        CurrentForm.SetWeight(block->GetFrequency());

        HasNext = block->HasNext();
    }

    // implements IWordformIterator
    inline bool IsValid() const override {
        return CurrentFlex != size_t(-1);
    }

    void operator++() override {
        Y_ASSERT(IsValid());
        GrammarIterator->Next();
        if (GrammarIterator->IsValid()) {
            CurrentForm.SetFlexGrammar(GrammarIterator->Get().AsCharPtr());
        } else {
            if (!HasNext) {
                CurrentFlex = size_t(-1);
                return;
            }
            ++CurrentFlex;
            IBlockPtr block = Scheme->GetBlock(CurrentFlex);
            GrammarIterator = block->GetGrammarStringIterator();
            CurrentForm.SetFlex(block->GetFormFlex(), GrammarIterator->Get().AsCharPtr());
            CurrentForm.SetWeight(block->GetFrequency());
            HasNext = block->HasNext();
        }
    }

    size_t FormsCount() const override {
        size_t formNumber = 0;
        while (Scheme->GetBlock(formNumber)->HasNext()) {
            ++formNumber;
        }
        return formNumber + 1;
    }

private:
    inline const TWordformKit& GetValue() const override {
        return CurrentForm;
    }

private:
    ISchemePtr Scheme;

    size_t CurrentFlex;
    IGrammarStringIteratorPtr GrammarIterator;
    bool HasNext;

    TNewWordformKit CurrentForm;
};

class TNewFormGenerator: public NLemmer::TFormGenerator {
public:
    TNewFormGenerator(const TYandexLemma& lemma, const TNewLemmer* lemmer, const NLemmer::TGrammarFiltr* filter)
        : NLemmer::TFormGenerator(lemma, filter)
    {
        Filter->SetLemma(lemma);
        if (Filter->IsProperStemGr()) {
            if (lemmer != nullptr)
                ItPtr.Reset(new TNewWordformIterator(*lemmer, lemma));
            else
                ItPtr.Reset(new TDefaultWordformIterator(lemma));
            FilterNext();
        }
    }
};
