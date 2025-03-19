#pragma once

#include "context.h"

#include <kernel/extended_mx_calcer/proto/scheme.sc.h>
#include <kernel/matrixnet/relev_calcer.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/ptr.h>


namespace NSc {
    class TValue;
}

namespace NExtendedMx {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TExtendedRelevCalcer - base class of all extended calcers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class TExtendedRelevCalcer : public NMatrixnet::IRelevCalcer {
    public:
        using TCalcerPtr = THolder<NMatrixnet::IRelevCalcer>;
        using TCalcerSharedPtr = TAtomicSharedPtr<NMatrixnet::IRelevCalcer>;
        using TCalcerConstSharedPtr = TAtomicSharedPtr<const NMatrixnet::IRelevCalcer>;
        using TCalcerConstSharedPtrs = TVector<TCalcerConstSharedPtr>;

    public:
        static THolder<TExtendedRelevCalcer> Create(TCalcerPtr);

        double DoCalcRelev(const float*) const override;

        virtual double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const = 0;
        double CalcRelevExtended(const TVector<float> &features, TCalcContext& context) const;

        void CalcRelevsExtended(const TVector< TVector<float> > &features, TVector<double> &resultRelev, TCalcContext& context) const {
            Y_UNUSED(features);
            Y_UNUSED(context);
            resultRelev.assign(features.size(), 0);
        }

        virtual double RecalcRelevExtended(TCalcContext& context, const NSc::TValue& recalcParams) {
            Y_UNUSED(context);
            Y_UNUSED(recalcParams);
            ythrow yexception() << "Recalc method unimplemented";
            return 0.;
        };

        const TMeta& ViewMeta() const;
        virtual const TString& GetAlias() const;
        virtual const NSc::TValue& ViewRoot() const;

        void Initialize(const ISharedFormulasAdapter* const formulasStoragePtr = nullptr) override;
        void GetStorageChildrenCalcersNames(TSet<TString>& storageChildrenCalcersNames) const;
        virtual THolder<TExtendedRelevCalcer> CloneAndOverrideParams(const NSc::TValue& paramsUpdate, TCalcContext& ctx, const ISharedFormulasAdapter* const formulasStoragePtr = nullptr) const = 0;


    protected:
        TMeta& GetMeta();
        void TryInitializeSubcalcer(const TCalcerSharedPtr& subcalcer, const TString& subcalcerName, const ISharedFormulasAdapter* const formulasStoragePtr);
        void AddChildCalcer(TCalcerConstSharedPtr childCalcer);
        void AddStorageChildCalcerName(const TString& storageChildCalcerName);

        enum class EInitStage {
            NoInit,
            InitInProcess,
            InitDone
        };
        EInitStage InitStage = EInitStage::NoInit;

    private:
        virtual void InitializeAfterStorageLoad(const ISharedFormulasAdapter* const) {
        }

        TMeta Meta;
        TCalcerConstSharedPtrs ChildrenCalcers;
        TVector<TString> StorageChildrenCalcersNames;
    };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TExtendedRelevCalcerSimple - simple implementation
//                              to hold standart .info and etc formulas
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class TExtendedRelevCalcerSimple final : public TExtendedRelevCalcer {
    public:
        TExtendedRelevCalcerSimple(TCalcerPtr calcer);

        double DoCalcRelev(const float* features) const override;
        void DoCalcRelevs(const float* const* docsFeatures, double* resultRelev, const size_t numDocs) const override;
        void DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const override;

        double DoCalcRelevExtended(const float* features, const size_t featuresCount, TCalcContext& context) const override;

        size_t GetNumFeats() const override;
        const NMatrixnet::TModelInfo* GetInfo() const override;
        const TCalcerPtr::TValueType* Get() const;
        TCalcerPtr::TValueType* Get();

        THolder<TExtendedRelevCalcer> CloneAndOverrideParams(const NSc::TValue&, TCalcContext&, const ISharedFormulasAdapter* const) const override final {
            ythrow yexception() << "TExtendedRelevCalcerSimple does not support cloning";
        }

    private:
        TCalcerPtr Calcer;
    };

} // NExtendedMx
