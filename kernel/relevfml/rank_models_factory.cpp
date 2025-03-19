#include "rank_models_factory.h"

#include <util/generic/vector.h>
#include <util/folder/filelist.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/ysaveload.h>


namespace {
    void ParseModelName(TStringBuf name, TString& mainName, TVector<TString>& exps) {
        const TString experiments(name.SplitOff(':').RSplitOff(':'));
        mainName = name;
        exps.clear();
        if (!experiments.empty()) {
            StringSplitter(experiments).Split('.').SkipEmpty().Collect(&exps);
        }
    }

    bool ModelNameIsID(const TStringBuf& name) {
        return (name.size()) && ('@' == name[0]);
    }

    TStringBuf GetMatrixnetIDFromName(const TStringBuf& name) {
        return name.SubStr(1);
    }
} // namespace


TRankModel::TRankModel()
    : MatrixnetEnabled(false)
    , PolynomEnabled(false)
{
}

TRankModel::TRankModel(const NMatrixnet::TRelevCalcerPtr &matrixnet)
    : MatrixnetPtr(matrixnet)
    , MatrixnetEnabled(!!matrixnet)
    , PolynomData()
    , PolynomEnabled(false)
{
}

TRankModel::TRankModel(const TPolynomDescrStatic &polynom)
    : MatrixnetPtr()
    , MatrixnetEnabled(false)
    , PolynomData(polynom)
    , PolynomEnabled(true)
{
}

TRankModel::TRankModel(const NMatrixnet::TRelevCalcerPtr &matrixnet, const TPolynomDescrStatic &polynom)
    : MatrixnetPtr(matrixnet)
    , MatrixnetEnabled(!!MatrixnetPtr)
    , PolynomData(polynom)
    , PolynomEnabled(true)
{
}

void TRankModel::SetMatrixnet(const NMatrixnet::TRelevCalcerPtr &matrixnet) {
    MatrixnetPtr = matrixnet;
    MatrixnetEnabled = !!matrixnet;
}

void TRankModel::SetPolynom(const TPolynomDescrStatic &polynom) {
    PolynomData = polynom;
    PolynomEnabled = true;
}

void TRankModel::DisableMatrixnet() {
    MatrixnetEnabled = false;
}

void TRankModel::EnableMatrixnet() {
    MatrixnetEnabled = true;
}

bool TRankModel::HasMatrixnet() const {
    return MatrixnetEnabled && !!MatrixnetPtr;
}

bool TRankModel::HasPolynom() const {
    return PolynomEnabled;
}

bool TRankModel::Complete() const {
    return HasPolynom() && HasMatrixnet();
}

const NMatrixnet::IRelevCalcer* TRankModel::Matrixnet() const {
    return MatrixnetPtr.Get();
}

const NMatrixnet::TMnSseInfo* TRankModel::MnSseInfo() const {
    if (!HasMatrixnet()) {
        return nullptr;
    }
    return dynamic_cast<const NMatrixnet::TMnSseInfo*>(Matrixnet());
}

const NMatrixnet::TBundle* TRankModel::Bundle() const {
    if (!HasMatrixnet()) {
        return nullptr;
    }
    return dynamic_cast<const NMatrixnet::TBundle*>(Matrixnet());
}

NMatrixnet::TRelevCalcerPtr TRankModel::MatrixnetShared() const {
    return MatrixnetPtr;
}

const TPolynomDescrStatic* TRankModel::Polynom() const {
    return &PolynomData;
}

void TRankModelsMapFactory::SetModelVariant(const TStringBuf& name, const TRankModelVariant& variant, const TString* id) {
    TString mainName;
    TVector<TString> exps;
    ParseModelName(name, mainName, exps);

    if (exps.empty()) {
        SetModelVariantSpecific(mainName, variant, id);
    } else {
        for (int i = 0; i < exps.ysize(); ++i) {
            SetModelVariantSpecific(mainName + ":" + exps[i] + ":", variant, id);
        }
    }
}

void TRankModelsMapFactory::SetModelVariantSpecific(const TString& name, const TRankModelVariant& variant, const TString* id) {
    BuiltBundles.clear(); // Changing Models/ModelsByID content can lead to rehashing and to invalidate pointers.
                          // Since SetModelVariantSpec call is not expected during work its seems ok to do this
    Models.erase(name);
    Models.emplace(name, variant);

    if (id && !ModelsByID.FindPtr(*id)) {
        ModelsByID.erase(*id);
        ModelsByID.emplace(*id, variant);
        ModelNamesById.erase(*id);
        ModelNamesById.emplace(*id, name);
    }
}

template <class T>
const TRankModelsMapFactory::TRankModelVariant* TRankModelsMapFactory::GetModelVariant(const TStringBuf& name, const TSet<TString>& exps, TString* actualName) const {
    if (!ModelNameIsID(name)) {
        return GetModelVariantByName<T>(name, exps, actualName);
    }
    const auto id = GetMatrixnetIDFromName(name);
    const auto variant = GetModelVariantByID(TString{id});
    if (variant && actualName) {
        *actualName = name;
    }
    return variant;
}

template <class T>
const TRankModelsMapFactory::TRankModelVariant* TRankModelsMapFactory::GetModelVariantByName(const TStringBuf& name, const TSet<TString>& exps, TString* actualName) const {
    TStringBuf rankingSpace(name);
    const size_t suffixPos = rankingSpace.rfind(':');
    if (suffixPos != TStringBuf::npos) {
        const size_t experimentsPos = rankingSpace.find(':');
        if (suffixPos == experimentsPos) {
            rankingSpace.Trunc(suffixPos);
        } else {
            rankingSpace.Trunc(suffixPos + 1);
            return GetModelVariantSpecific<T>(TString{rankingSpace}, actualName);
        }
    }

    /* using reverse iterator for compatibility with old code.
     * Apart from keeping tests unchanged, is important to be able to substitute production models via Report flags instantly. */
    for (; !rankingSpace.empty(); rankingSpace.RNextTok('.')) {
        for (auto itexp = exps.rbegin(); itexp != exps.rend(); ++itexp) {
            auto stream = TStringBuf(*itexp);
            const TStringBuf formula = stream.RNextTok(':');

            TStringStream modelName;
            modelName << rankingSpace << ':' << formula << ':';

            if (const auto result = GetModelVariantSpecific<T>(modelName.Str(), actualName)) {
                return result;
            }
        }
        if (const auto result = GetModelVariantSpecific<T>(TString{rankingSpace}, actualName)) {
            return result;
        }
    }
    return nullptr;
}

const TRankModelsMapFactory::TRankModelVariant* TRankModelsMapFactory::GetModelVariantByID(const TString& id) const {
    return ModelsByID.FindPtr(id);
}

template <class T>
const TRankModelsMapFactory::TRankModelVariant* TRankModelsMapFactory::GetModelVariantSpecific(const TString& name, TString* actualName) const {
    const auto variant = Models.FindPtr(name);
    if (!variant) {
        return nullptr;
    }
    if constexpr (!std::is_same_v<T, void>) {
        if (!std::holds_alternative<T>(*variant)) {
            return nullptr;
        }
    }
    if (actualName) {
        *actualName = name;
    }
    return variant;
}

void TRankModelsMapFactory::SetMatrixnet(const TStringBuf& name, const NMatrixnet::TMnSsePtr& model)
{
    if (!model) {
        return;
    }

    const TRankModelVariant variant(model);
    SetModelVariant(name, variant, model->Info.FindPtr("formula-id"));
}

NMatrixnet::TRelevCalcerPtr TRankModelsMapFactory::GetModelByID(const TString& id) const {
    const auto variant = GetModelVariantByID(id);
    if (auto ptr = TryGetBundle(variant)) {
        return ptr;
    }
    return BuildModel(variant);
}

NMatrixnet::TMnSsePtr TRankModelsMapFactory::GetMatrixnetByID(const TString& id) const {
    const auto variant = GetModelVariantByID(id);
    return BuildMatrixnet(variant);
}

NMatrixnet::TRelevCalcerPtr TRankModelsMapFactory::GetModel(const TStringBuf& name, const TSet<TString>& exps, TString* actualName) const {
    const auto variant = GetModelVariant(name, exps, actualName);
    if (exps.empty()) {
        if (auto ptr = TryGetBundle(variant)) {
            return ptr;
        }
    }
    return BuildModel(variant, exps);
}

NMatrixnet::TMnSsePtr TRankModelsMapFactory::GetMatrixnet(const TStringBuf& name, const TSet<TString>& exps, TString* actualName) const {
    const auto variant = GetModelVariant<NMatrixnet::TMnSsePtr>(name, exps, actualName);
    return BuildMatrixnet(variant);
}

const NNeuralNetApplier::TModel* TRankModelsMapFactory::GetDssmModel(const TStringBuf name) const {
    auto it = DssmModels.find(name);
    return it == DssmModels.end()
        ? nullptr
        : it->second.Get();
}

void TRankModelsMapFactory::SetDssmModel(const TString& name, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& dssmModel) {
    DssmModels[name] = dssmModel;
}

const NOptimizedModel::TOptimizedModelApplier* TRankModelsMapFactory::GetOptimizedModel(const NBuildModels::NProto::EApplyMode applyMode) const {
    const TSimpleSharedPtr<NOptimizedModel::TOptimizedModelApplier>* result = OptimizedModels.FindPtr(applyMode);
    return result ? result->Get() : nullptr;
}

void TRankModelsMapFactory::SetOptimizedModel(const NBuildModels::NProto::EApplyMode applyMode, const TSimpleSharedPtr<NNeuralNetApplier::TModel>& dssmModel) {
    if (dssmModel) {
        OptimizedModels[applyMode] = new NOptimizedModel::TOptimizedModelApplier(*dssmModel, applyMode);
    } else {
        OptimizedModels[applyMode] = nullptr;
    }
}

void TRankModelsMapFactory::SetQueryWordDictionary(const TString& name, const TSimpleSharedPtr<NQueryWordTitle::TQueryWordDictionary>& dictionary) {
    QueryWordDictionaries[name] = dictionary;
}

const NQueryWordTitle::TQueryWordDictionary* TRankModelsMapFactory::GetQueryWordDictionary(const TString& name) const {
    if (QueryWordDictionaries.contains(name)) {
        return QueryWordDictionaries.at(name).Get();
    }
    return nullptr;
}

const NVowpalWabbit::TModel* TRankModelsMapFactory::GetVowpalWabbitModel(const TString& name) const {
    const auto it = VowpalWabbitModels.find(name);
    return it == VowpalWabbitModels.end() ? nullptr : it->second.Get();
}

void TRankModelsMapFactory::SetVowpalWabbitModel(const TString& name, const TSimpleSharedPtr<NVowpalWabbit::TModel>& vowpalWabbitModel) {
    VowpalWabbitModels[name] = vowpalWabbitModel;
}

const NCatboostCalcer::TCatboostCalcer* TRankModelsMapFactory::GetCatboost(const TString& name) const {
    if (CatboostModels.contains(name)) {
        return CatboostModels.at(name).Get();
    }
    return nullptr;
}

void TRankModelsMapFactory::SetCatboost(const TString& name, const NCatboostCalcer::TCatboostCalcerPtr& model) {
    CatboostModels[name] = model;
}

void TRankModelsMapFactory::SetBundle(const TStringBuf& name, const NMatrixnet::TBundleDescription& bundle) {
    if (bundle.Elements.empty()) {
        return;
    }

    const TRankModelVariant variant(bundle);
    SetModelVariant(name, variant, bundle.Info.FindPtr("formula-id"));
}

NMatrixnet::TBundlePtr TRankModelsMapFactory::GetBundle(const TStringBuf& name, const TSet<TString>& exps, TString* actualName) const {
    const auto variant = GetModelVariant<NMatrixnet::TBundleDescription>(name, exps, actualName);
    if (exps.empty()) {
        if (auto ptr = TryGetBundle(variant)) {
            return ptr;
        }
    }
    return BuildBundle(variant, exps);
}

NMatrixnet::TRelevCalcerPtr TRankModelsMapFactory::BuildModel(const TRankModelVariant* variant, const TSet<TString>& exps) const {
    if (const auto matrixnet = BuildMatrixnet(variant)) {
        return NMatrixnet::TRelevCalcerPtr(matrixnet);
    }
    if (const auto bundle = BuildBundle(variant, exps)) {
        return NMatrixnet::TRelevCalcerPtr(bundle);
    }
    return NMatrixnet::TRelevCalcerPtr();
}

NMatrixnet::TMnSsePtr TRankModelsMapFactory::BuildMatrixnet(const TRankModelVariant* variant) const {
    if (!variant) {
        return NMatrixnet::TMnSsePtr();
    }
    const auto matrixnet = std::get_if<NMatrixnet::TMnSsePtr>(variant);
    if (!matrixnet) {
        return NMatrixnet::TMnSsePtr();
    }
    return *matrixnet;
}

NMatrixnet::TBundlePtr TRankModelsMapFactory::BuildBundle(const TRankModelVariant* variant,
                                                          const TSet<TString>& exps) const {
    TBundleSet visited;
    return BuildBundle(variant, exps, visited);
}

NMatrixnet::TBundlePtr TRankModelsMapFactory::BuildBundle(const TRankModelVariant* variant,
                                                          const TSet<TString>& exps,
                                                          TBundleSet& visited) const {
    if (!variant) {
        return NMatrixnet::TBundlePtr();
    }
    const auto description = std::get_if<NMatrixnet::TBundleDescription>(variant);
    if (!description) {
        return NMatrixnet::TBundlePtr();
    }
    TMatrixnetPositions positions;
    NMatrixnet::TRankModelVector matrixnets;
    BuildBundle(description, exps, matrixnets, positions, visited);
    return MakeAtomicShared<NMatrixnet::TBundle>(matrixnets, description->Info);
}

void TRankModelsMapFactory::BuildBundle(const NMatrixnet::TBundleDescription* description,
                                        const TSet<TString>& exps,
                                        NMatrixnet::TRankModelVector& matrixnets,
                                        TMatrixnetPositions& positions,
                                        TBundleSet& visited) const {
    Y_ENSURE(!visited.contains(description), "Bundle contains a cycle");
    visited.insert(description);
    for (const auto& element : description->Elements) {
        const auto variant = GetModelVariant(element.Matrixnet, exps);
        if (const auto matrixnet = BuildMatrixnet(variant)) {
            BuildBundle(matrixnet, element.Renorm, matrixnets, positions);
        } else if (const auto bundle = BuildBundle(variant, exps, visited)) {
            for (const auto& subElement : bundle->Matrixnets) {
                auto compositeRenorm = NMatrixnet::TBundleRenorm::Compose(element.Renorm, subElement.Renorm);
                BuildBundle(subElement.Matrixnet, compositeRenorm, matrixnets, positions);
            }
        }
    }
    visited.erase(description);
}

void TRankModelsMapFactory::BuildBundle(const NMatrixnet::TMnSsePtr& matrixnet,
                                        const NMatrixnet::TBundleRenorm renorm,
                                        NMatrixnet::TRankModelVector& matrixnets,
                                        TMatrixnetPositions& positions) const {
    if (const auto pos = positions.FindPtr(matrixnet.Get())) {
        matrixnets[*pos].Renorm = NMatrixnet::TBundleRenorm::Sum(matrixnets[*pos].Renorm, renorm);
    } else {
        positions.emplace(matrixnet.Get(), matrixnets.size());
        matrixnets.emplace_back(matrixnet, renorm);
    }
}

void TRankModelsMapFactory::FreezeBundles() {
    for (const auto* m : { &Models, &ModelsByID }) {
        for (const auto& [name, variant] : *m) {
            if (const auto bundleDescriptionPtr = std::get_if<NMatrixnet::TBundleDescription>(&variant)) {
                BuiltBundles[bundleDescriptionPtr] = BuildBundle(&variant, {});
            }
        }
    }
}

NMatrixnet::TBundlePtr TRankModelsMapFactory::TryGetBundle(const TRankModelVariant* variant) const {
    if (const auto bundleDescriptionPtr = std::get_if<NMatrixnet::TBundleDescription>(variant)) {
        if (auto it = BuiltBundles.find(bundleDescriptionPtr); it != BuiltBundles.end()) {
            return it->second;
        }
    }
    return nullptr;
}
