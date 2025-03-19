#pragma once

#include "weighter_decl.h"

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/factors/factor_storage.h>
#include <kernel/snippets/formulae/formula.h>
#include <kernel/snippets/sent_info/sentword.h>
#include <kernel/snippets/span/span.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/random/mersenne.h>
#include <utility>

namespace NSnippets
{
    class TConfig;
    class TWordStat;
    class TSentsMatchInfo;
    class TSnipTitle;
    class TWordSpanLen;

    class TFactorsCalcer : private TNonCopyable
    {
    private:
        struct TImpl;

        THolder<TImpl> Impl;

    public:
        TFactorsCalcer(const TSentsMatchInfo& info, const TConfig& cfg, const TString& url, const TWordSpanLen& wordSpanLen, const TSnipTitle* title, float maxLenForLPFactors, const TSchemaOrgArchiveViewer* schemaOrgViewer = nullptr, bool ignoreWidthOfScreen = false);
        ~TFactorsCalcer();

        const TWordStat& GetWordStat() const;
        void CalcAll(TFactorStorage& x, const TSpans& ws, const TSpans& ss);
    };

    class TWeighter
    {
    public:
        typedef std::pair<TSentMultiword, TSentMultiword> TFragment;
        typedef std::pair<int, int> TFragmentII;

    private:
        TFormula Formula;
        const TSentsMatchInfo& Info;
        TSpans WSpans;
        TSpans SSpans;
        TFactorsCalcer FCalc;
        TFactorStorage128 FStore;
        // SNIPPETS-7241
        THolder<TMersenne<ui32>> FormulaDegrader;
        ui32 FormulaDegradeThreshold = 0; // percents
        bool SingleFragmentMode = false; // we don't want to degrade single fragments for multi-fragment candidates
        static constexpr double FormulaDegradeValue = 1000000;

    public:
        TWeighter(const TSentsMatchInfo& info, const TConfig& cfg, const TWordSpanLen& wordSpanLen, const TSnipTitle* title, float maxLenForLPFactors, const TString& url = TString(), const TSchemaOrgArchiveViewer* schemaOrgViewer = nullptr, bool isFactSnippet = false);

        size_t GetUsed() const {
            return FStore.GetUsed();
        }
        bool IsFull() const {
            return FStore.GetUsed() == TFactorStorage128::MAX;
        }
        void BatchCalc(double (&res)[TFactorStorage128::MAX]) {
            Formula.BatchCalc(FStore, res);
            if (Y_UNLIKELY(FormulaDegradeThreshold) && !SingleFragmentMode) {
                for (size_t i = 0; i < FStore.GetUsed(); i++) {
                    if (FormulaDegrader->Uniform(100) < FormulaDegradeThreshold) {
                        res[i] -= FormulaDegradeValue;
                    }
                }
            }
        }
        double GetWeight() {
            Y_ASSERT(GetUsed() == 1);
            double weight = Formula.GetWeight(FStore[0]);
            if (Y_UNLIKELY(FormulaDegradeThreshold) && !SingleFragmentMode && FormulaDegrader->Uniform(100) < FormulaDegradeThreshold) {
                weight -= FormulaDegradeValue;
            }
            return weight;
        }

        const TFactorStorage& GetFactors(size_t i) const {
            return FStore[i];
        }
        TFactorStorage& GetFactors(size_t i) {
            return FStore[i];
        }
        const TFactorStorage& GetFactors() const {
            Y_ASSERT(GetUsed() == 1);
            return FStore[0];
        }
        TFactorStorage& GetFactors() {
            Y_ASSERT(GetUsed() == 1);
            return FStore[0];
        }

        void Reset()
        {
            FStore.Reset();
            Clear();
        }

        void SetSpan(const TSentMultiword& i, const TSentMultiword& j)
        {
            Clear();
            SetFragment(i, j);
            CalcAll();
        }

        void SetSpan(int i, int j)
        {
            Clear();
            SetFragment(i, j);
            CalcAll();
        }

        void SetSpans(const TVector<TFragmentII> &spans)
        {
            Clear();
            for (size_t i = 0; i < spans.size(); ++i) {
                SetFragment(spans[i].first, spans[i].second);
            }
            CalcAll();
        }

        void SetSpans(const TVector<TFragment>& spans)
        {
            Clear();
            for (TVector<TFragment>::const_iterator it = spans.begin(); it != spans.end(); ++it) {
                SetFragment(it->first, it->second);
            }
            CalcAll();
        }

        const TWordStat& GetWordStat() const
        {
            return FCalc.GetWordStat();
        }

        void InitFormulaDegrade(ui32 threshold, ui32 seed) {
            FormulaDegrader = MakeHolder<TMersenne<ui32>>(seed);
            FormulaDegradeThreshold = threshold;
        }
        void SetSingleFragmentMode(bool isSingleFragment) {
            Y_ASSERT(GetUsed() == 0); // should not be called within batches
            SingleFragmentMode = isSingleFragment;
        }

    private:
        void SetFragment(int i, int j);
        void SetFragment(const TSentMultiword& i, const TSentMultiword& j);
        void Clear();
        TFactorStorage& Next();
        void CalcAll();
    };

}

