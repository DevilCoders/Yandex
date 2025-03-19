#pragma once

#include "rank_models_factory_fwd.h"
#include "relev_fml.h"

#include <kernel/catboost/catboost_calcer.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/dssm_applier/optimized_model/optimized_model_applier.h>
#include <kernel/dssm_applier/query_word_title/dictionary.h>
#include <kernel/bundle/bundle.h>
#include <kernel/matrixnet/mn_dynamic.h>
#include <kernel/matrixnet/mn_sse.h>

#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>

#include <util/generic/map.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/memory/blob.h>

/// Hold information about ranking models.
class TRankModel {
public:
    TRankModel();
    explicit TRankModel(const NMatrixnet::TRelevCalcerPtr &matrixnet);
    explicit TRankModel(const TPolynomDescrStatic &polynom);
    TRankModel(const NMatrixnet::TRelevCalcerPtr &matrixnet, const TPolynomDescrStatic &polynom);

    void SetMatrixnet(const NMatrixnet::TRelevCalcerPtr &matrixnet);
    void SetPolynom(const TPolynomDescrStatic &polynom);
    void DisableMatrixnet();
    void EnableMatrixnet();

    bool HasMatrixnet() const;
    bool HasPolynom() const;
    const NMatrixnet::IRelevCalcer* Matrixnet() const;

    // TODO: these two functions are wrappers for dynamic_casts. Most cases of their usage
    // should be rewritten to virtual functions in IRelevCalcer.
    // See https://st.yandex-team.ru/SEARCH-2223
    const NMatrixnet::TMnSseInfo* MnSseInfo() const;
    const NMatrixnet::TBundle* Bundle() const;

    NMatrixnet::TRelevCalcerPtr MatrixnetShared() const;
    const TPolynomDescrStatic* Polynom() const;
    bool Complete() const;

private:
    NMatrixnet::TRelevCalcerPtr MatrixnetPtr;
    bool MatrixnetEnabled;
    TPolynomDescrStatic PolynomData;
    bool PolynomEnabled;
};

/// Ranking model wtih it's name.
struct TRankModelDescr: public TRankModel {
public:
    TString Name;

    TRankModelDescr() = default;

    TRankModelDescr(const TRankModel &m, const char *name)
        : TRankModel(m)
        , Name(name)
    {
    }
};

class IRankModelsFactory {
public:
    using TModelMapping = THashMap<TString, TString>;
    using TModelMappings = THashMap<ui32, TModelMapping>;
    using TRankModelVariant = std::variant<NMatrixnet::TMnSsePtr, NMatrixnet::TBundleDescription>;

    virtual ~IRankModelsFactory() = default;

    /// Get ranking model with information about enabled experiments
    virtual NMatrixnet::TRelevCalcerPtr GetModel(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const = 0;

    /// Get matrixnet with information about enabled experiments
    virtual NMatrixnet::TMnSsePtr GetMatrixnet(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const = 0;

    virtual void SetDssmModel(const TString& /*name*/, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& /*dssmModel*/) {
    };
    virtual const NNeuralNetApplier::TModel* GetDssmModel(const TStringBuf /*name*/) const {
        return nullptr;
    }

    virtual void SetOptimizedModel(const NBuildModels::NProto::EApplyMode /*applyMode*/, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& /*dssmModel*/) {
    };
    virtual const NOptimizedModel::TOptimizedModelApplier* GetOptimizedModel(const NBuildModels::NProto::EApplyMode /*applyMode*/) const {
        return nullptr;
    }

    virtual void SetQueryWordDictionary(const TString& /*name*/, const TSimpleSharedPtr<NQueryWordTitle::TQueryWordDictionary>& /*dictionary*/) {
    };
    virtual const NQueryWordTitle::TQueryWordDictionary* GetQueryWordDictionary(const TString& /*name*/) const {
        return nullptr;
    }

    virtual const NVowpalWabbit::TModel* GetVowpalWabbitModel(const TString& /*name*/) const {
        return nullptr;
    };
    virtual void SetVowpalWabbitModel(const TString& /*name*/, const TSimpleSharedPtr<NVowpalWabbit::TModel>& /*vowpalWabbitModel*/) {
    }

    virtual const NCatboostCalcer::TCatboostCalcer* GetCatboost(const TString& /*name*/) const {
        return nullptr;
    }
    virtual void SetCatboost(const TString& /*name*/, const NCatboostCalcer::TCatboostCalcerPtr& /*model*/) {
    }

    virtual NMatrixnet::TBundlePtr GetBundle(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const {
        Y_UNUSED(name);
        Y_UNUSED(exps);
        Y_UNUSED(actualName);

        return NMatrixnet::TBundlePtr();
    }

    virtual NMatrixnet::TRelevCalcerPtr BuildModel(const TRankModelVariant* /*variant*/, const TSet<TString>& /*exps*/) const {
        return NMatrixnet::TRelevCalcerPtr();
    }

    /// Get optional subfactory by name
    virtual const IRankModelsFactory* GetSubFactory(const TString& /*name*/) const {
        return nullptr;
    }

    virtual const TString* GetModelNameById(const TString& /*id*/) const {
        return nullptr;
    }

    virtual const TModelMappings* GetModelMappings() const {
        return nullptr;
    }

    virtual bool HasModelMappingsRevision(ui32 /*revision*/) const {
        return false;
    }

    virtual bool HasModelMappingsDefaultRevision() const {
        return false;
    }

    virtual const TModelMapping* GetModelMappingAtRevisionOrDefault(ui32 /*revision*/) const {
        return nullptr;
    }
};

class TRankModelsSingleFactory : public IRankModelsFactory {
public:
    TRankModelsSingleFactory() = default;

    explicit TRankModelsSingleFactory(const NMatrixnet::TMnSsePtr& model)
        : Model(model) {}

    void SetMatrixnet(const TStringBuf&, const NMatrixnet::TMnSsePtr& model) {
        Model = model;
    }

    NMatrixnet::TRelevCalcerPtr GetModel(const TStringBuf&, const TSet<TString>& = TSet<TString>(), TString* = nullptr) const override {
        return NMatrixnet::TRelevCalcerPtr(Model);
    }

    NMatrixnet::TMnSsePtr GetMatrixnet(const TStringBuf&, const TSet<TString>& = TSet<TString>(), TString* = nullptr) const override {
        return Model;
    }

private:
    NMatrixnet::TMnSsePtr Model;
};

class TRankModelsMapFactory : public IRankModelsFactory {
public:
    using TRankModelVariant = IRankModelsFactory::TRankModelVariant;
    using TRankModels = THashMap<TString, TRankModelVariant>;
    using const_iterator = TRankModels::const_iterator;
    using TSubFactories = THashMap<TString, TAtomicSharedPtr<IRankModelsFactory> >;
    using TDssmModels = THashMap<TString, TSimpleSharedPtr<NNeuralNetApplier::TModel>>;
    using TVowpalWabbitModels = THashMap<TString, TSimpleSharedPtr<NVowpalWabbit::TModel> >;
    using TQueryWordDictionaries = THashMap<TString, TSimpleSharedPtr<NQueryWordTitle::TQueryWordDictionary>>;
    using TCatboostModels = THashMap<TString, NCatboostCalcer::TCatboostCalcerPtr>;

public:
    NMatrixnet::TRelevCalcerPtr GetModel(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const override;
    NMatrixnet::TRelevCalcerPtr GetModelByID(const TString& id) const;

    void SetMatrixnet(const TStringBuf& name, const NMatrixnet::TMnSsePtr& model);

    NMatrixnet::TMnSsePtr GetMatrixnet(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const override;

    NMatrixnet::TMnSsePtr GetMatrixnetByID(const TString& id) const;

    const_iterator begin() const {
        return Models.begin();
    }

    const_iterator end() const {
        return Models.end();
    }

    void FreezeBundles();

    NMatrixnet::TRelevCalcerPtr BuildModel(const TRankModelVariant* variant, const TSet<TString>& exps = TSet<TString>()) const override;
    void SetBundle(const TStringBuf& name, const NMatrixnet::TBundleDescription& bundle);

    NMatrixnet::TBundlePtr GetBundle(const TStringBuf& name, const TSet<TString>& exps = TSet<TString>(), TString* actualName = nullptr) const override;

    const NNeuralNetApplier::TModel* GetDssmModel(const TStringBuf name) const override;
    void SetDssmModel(const TString& name, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& dssmModel) override;

    const NOptimizedModel::TOptimizedModelApplier* GetOptimizedModel(const NBuildModels::NProto::EApplyMode applyMode) const override;
    void SetOptimizedModel(const NBuildModels::NProto::EApplyMode applyMode, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& dssmModel) override;

    const NVowpalWabbit::TModel* GetVowpalWabbitModel(const TString& name) const override;
    void SetVowpalWabbitModel(const TString& name, const TSimpleSharedPtr<NVowpalWabbit::TModel>& vowpalWabbitModel) override;

    const NCatboostCalcer::TCatboostCalcer* GetCatboost(const TString& name) const override;
    void SetCatboost(const TString& name, const NCatboostCalcer::TCatboostCalcerPtr& model) override;

    const IRankModelsFactory* GetSubFactory(const TString& name) const override {
        const auto it = SubFactories.find(name);
        return it == SubFactories.end() ? nullptr : it->second.Get();
    }

    void SetSubFactory(const TString& name, TAtomicSharedPtr<IRankModelsFactory> factory) {
        SubFactories[name] = factory;
    }

    void SetQueryWordDictionary(const TString& name, const TSimpleSharedPtr<NQueryWordTitle::TQueryWordDictionary>& dictionary) override;
    const NQueryWordTitle::TQueryWordDictionary* GetQueryWordDictionary(const TString& name) const override;

    void SetModelMapping(ui32 revision, const TString& model, const TString& modelName) {
        ModelMappings[revision][model] = modelName;
    }

    bool HasModelMappingsRevision(ui32 revision) const override {
        return ModelMappings.contains(revision);
    }

    bool HasModelMappingsDefaultRevision() const override {
        return ModelMappings.contains(0);
    }

    const TModelMapping* GetModelMappingAtRevisionOrDefault(ui32 revision) const override {
        if (HasModelMappingsRevision(revision)) {
            return &ModelMappings.at(revision);
        }
        if (HasModelMappingsDefaultRevision()) {
            return &ModelMappings.at(0);
        }
        return nullptr;
    }

    const TModelMappings* GetModelMappings() const override {
        return &ModelMappings;
    }

private:
    typedef THashMap<const NMatrixnet::TMnSseInfo*, size_t> TMatrixnetPositions;
    typedef THashSet<const NMatrixnet::TBundleDescription*> TBundleSet;

    void SetModelVariant(const TStringBuf& name, const TRankModelVariant& variant, const TString* id = nullptr);
    void SetModelVariantSpecific(const TString& name, const TRankModelVariant& variant, const TString* id = nullptr);

    template <class T = void>
    const TRankModelVariant* GetModelVariant(const TStringBuf& name, const TSet<TString>& exps, TString* actualName = nullptr) const;
    template <class T = void>
    const TRankModelVariant* GetModelVariantByName(const TStringBuf& name, const TSet<TString>& exps, TString* actualName = nullptr) const;
    const TRankModelVariant* GetModelVariantByID(const TString& id) const;
    template <class T = void>
    const TRankModelVariant* GetModelVariantSpecific(const TString& name, TString* actualName = nullptr) const;

    NMatrixnet::TMnSsePtr BuildMatrixnet(const TRankModelVariant* variant) const;

    NMatrixnet::TBundlePtr BuildBundle(const TRankModelVariant* variant,
                                       const TSet<TString>&) const;
    NMatrixnet::TBundlePtr BuildBundle(const TRankModelVariant* variant,
                                       const TSet<TString>&,
                                       TBundleSet& visited) const;
    void BuildBundle(const NMatrixnet::TBundleDescription* description,
                     const TSet<TString>& exps,
                     NMatrixnet::TRankModelVector& matrixnets,
                     TMatrixnetPositions& positions,
                     TBundleSet& visited) const;
    void BuildBundle(const NMatrixnet::TMnSsePtr& matrixnet,
                     const NMatrixnet::TBundleRenorm renorm,
                     NMatrixnet::TRankModelVector& matrixnets,
                     TMatrixnetPositions& positions) const;

    virtual const TString* GetModelNameById(const TString& id) const override {
        return MapFindPtr(ModelNamesById, id);
    }

    NMatrixnet::TBundlePtr TryGetBundle(const TRankModelVariant* variant) const;

private:
    TRankModels Models;
    TRankModels ModelsByID;
    THashMap<const NMatrixnet::TBundleDescription*, NMatrixnet::TBundlePtr> BuiltBundles;
    TSubFactories SubFactories;
    TDssmModels DssmModels;
    THashMap<NBuildModels::NProto::EApplyMode, TSimpleSharedPtr<NOptimizedModel::TOptimizedModelApplier>> OptimizedModels;
    TVowpalWabbitModels VowpalWabbitModels;
    THashMap<TString, TString> ModelNamesById;
    TQueryWordDictionaries QueryWordDictionaries;
    TCatboostModels CatboostModels;


    /*
        ModelMappings. THashMap<ui32, THashMap<TString, TString>>

        ui32 (id, 0 for default mapping)
        to
        FormulaMapping (Formula index (in dot format) to FormulaID)
    */
    TModelMappings ModelMappings;
};
