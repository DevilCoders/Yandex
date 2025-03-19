#pragma once

#include "meta.h"

#include <kernel/catboost/catboost_calcer.h>
#include <kernel/extended_mx_calcer/interface/calcer_exception.h>
#include <kernel/extended_mx_calcer/interface/extended_relev_calcer.h>
#include <kernel/extended_mx_calcer/factory/factory.h>
#include <kernel/extended_mx_calcer/proto/typedefs.h>

#include <kernel/formula_storage/formula_storage.h>
#include <kernel/formula_storage/loader.h>
#include <kernel/formula_storage/shared_formulas_adapter/shared_formulas_adapter.h>

#include <library/cpp/scheme/scheme.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/strbuf.h>


namespace NExtendedMx {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// some typedefs, constants and helper functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    using TFactors = TVector<TVector<float>>;
    using TCategoricalFactors = TVector<TVector<TString>>;

    extern const TString FEATURE_AVAIL_PREF;
    extern const TString FEATURE_WEIGHT_PREF;
    extern const TString FEATURE_LOG_PREF;

    const TMeta& GetTargetMeta(NMatrixnet::IRelevCalcer* const calcer);
    const TMeta& GetTargetMeta(TExtendedRelevCalcer* const calcer);
    TString GetAliasFromBundle(const TBundleConstProto& bundle);
    double ApplyResultTransform(double res, const TResultTransformConstProto& rtp);
    void ApplyResultTransform(TVector<double>& res, const TResultTransformConstProto& rtp);
    size_t NumFeatsWithOffset(const size_t totalFeats, const size_t offset);
    size_t StepToPos(float step);
    void ProcessStepAsPos(const TExtendedRelevCalcer& calcer, TCalcContext& context, float step, const TStringBuf& stepName);
    TVector<const float*> CastTFactorsToConstFloatPtr(const TFactors& features);
    TVector<TConstArrayRef<float>> CastTFactorsToConstArrayRef(const TFactors& features);
    TVector<TVector<TStringBuf>> CastTCategoricalFactorsToTStringBufVector(const TCategoricalFactors& catFeatures);
    void CalcMultiple(
            const TVector<const NCatboostCalcer::TCatboostCalcer*>& calcers,
            const TVector<TConstArrayRef<float>>& fPtrs,
            const TVector<TVector<TStringBuf>>& catFeaturesBufs,
            TVector<TVector<double>>& results);



    template <typename TKey, typename TValue>
    void LogValue(const TExtendedRelevCalcer& calcer, TCalcContext& ctx, const TKey& key, const TValue& value) {
            ctx.Log(calcer.GetAlias(), key, value);
    }

    template <typename T>
    void DebugFactorsDump(const TString& title, const TVector<TVector<T>>& factors, TCalcContext& ctx) {
        if (ctx.DbgLog().IsEnabled()) {
            const auto rows = factors.size();
            const auto cols = rows ? factors[0].size() : 0;
            ctx.DbgLog() << title << "\trows: " << rows << "\tcols: " << cols << "\n";
            for (const auto& row : factors) {
                ctx.DbgLog() << JoinVectorIntoString(row, "\t") << "\n";
            }
        }
    }

    template <typename TCalcerPtr>
    void CalcMultiple(
            const TVector<TCalcerPtr>& calcers, const TVector<const float*>& fPtrs,
            TVector<TVector<double>>& results) {
        results.resize(calcers.size());
        for (size_t i = 0; i < calcers.size(); ++i) {
            calcers[i]->CalcRelevs(fPtrs, results[i]);
        }
    }

    template <typename TCalcerPtr>
    void CalcMultiple(
            const TVector<TCalcerPtr>& calcers, const TFactors& features, const TCategoricalFactors& catFeatures,
            TVector<TVector<double>>& results) {
        if (catFeatures.empty() || catFeatures[0].empty()) {
            CalcMultiple(calcers, CastTFactorsToConstFloatPtr(features), results);
        } else {
            TVector<const NCatboostCalcer::TCatboostCalcer*> catboostCalcers;
            catboostCalcers.reserve(calcers.size());
            for (const auto& calcer : calcers) {
                auto catboostCalcer = NCommonHelpers::DynamicCastHelper<const NCatboostCalcer::TCatboostCalcer *>(calcer);
                Y_ENSURE(catboostCalcer, "Can use categorical features only with CatBoost model");
                catboostCalcers.push_back(catboostCalcer);
            }

            CalcMultiple(
                catboostCalcers,
                CastTFactorsToConstArrayRef(features),
                CastTCategoricalFactorsToTStringBufVector(catFeatures),
                results);
        }
    }

    template <typename TCalcerPtr>
    void CalcMultiple(
            const TVector<TCalcerPtr>& calcers, const TFactors& features, TVector<TVector<double>>& results) {
        TCategoricalFactors emptyCategoricalFactors(features.size());
        CalcMultiple(calcers, features, emptyCategoricalFactors, results);
    }

    template <typename TTo, typename TFrom>
    THolder<TTo> DynamicCastPtr(THolder<TFrom> &&ptr) {
        TTo *obj = dynamic_cast<TTo*>(ptr.Get());
        THolder<TTo> res(obj);
        if (obj != nullptr)
            ptr.Release();
        return res;
    }

    template <typename TTo, typename TFrom>
    THolder<TTo> DynamicCastPtrEnsure(THolder<TFrom> &&ptr) {
        TTo *obj = dynamic_cast<TTo*>(ptr.Get());
        Y_ENSURE(obj, "bad dynamic cast");
        THolder<TTo> res(obj);
        return res;
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TBundleBase - helper to work with scheme proto in derived class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename TSchemeProto>
    class TBundleBase : public TBasedOn<TSchemeProto>, public TExtendedRelevCalcer {
        using TBasedOnParent = TBasedOn<TSchemeProto>;
    protected:
        TBundleBase(const NSc::TValue& scheme)
            : TBasedOnParent(scheme)
            , BundleProto_(TBasedOnParent::Scheme().Value__)
            , Alias_(GetAliasFromBundle(BundleProto_))
        {
            TBasedOnParent::ValidateProtoThrow();
            UpdateMetaAndInfo(BundleProto_);
        }


        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Load formula helper
        //    Bin node may contain either:
        //      * string with base64 encoded formula
        //      * dict with valid TBundleProto json
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        template <typename TBaseCalcer, typename TDerivedCalcer = TBaseCalcer>
        TDerivedCalcer* LoadFormulaProto(const TFmlConstProto& fmlProto, const ISharedFormulasAdapter* const formulasStoragePtr = nullptr) {
            Y_ENSURE(InitStage == EInitStage::InitInProcess, "it is forbidden to load subformulas before or after InitializeAfterStorageLoad-stage");
            auto calcer = LoadFormulaProtoImpl<TBaseCalcer>(fmlProto, formulasStoragePtr);
            UpdateMetaAndInfo(calcer);
            return NCommonHelpers::DynamicCastHelper<TDerivedCalcer*>(calcer);
        }

        TExtendedRelevCalcer* LoadFormulaProto(const TBundleConstProto& bundleProto, const ISharedFormulasAdapter* const formulasStoragePtr) {
            Y_ENSURE(InitStage == EInitStage::InitInProcess, "it is forbidden to load subformulas before or after InitializeAfterStorageLoad-stage");
            TExtendedLoaderFactory factory;
            TAtomicSharedPtr<TExtendedRelevCalcer> calcer = factory.Load(bundleProto);
            TryInitializeSubcalcer(calcer, "<inner bundle>", formulasStoragePtr);
            AddChildCalcer(calcer);
            UpdateMetaAndInfo(calcer.Get());
            return calcer.Get();
        }

        template <typename TKey, typename TValue>
        void LogValue(TCalcContext& ctx, const TKey& key, const TValue& value) const {
            NExtendedMx::LogValue(*this, ctx, key, value);
        }

        void EnsureFeatureCount(const size_t featureCount) const {
            Y_ENSURE(GetNumFeats() <= featureCount, TStringBuilder() << "not enough features for calculation " << GetNumFeats() << ">" << featureCount);
        }

        const TString& GetAlias() const override {
            return Alias_;
        }

        const NMatrixnet::TModelInfo* GetInfo() const final {
            return &ModelInfo_;
        }

        const NSc::TValue& ViewRoot() const override {
            return *TBasedOnParent::Scheme().Value__;
        }

        THolder<TExtendedRelevCalcer> CloneAndOverrideParams(const NSc::TValue& patch, TCalcContext& ctx, const ISharedFormulasAdapter* const formulasStoragePtr) const override final {
            TExtendedLoaderFactory factory;
            NSc::TValue patched = BundleProto_.GetRawValue()->Clone();
            patched.MergeUpdate(patch);
            if (ctx.DbgLog().IsEnabled()) {
                ctx.DbgLog() << "Make patched bundle: " << patched << '\n';
            }
            THolder<TExtendedRelevCalcer> cloned = factory.Load(TBundleConstProto(&patched));
            cloned->Initialize(formulasStoragePtr);
            LogValue(ctx, "patched", "1");
            return cloned;
        }

    private:
        template <typename TBaseCalcer>
        THolder<TBaseCalcer> LoadFormulaBase64Impl(const TString& fmlName, const TStringBuf fmlB64) {
            Y_ENSURE(fmlName, "empty fml name");
            Y_ENSURE(fmlB64, "empty base64 formula");
            TStringStream ss = Base64Decode(fmlB64);
            return LoadFormula<TBaseCalcer>(fmlName, &ss);
        }

        template <typename TBaseCalcer>
        TBaseCalcer* LoadFormulaProtoImpl(const TFmlConstProto& fmlProto, const ISharedFormulasAdapter* const formulasStoragePtr = nullptr) {
            const auto& bin = *fmlProto.Bin();
            const auto& name = TString{*fmlProto.Name()};
            TAtomicSharedPtr<NMatrixnet::IRelevCalcer> calcer;
            if (bin.IsString()) {
                if (bin.GetString()) {
                    calcer = LoadFormulaBase64Impl<TBaseCalcer>(name, bin.GetString());
                } else {
                    Y_ENSURE(formulasStoragePtr, "haven't got formulas storage to load ready formulas from");
                    const auto formulaName = NCommonHelpers::GetFormulaNameFromPath(TFsPath(name));
                    calcer = formulasStoragePtr->GetSharedFormula(formulaName);
                    AddStorageChildCalcerName(formulaName);
                }
            } else if (bin.IsDict()) {
                TExtendedLoaderFactory factory;
                calcer = factory.Load(TBundleConstProto(&bin));
            } else {
                ythrow yexception() << "unknown \"Bin\" value, expected base64 or dict, get: " << bin;
            }

            if (TBaseCalcer* calcerRawPtr = dynamic_cast<TBaseCalcer*>(calcer.Get())) {
                TryInitializeSubcalcer(calcer, name, formulasStoragePtr);
                AddChildCalcer(calcer);
                return calcerRawPtr;
            } else {
                InitStage = EInitStage::NoInit;
                ythrow TCalcerException(TCalcerException::GetCalcerNotFoundMessage()).AddCalcerToErroredChain(name);
            }
        }

        void UpdateMetaAndInfo(const TBundleConstProto& bundle) {
            GetMeta().MergeUpdate(bundle);
            UpdateModelInfo();
        }

        template <typename TCalcer>
        void UpdateMetaAndInfo(TCalcer* const calcer) {
            GetMeta().MergeUpdate(GetTargetMeta(calcer));
            UpdateModelInfo();
        }

        void UpdateModelInfo() {
            for (const auto& kv : GetMeta()) {
                ModelInfo_[kv.first] = kv.second.ToJson();
            }
        }

    private:
        TBundleConstProto BundleProto_;
        const TString Alias_;
        NMatrixnet::TModelInfo ModelInfo_;
    };

} // NExtendedMx
