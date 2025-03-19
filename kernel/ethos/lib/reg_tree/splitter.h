#pragma once

#include "linear_regression.h"
#include "options.h"
#include "pool.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>

#include <util/system/yassert.h>

namespace NRegTree {

template <typename TSplitterBase, typename TFloatType>
inline void Fill(TSplitterBase& splitterBase, const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
    size_t instanceNumber = 0;
    for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance, ++instanceNumber) {
        if (!testInstances.contains(instanceNumber)) {
            splitterBase.AddInstance(*instance);
        }
    }
}

template <typename TSplitterBase, typename TFloatType>
class TSplitter {
private:
    TFloatType BaseSumSquaredErrors;
    TFloatType SplitSumSquaredErrors;

    TSplitterBase Left;
    TSplitterBase Right;
public:
    TSplitter() {
    }

    explicit TSplitter(TSplitterBase base)
        : BaseSumSquaredErrors(base.SumSquaredErrors())
        , SplitSumSquaredErrors(BaseSumSquaredErrors)
    {
        Left = base.CreateEmptyClone();
        Right = base;
    }

    void Clear() {
        BaseSumSquaredErrors = SplitSumSquaredErrors = TFloatType();

        Left = Left.CreateEmptyClone();
        Right = Right.CreateEmptyClone();
    }

    void Add(const TInstance<TFloatType>& instance, TFloatType currentFeature) {
        Right.AddInstance(instance, currentFeature);
    }

    void Move(const TInstance<TFloatType>& instance, TFloatType currentFeature) {
        Left.AddInstance(instance, currentFeature);
        Right.RemoveInstance(instance, currentFeature);
    }

    void InitQuality() {
        Y_ASSERT(Left.GetInstancesCount() == 0 || Right.GetInstancesCount() == 0);

        BaseSumSquaredErrors = Left.SumSquaredErrors() + Right.SumSquaredErrors();
        SplitSumSquaredErrors = BaseSumSquaredErrors;
    }

    void UpdateSplitQuality() {
        SplitSumSquaredErrors = Left.SumSquaredErrors() + Right.SumSquaredErrors();
    }

    TFloatType SumSquaredErrors() {
        return SplitSumSquaredErrors;
    }

    const TSplitterBase& GetLeft() {
        return Left;
    }

    const TSplitterBase& GetRight() {
        return Right;
    }

    TFloatType IsCorrect(const TOptions& options) const {
        size_t leftInstancesCount = Left.GetInstancesCount();
        size_t rightInstancesCount = Right.GetInstancesCount();

        return leftInstancesCount >= options.MinLeafSize.Value &&
               rightInstancesCount >= options.MinLeafSize.Value &&
               leftInstancesCount * 4 >= rightInstancesCount &&
               rightInstancesCount * 4 >= leftInstancesCount;
    }
};

template <typename TFloatType>
class TDeviationSplitterBase {
private:
    TFloatType SumGoals;
    TFloatType SumSquaredGoals;
    TFloatType SumWeights;

    size_t InstancesCount;
public:
    explicit TDeviationSplitterBase(TFloatType sumGoals = TFloatType(),
                                    TFloatType sumSquaredGoals = TFloatType(),
                                    TFloatType sumWeights = TFloatType())
        : SumGoals(sumGoals)
        , SumSquaredGoals(sumSquaredGoals)
        , SumWeights(sumWeights)
        , InstancesCount(0)
    {
    }

    TDeviationSplitterBase(const TPool<TFloatType>& pool, const TOptions&, const THashSet<size_t>& testInstances, bool fill = true)
        : SumGoals(TFloatType())
        , SumSquaredGoals(TFloatType())
        , SumWeights(TFloatType())
        , InstancesCount(0)
    {
        if (fill) {
            Fill(*this, pool, testInstances);
        }
    }

    TDeviationSplitterBase<TFloatType>& operator += (const TDeviationSplitterBase<TFloatType>& other) {
        SumGoals += other.SumGoals;
        SumSquaredGoals += other.SumSquaredGoals;
        SumWeights += other.SumWeights;
        InstancesCount += other.InstancesCount;

        return *this;
    }

    TDeviationSplitterBase<TFloatType>& operator -= (const TDeviationSplitterBase<TFloatType>& other) {
        SumGoals -= other.SumGoals;
        SumSquaredGoals -= other.SumSquaredGoals;
        SumWeights -= other.SumWeights;
        InstancesCount -= other.InstancesCount;

        return *this;
    }

    const TDeviationSplitterBase<TFloatType> operator + (const TDeviationSplitterBase<TFloatType>& other) {
        TDeviationSplitterBase<TFloatType> result(*this);
        result += other;
        return result;
    }

    const TDeviationSplitterBase<TFloatType> operator - (const TDeviationSplitterBase<TFloatType>& other) {
        TDeviationSplitterBase<TFloatType> result(*this);
        result -= other;
        return result;
    }

    void UpdateGoals(const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
        SumGoals = SumSquaredGoals = SumWeights = InstancesCount = TFloatType();
        Fill(*this, pool, testInstances);
    }

    TDeviationSplitterBase<TFloatType> CreateEmptyClone() const {
        return TDeviationSplitterBase<TFloatType>();
    }

    inline void AddInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        ++InstancesCount;
        Add(instance.Goal, instance.Weight);
    }

    inline void RemoveInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        --InstancesCount;
        Add(instance.Goal, -instance.Weight);
    }

    inline TFloatType SumSquaredErrors() const {
        return SumSquaredErrors(SumGoals, SumSquaredGoals, SumWeights);
    }

    inline TFloatType SumSquaredErrors(const TLinearModel<TFloatType>&) {
        return SumSquaredErrors();
    }

    inline bool NeedsReset() const {
        return false;
    }

    inline size_t GetInstancesCount() const {
        return InstancesCount;
    }

    void Solve(TLinearModel<TFloatType>& linearModel) {
        linearModel.Intercept = SumWeights ? SumGoals / SumWeights : TFloatType();
    }

    static inline TFloatType SumSquaredErrors(TFloatType sumGoals, TFloatType sumSquaredGoals, TFloatType sumWeights) {
        if (!sumWeights) {
            return TFloatType();
        }
        TFloatType sumSquaredErrors = sumSquaredGoals - sumGoals / sumWeights * sumGoals;
        return Max(TFloatType(), sumSquaredErrors);
    }

    inline void Add(TFloatType goal, TFloatType weight) {
        TFloatType weightedGoal = goal * weight;
        SumGoals += weightedGoal;
        SumSquaredGoals += goal * weightedGoal;
        SumWeights += weight;
    }
};

template <typename TFloatType>
class TSimpleLinearRegressionSplitterBase {
private:
    TFloatType SumFeatures;
    TFloatType SumSquaredFeatures;

    TFloatType SumGoals;
    TFloatType SumSquaredGoals;

    TFloatType SumProducts;

    TFloatType SumWeights;

    size_t InstancesCount;
public:
    explicit TSimpleLinearRegressionSplitterBase(TFloatType sumFeatures = TFloatType(),
                                                 TFloatType sumSquaredFeatures = TFloatType(),
                                                 TFloatType sumGoals = TFloatType(),
                                                 TFloatType sumSquaredGoals = TFloatType(),
                                                 TFloatType sumProducts = TFloatType(),
                                                 TFloatType sumWeights = TFloatType())
        : SumFeatures(sumFeatures)
        , SumSquaredFeatures(sumSquaredFeatures)
        , SumGoals(sumGoals)
        , SumSquaredGoals(sumSquaredGoals)
        , SumProducts(sumProducts)
        , SumWeights(sumWeights)
        , InstancesCount(0)
    {
    }

    TSimpleLinearRegressionSplitterBase(const TPool<TFloatType>&, const TOptions&, const THashSet<size_t>&, bool = true)
        : SumFeatures(TFloatType())
        , SumSquaredFeatures(TFloatType())
        , SumGoals(TFloatType())
        , SumSquaredGoals(TFloatType())
        , SumProducts(TFloatType())
        , SumWeights(TFloatType())
        , InstancesCount(0)
    {
    }

    TSimpleLinearRegressionSplitterBase& operator += (const TSimpleLinearRegressionSplitterBase& other) {
        SumFeatures += other.SumFeatures;
        SumSquaredFeatures += other.SumSquaredFeatures;
        SumGoals += other.SumGoals;
        SumSquaredGoals += other.SumSquaredGoals;
        SumProducts += other.SumProducts;
        SumWeights += other.SumWeights;
        InstancesCount += other.InstancesCount;

        return *this;
    }

    TSimpleLinearRegressionSplitterBase<TFloatType> operator + (const TSimpleLinearRegressionSplitterBase<TFloatType>& rhs) const {
        TSimpleLinearRegressionSplitterBase<TFloatType> result(*this);
        result += rhs;
        return result;
    }

    void UpdateGoals(const TPool<TFloatType>&, const THashSet<size_t>&) {
    }

    TSimpleLinearRegressionSplitterBase<TFloatType> CreateEmptyClone() const {
        return TSimpleLinearRegressionSplitterBase<TFloatType>();
    }

    template <typename F, typename G>
    inline void Add(const TVector<F>& features, const TVector<G>& goals) {
        Y_ASSERT(features.size() == goals.size());

        const F* feature = features.begin();
        const G* goal = goals.begin();
        for (; feature != features.end(); ++feature, ++goal) {
            Add(*goal, TFloatType(1), *feature);
        }
    }

    template <typename F, typename G, typename W>
    inline void Add(const TVector<F>& features, const TVector<G>& goals, const TVector<W>& weights, W minWeight = W()) {
        Y_ASSERT(features.size() == weights.size());
        Y_ASSERT(goals.size() == weights.size());

        const F* feature = features.begin();
        const G* goal = goals.begin();
        const W* weight = weights.begin();
        for (; feature != features.end(); ++feature, ++goal, ++weight) {
            Add(*goal, Max(*weight, minWeight), *feature);
        }
    }

    inline void AddInstance(const TInstance<TFloatType>& instance, TFloatType feature) {
        ++InstancesCount;
        Add(instance.Goal, instance.Weight, feature);
    }

    inline void RemoveInstance(const TInstance<TFloatType>& instance, TFloatType feature) {
        --InstancesCount;
        Add(instance.Goal, -instance.Weight, feature);
    }

    inline TFloatType SumSquaredErrors() const {
        return SumSquaredErrors(SumGoals, SumSquaredGoals, SumProducts);
    }

    inline TFloatType DeterminationCoefficient() const {
        if (!SumWeights) {
            return TFloatType();
        }

        TFloatType sumSquaredGoalDeviations = SumSquaredGoals - SumGoals / SumWeights * SumGoals;
        if (!sumSquaredGoalDeviations) {
            return TFloatType();
        }

        return 1 - SumSquaredErrors() / sumSquaredGoalDeviations;
    }

    TFloatType GetSumSquaredGoals() const {
        return SumSquaredGoals;
    }

    void Reset() {
        SumFeatures = SumSquaredFeatures = TFloatType();
        SumGoals = SumSquaredGoals = TFloatType();

        SumProducts = TFloatType();

        SumWeights = TFloatType();

        InstancesCount = 0;
    }

    bool NeedsReset() const {
        return true;
    }

    inline size_t GetInstancesCount() const {
        return InstancesCount;
    }

    inline TFloatType SumSquaredErrors(TFloatType sumGoals, TFloatType sumSquaredGoals, TFloatType sumFeatureGoalProducts) const {
        if (!SumWeights) {
            return TFloatType();
        }

        TFloatType sumGoalSquaredDeviations = sumSquaredGoals - sumGoals / SumWeights * sumGoals;

        TFloatType numerator, denominator;
        SetupSolutionFactors(sumGoals, sumFeatureGoalProducts, numerator, denominator);
        if (!denominator) {
            return sumGoalSquaredDeviations;
        }

        TFloatType sumSquaredErrors = sumGoalSquaredDeviations - numerator * numerator / denominator;
        return Max(TFloatType(), sumSquaredErrors);
    }

    inline TFloatType SumSquaredErrorsTrivial() const {
        if (!SumWeights) {
            return TFloatType();
        }
        return SumSquaredGoals - SumGoals / SumWeights * SumGoals;
    }

    inline TFloatType SumSquaredErrors(const TLinearModel<TFloatType>&) {
        return SumSquaredErrors();
    }

    void Solve(TFloatType& factor, TFloatType& offset) const {
        Solve(SumGoals, SumProducts, factor, offset);
    }

    std::pair<TFloatType, TFloatType> Solve() const {
        std::pair<TFloatType, TFloatType> result;
        Solve(result.first, result.second);
        return result;
    }

    void SolveTrivial(TFloatType& factor, TFloatType& offset) const {
        if (!SumGoals) {
            factor = offset = TFloatType();
            return;
        }
        factor = TFloatType();
        offset = SumGoals / SumWeights;
    }

    void Solve(TFloatType sumGoals,
               TFloatType sumFeatureGoalProducts,
               TFloatType& factor,
               TFloatType& offset) const
    {
        if (!sumGoals) {
            factor = offset = TFloatType();
            return;
        }

        TFloatType numerator, denominator;
        SetupSolutionFactors(sumGoals, sumFeatureGoalProducts, numerator, denominator);

        if (!denominator) {
            factor = TFloatType();
            offset = sumGoals / SumWeights;
            return;
        }

        factor = numerator / denominator;
        offset = sumGoals / SumWeights - factor * SumFeatures / SumWeights;
    }

    TFloatType GetSumFeatures() const {
        return SumFeatures;
    }

    TFloatType GetSumProducts() const {
        return SumProducts;
    }

    inline void Add(TFloatType goal, TFloatType weight, TFloatType feature) {
        TFloatType weightedFeature = feature * weight;
        TFloatType weightedGoal = goal * weight;

        SumFeatures += weightedFeature;
        SumSquaredFeatures += feature * weightedFeature;

        SumGoals += weightedGoal;
        SumSquaredGoals += goal * weightedGoal;

        SumProducts += feature * weightedGoal;

        SumWeights += weight;
    }

    inline void ResetGoals() {
        SumGoals = SumSquaredGoals = SumProducts = 0;
    }

    inline void AddGoal(TFloatType goal, TFloatType feature, TFloatType weight) {
        TFloatType weightedGoal = goal * weight;

        SumGoals += weightedGoal;
        SumSquaredGoals += goal * weightedGoal;
        SumProducts += feature * weightedGoal;
    }
private:
    inline void SetupSolutionFactors(TFloatType sumGoals,
                                     TFloatType sumProducts,
                                     TFloatType& numerator,
                                     TFloatType& denominator) const {
        if (!SumWeights) {
            denominator = numerator = TFloatType();
            return;
        }

        denominator = SumSquaredFeatures - SumFeatures / SumWeights * SumFeatures;
        if (!denominator) {
            return;
        }
        numerator = sumProducts - SumFeatures / SumWeights * sumGoals;
    }
};

template <typename TFloatType>
class TBestSimpleLinearRegressionSplitterBase {
private:
    typedef TSimpleLinearRegressionSplitterBase<TFloatType> TSimpleSplitter;
    TVector<TSimpleSplitter> SimpleSplitters;

    size_t InstancesCount;
public:
    explicit TBestSimpleLinearRegressionSplitterBase(size_t featuresCount = 0)
        : SimpleSplitters(featuresCount)
        , InstancesCount(0)
    {
    }

    TBestSimpleLinearRegressionSplitterBase(const TPool<TFloatType>& pool, const TOptions&, const THashSet<size_t>& testInstances, bool fill = true)
        : SimpleSplitters(pool.GetFeaturesCount())
        , InstancesCount(0)
    {
        if (fill) {
            Fill(*this, pool, testInstances);
        }
    }

    TBestSimpleLinearRegressionSplitterBase& operator += (const TBestSimpleLinearRegressionSplitterBase& other) {
        Y_ASSERT(SimpleSplitters.size() == other.SimpleSplitters.size());

        for (size_t i = 0; i < SimpleSplitters.size(); ++i) {
            SimpleSplitters[i] += other.SimpleSplitters[i];
        }
        InstancesCount += other.InstancesCount;

        return *this;
    }

    void UpdateGoals(const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
        for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter) {
            simpleSplitter->ResetGoals();
        }

        for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (testInstances.contains(instance - pool.begin())) {
                continue;
            }
            const TFloatType* feature = instance->Features.begin();
            for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter, ++feature) {
                simpleSplitter->AddGoal(instance->Goal, *feature, instance->Weight);
            }
        }
    }

    TBestSimpleLinearRegressionSplitterBase<TFloatType> CreateEmptyClone() const {
        return TBestSimpleLinearRegressionSplitterBase<TFloatType>(SimpleSplitters.size());
    }

    inline void AddInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        ++InstancesCount;
        Add(instance.Features, instance.Goal, instance.Weight);
    }

    inline void RemoveInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        --InstancesCount;
        Add(instance.Features, instance.Goal, -instance.Weight);
    }

    void Solve(TLinearModel<TFloatType>& linearModel) {
        Y_ASSERT(!SimpleSplitters.empty());

        TSimpleSplitter* bestSimpleSplitter = nullptr;
        {
            TFloatType bestSumSquaredErrors = TFloatType();
            for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter) {
                TFloatType sumSquaredErrors = simpleSplitter->SumSquaredErrors();
                if (!bestSimpleSplitter || bestSumSquaredErrors > sumSquaredErrors) {
                    bestSumSquaredErrors = sumSquaredErrors;
                    bestSimpleSplitter = simpleSplitter;
                }
            }
        }

        if (!bestSimpleSplitter) {
            return;
        }

        bestSimpleSplitter->Solve(linearModel.Coefficients[bestSimpleSplitter - SimpleSplitters.begin()], linearModel.Intercept);
    }

    TFloatType SumSquaredErrors() {
        TSimpleSplitter* bestSimpleSplitter = nullptr;
        TFloatType bestSumSquaredErrors = TFloatType();
        for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter) {
            TFloatType sumSquaredErrors = simpleSplitter->SumSquaredErrors();
            if (!bestSimpleSplitter || bestSumSquaredErrors > sumSquaredErrors) {
                bestSumSquaredErrors = sumSquaredErrors;
                bestSimpleSplitter = simpleSplitter;
            }
        }

        Y_ASSERT(!!bestSimpleSplitter);
        return bestSumSquaredErrors;
    }

    inline TFloatType SumSquaredErrors(const TLinearModel<TFloatType>&) {
        return SumSquaredErrors();
    }

    inline bool NeedsReset() const {
        return false;
    }

    inline size_t GetInstancesCount() const {
        return InstancesCount;
    }

    inline void Add(const TVector<TFloatType>& features, TFloatType goal, TFloatType weight) {
        TSimpleSplitter* simpleSplitter = SimpleSplitters.begin();
        const TFloatType* feature = features.begin();
        for (; feature != features.end(); ++feature, ++simpleSplitter) {
            simpleSplitter->Add(goal, weight, *feature);
        }
    }
};

template <typename TFloatType>
class TPositionalLinearRegressionSplitterBase {
private:
    size_t FeaturesCount;
    size_t IterationsCount;

    TLinearizedTriangleOLSMatrix<TFloatType> OLSMatrix;

    typedef TSimpleLinearRegressionSplitterBase<TFloatType> TSimpleSplitter;
    TVector<TSimpleSplitter> SimpleSplitters;
    TVector<TFloatType> SumGoalProducts;

    TFloatType SumGoals;
    TFloatType SumSquaredGoals;
    TFloatType SumWeights;

    size_t InstancesCount;
public:
    TPositionalLinearRegressionSplitterBase(size_t featuresCount = 0, size_t iterationsCount = 0)
        : FeaturesCount(featuresCount)
        , IterationsCount(iterationsCount)
        , OLSMatrix(FeaturesCount)
        , SimpleSplitters(FeaturesCount)
        , SumGoalProducts(FeaturesCount)
        , SumGoals(TFloatType())
        , SumSquaredGoals(TFloatType())
        , SumWeights(TFloatType())
        , InstancesCount(0)
    {
    }

    TPositionalLinearRegressionSplitterBase(const TPool<TFloatType>& pool, const TOptions& options, const THashSet<size_t>& testInstances, bool fill = true)
        : FeaturesCount(pool.GetFeaturesCount())
        , IterationsCount(options.SplitterIterationsCount)
        , OLSMatrix(FeaturesCount)
        , SimpleSplitters(FeaturesCount)
        , SumGoalProducts(FeaturesCount)
        , SumGoals(TFloatType())
        , SumSquaredGoals(TFloatType())
        , SumWeights(TFloatType())
        , InstancesCount(0)
    {
        if (fill) {
            Fill(*this, pool, testInstances);
        }
    }

    void UpdateGoals(const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
        for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter) {
            simpleSplitter->ResetGoals();
        }

        SumGoals = TFloatType();
        SumSquaredGoals = TFloatType();

        for (const TInstance<TFloatType>* instance = pool.begin(); instance != pool.end(); ++instance) {
            if (testInstances.contains(instance - pool.begin())) {
                continue;
            }

            const TFloatType* feature = instance->Features.begin();
            for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter, ++feature) {
                simpleSplitter->AddGoal(instance->Goal, *feature, instance->Weight);
            }

            TFloatType weightedGoal = instance->Goal * instance->Weight;
            SumGoals += weightedGoal;
            SumSquaredGoals += instance->Goal * weightedGoal;
        }
    }

    TPositionalLinearRegressionSplitterBase<TFloatType>& operator += (const TPositionalLinearRegressionSplitterBase<TFloatType>& other) {
        Y_ASSERT(FeaturesCount == other.FeaturesCount);

        OLSMatrix += other.OLSMatrix;

        for (size_t i = 0; i < SimpleSplitters.size(); ++i) {
            SimpleSplitters[i] += other.SimpleSplitters[i];
        }

        SumGoals += other.SumGoals;
        SumSquaredGoals += other.SumSquaredGoals;
        SumWeights += other.SumWeights;

        InstancesCount += other.InstancesCount;

        return *this;
    }

    TPositionalLinearRegressionSplitterBase<TFloatType> CreateEmptyClone() const {
        return TPositionalLinearRegressionSplitterBase<TFloatType>(FeaturesCount, IterationsCount);
    }

    inline void AddInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        ++InstancesCount;
        Add(instance.Features, instance.Goal, instance.Weight);
    }

    inline void RemoveInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        --InstancesCount;
        Add(instance.Features, instance.Goal, -instance.Weight);
    }

    void Solve(TLinearModel<TFloatType>& linearModel) {
        InitGoalProducts();

        TFloatType sumGoals = SumGoals;
        TFloatType sumSquaredGoals = SumSquaredGoals;

        for (size_t iterationNumber = 0; iterationNumber < IterationsCount; ++iterationNumber) {
            std::pair<const TSimpleSplitter*, TFloatType> bestSplitterInfo = GetBestSplitter(sumGoals, sumSquaredGoals);

            const TSimpleSplitter* bestSplitter = bestSplitterInfo.first;
            TFloatType bestSumSquaredErrors = bestSplitterInfo.second;

            ProcessBestSplitter(bestSplitter, sumGoals, &linearModel);

            sumGoals = TFloatType();
            sumSquaredGoals = bestSumSquaredErrors;
        }
    }

    TFloatType SumSquaredErrors() {
        InitGoalProducts();

        TFloatType sumGoals = SumGoals;
        TFloatType sumSquaredGoals = SumSquaredGoals;

        for (size_t iterationNumber = 0; iterationNumber < IterationsCount; ++iterationNumber) {
            std::pair<const TSimpleSplitter*, TFloatType> bestSplitterInfo = GetBestSplitter(sumGoals, sumSquaredGoals);

            const TSimpleSplitter* bestSplitter = bestSplitterInfo.first;
            TFloatType bestSumSquaredErrors = bestSplitterInfo.second;

            ProcessBestSplitter(bestSplitter, sumGoals);

            sumGoals = TFloatType();
            sumSquaredGoals = bestSumSquaredErrors;
        }

        return sumSquaredGoals;
    }

    inline TFloatType SumSquaredErrors(const TLinearModel<TFloatType>&) {
        return SumSquaredErrors();
    }

    inline bool NeedsReset() const {
        return false;
    }

    inline size_t GetInstancesCount() const {
        return InstancesCount;
    }

    inline void Add(const TVector<TFloatType>& features, TFloatType goal, TFloatType weight) {
        OLSMatrix.Add(features, weight);

        const TFloatType* feature = features.begin();
        for (TSimpleSplitter* simpleSplitter = SimpleSplitters.begin(); simpleSplitter != SimpleSplitters.end(); ++simpleSplitter, ++feature) {
            simpleSplitter->Add(goal, weight, *feature);
        }

        TFloatType weightedGoal = goal * weight;
        SumGoals += weightedGoal;
        SumSquaredGoals += goal * weightedGoal;

        SumWeights += weight;
    }
private:
    void InitGoalProducts() {
        TFloatType* sumGoalProducts = SumGoalProducts.begin();
        const TSimpleSplitter* simpleSplitter = SimpleSplitters.begin();
        for (; sumGoalProducts != SumGoalProducts.end(); ++sumGoalProducts, ++simpleSplitter) {
            *sumGoalProducts = simpleSplitter->GetSumProducts();
        }
    }

    std::pair<const TSimpleSplitter*, TFloatType> GetBestSplitter(TFloatType sumGoals, TFloatType sumSquaredGoals) {
        const TSimpleSplitter* bestSimpleSplitter = nullptr;
        TFloatType bestSumSquaredErrors = TFloatType();

        const TSimpleSplitter* simpleSplitter = SimpleSplitters.begin();
        const TFloatType* sumGoalProducts = SumGoalProducts.begin();
        for (; simpleSplitter != SimpleSplitters.end(); ++simpleSplitter, ++sumGoalProducts) {
            TFloatType sumSquaredErrors = simpleSplitter->SumSquaredErrors(sumGoals, sumSquaredGoals, *sumGoalProducts);
            if (!bestSimpleSplitter || bestSumSquaredErrors > sumSquaredErrors) {
                bestSumSquaredErrors = sumSquaredErrors;
                bestSimpleSplitter = simpleSplitter;
            }
        }

        Y_ASSERT(!!bestSimpleSplitter);

        return std::make_pair(bestSimpleSplitter, bestSumSquaredErrors);
    }

    void ProcessBestSplitter(const TSimpleSplitter* bestSimpleSplitter,
                             TFloatType sumGoals,
                             TLinearModel<TFloatType>* linearModel = nullptr)
    {
        size_t bestSplitterNumber = bestSimpleSplitter - SimpleSplitters.begin();

        TFloatType factor, offset;
        bestSimpleSplitter->Solve(sumGoals, SumGoalProducts[bestSplitterNumber], factor, offset);

        if (!!linearModel) {
            linearModel->Coefficients[bestSplitterNumber] += factor;
            linearModel->Intercept += offset;
        }

        const TSimpleSplitter* simpleSplitter = SimpleSplitters.begin();
        TFloatType* sumGoalProducts = SumGoalProducts.begin();

        size_t step = FeaturesCount;
        const TFloatType* covariationMatrixElement = OLSMatrix.begin() + bestSplitterNumber;
        for (; simpleSplitter < bestSimpleSplitter; ++simpleSplitter, ++sumGoalProducts, covariationMatrixElement += step) {
            *sumGoalProducts -= factor * *covariationMatrixElement + offset * simpleSplitter->GetSumFeatures();
            --step;
        }

        for (; simpleSplitter < SimpleSplitters.end(); ++covariationMatrixElement, ++simpleSplitter, ++sumGoalProducts) {
            *sumGoalProducts -= factor * *covariationMatrixElement + offset * simpleSplitter->GetSumFeatures();
        }
    }
};

template <typename TFloatType>
class TLinearRegressionSplitterBase {
private:
    size_t InstancesCount;

    size_t FeaturesCount;

    TLinearRegressionData<TFloatType> LinearRegressionData;

    TFloatType RegularizationParameter;
    TFloatType RegularizationThreshold;

    TLinearModel<TFloatType> Solution;
public:
    TLinearRegressionSplitterBase(size_t featuresCount = 0,
                                  TFloatType regularizationParameter = 1e-5,
                                  TFloatType regularizationThreshold = 1e-5)
        : InstancesCount(0)
        , FeaturesCount(featuresCount)
        , LinearRegressionData(FeaturesCount)
        , RegularizationParameter(regularizationParameter)
        , RegularizationThreshold(regularizationThreshold)
        , Solution(FeaturesCount)
    {
    }

    TLinearRegressionSplitterBase(const TPool<TFloatType>& pool,
                                  const TOptions& options,
                                  const THashSet<size_t>& testInstances,
                                  bool fill = true)
        : InstancesCount(0)
        , FeaturesCount(pool.GetFeaturesCount())
        , LinearRegressionData(FeaturesCount)
        , RegularizationParameter(options.RegularizationParameter)
        , RegularizationThreshold(options.RegularizationThreshold)
        , Solution(FeaturesCount)
    {
        if (fill) {
            Fill(*this, pool, testInstances);
        }
    }

    TLinearRegressionSplitterBase<TFloatType>& operator += (const TLinearRegressionSplitterBase<TFloatType>& other) {
        Y_ASSERT(FeaturesCount == other.FeaturesCount);

        InstancesCount += other.InstancesCount;
        LinearRegressionData += other.LinearRegressionData;

        return *this;
    }

    void UpdateGoals(const TPool<TFloatType>& pool, const THashSet<size_t>& testInstances) {
        LinearRegressionData.UpdateGoals(pool, testInstances);
    }

    TLinearRegressionSplitterBase<TFloatType> CreateEmptyClone() const {
        return TLinearRegressionSplitterBase<TFloatType>(FeaturesCount, RegularizationParameter, RegularizationThreshold);
    }

    inline void AddInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        ++InstancesCount;
        LinearRegressionData.AddTriangle(instance.Features, instance.Goal, instance.Weight);
    }

    inline void RemoveInstance(const TInstance<TFloatType>& instance, TFloatType = TFloatType()) {
        --InstancesCount;
        LinearRegressionData.AddTriangle(instance.Features, instance.Goal, -instance.Weight);
    }

    inline TFloatType SumSquaredErrors() {
        Solve(Solution);
        return LinearRegressionData.SumSquaredErrors(Solution);
    }

    inline TFloatType SumSquaredErrors(const TLinearModel<TFloatType>& solution) {
        return LinearRegressionData.SumSquaredErrors(solution);
    }

    inline bool NeedsReset() const {
        return false;
    }

    inline size_t GetInstancesCount() const {
        return InstancesCount;
    }

    void Solve(TLinearModel<TFloatType>& linearModel) {
        LinearRegressionData.Solve(linearModel, RegularizationParameter, RegularizationThreshold);
    }
};

}
