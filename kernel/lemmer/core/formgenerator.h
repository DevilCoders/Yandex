#pragma once

#include "lemmer.h"
#include <util/generic/string.h>
#include "wordform.h"

namespace NLemmer {
    class TGrammarFiltr {
    public:
        virtual void SetLemma(const TYandexLemma& lemma) = 0;
        virtual bool IsProperStemGr() const = 0;
        virtual bool CheckFlexGr(const char* flexGram) const = 0;
        virtual TGrammarFiltr* Clone() const = 0;
        virtual ~TGrammarFiltr() {
        }
    };

    class TFormGenerator {
    public:
        TFormGenerator(const TYandexLemma& lemma, const TGrammarFiltr* filter = nullptr);

        // Iterator interface --------------------

        inline bool IsValid() const {
            return ItPtr.Get() != nullptr && ItPtr->IsValid();
        }

        inline void operator++() {
            ItPtr->operator++();
            FilterNext();
        }

        inline const TWordformKit& operator*() const {
            return ItPtr->operator*();
        }

        inline const TWordformKit* operator->() const {
            return ItPtr->operator->();
        }

        // Generator interface --------------------

        bool GenerateNext(TYandexWordform& form);

        inline size_t FormsCount() const {
            return ItPtr.Get() != nullptr ? ItPtr->FormsCount() : 0;
        }

    protected:
        void FilterNext() {
            while (ItPtr->IsValid() && !Filter->CheckFlexGr((*ItPtr)->GetFlexGram()))
                ItPtr->operator++();
        }

        THolder<TGrammarFiltr> Filter;
        THolder<IWordformIterator> ItPtr;
    };

    namespace NDetail {
        class TDummyGrammarFiltr: public NLemmer::TGrammarFiltr {
        public:
            TDummyGrammarFiltr()
                : IsProper(false)
            {
            }
            void SetLemma(const TYandexLemma&) override {
                IsProper = true;
            }
            bool IsProperStemGr() const override {
                return IsProper;
            }
            bool CheckFlexGr(const char*) const override {
                return IsProper;
            }
            TDummyGrammarFiltr* Clone() const override {
                return new TDummyGrammarFiltr(*this);
            }
            ~TDummyGrammarFiltr() override{};

        private:
            bool IsProper;
        };

        class TClueGrammarFiltr: public NLemmer::TGrammarFiltr {
        public:
            TClueGrammarFiltr(const TString neededGr[]);
            void SetLemma(const TYandexLemma& lemma) override;
            bool IsProperStemGr() const override {
                return !FlexGram.empty();
            }
            bool CheckFlexGr(const char* flexGram) const override;
            TClueGrammarFiltr* Clone() const override {
                return new TClueGrammarFiltr(*this);
            }
            ~TClueGrammarFiltr() override {
            }

        private:
            TVector<TString> Grammar;
            TVector<TString> FlexGram;
        };

    }
}
