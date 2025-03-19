#pragma once

#include "options.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

#include <util/thread/pool.h>

#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/stream/file.h>

#include <util/ysaveload.h>

namespace NRegTree {

template <typename TFloatType>
struct TInstance {
    size_t PoolId = 0;

    size_t QueryId = 0;

    TString Mark;

    TFloatType OriginalGoal = TFloatType();

    TFloatType Prediction = TFloatType();
    TFloatType Goal = TFloatType();

    TFloatType Weight = TFloatType(1);

    TVector<TFloatType> Features;

    TVector<TFloatType> RandomWeights;
    TVector<TFloatType> PredictionsForRandomWeights;

    TString ToFeaturesString(const TString& url = "") const {
        TStringStream result;
        result << QueryId << "\t"
               << OriginalGoal << "\t"
               << (url ? url : "url") << "\t"
               << Weight << "\t";
        result << JoinSeq("\t", Features);
        return result.Str();
    }

    TInstance() {
    }

    TInstance(const TString& featuresStr,
              const TOptions& options,
              THashMap<TString, size_t>* queryMatchings = nullptr,
              size_t queriesCount = 0)
        : PoolId(0)
        , Prediction(TFloatType())
        , Weight(TFloatType(1))
    {
        TStringBuf featuresStrBuf(featuresStr);

        TString queryId = ToString(featuresStrBuf.NextTok('\t'));

        if (options.LossFunctionName != "RANK") {
            QueryId = CombineHashes(PoolId, THash<TString>()(queryId));
            if (queryMatchings) {
                if (queryMatchings->contains(queryId)) {
                    QueryId = (*queryMatchings)[queryId];
                } else {
                    QueryId = CombineHashes(queryMatchings->size() + queriesCount, QueryId);
                    (*queryMatchings)[queryId] = QueryId;
                }
            }
        } else {
            if (queryMatchings->contains(queryId)) {
                QueryId = (*queryMatchings)[queryId];
            } else {
                QueryId = queryMatchings->size() + queriesCount;
                (*queryMatchings)[queryId] = QueryId;
            }
        }

        Mark = ToString(featuresStrBuf.NextTok('\t'));
        if (!!options.PositiveMark) {
            OriginalGoal = Mark == options.PositiveMark
                           ? options.ClassificationThreshold + 1
                           : options.ClassificationThreshold - 1;
        } else {
            OriginalGoal = FromString(Mark);
        }

        Goal = OriginalGoal;

        featuresStrBuf.NextTok('\t');

        TStringBuf weightStr = featuresStrBuf.NextTok('\t');
        if (options.UseWeights) {
            Weight = FromString<TFloatType>(weightStr);
        }

        while (!!featuresStrBuf) {
            Features.push_back(FromString<TFloatType>(featuresStrBuf.NextTok('\t')));
        }

        if (options.LossFunctionName.find("SURPLUS") != TString::npos || options.LossFunctionName == "QUERY_REGULARIZED_LOGISTIC") {
            for (size_t i = 0; i < options.IntentWeightStepsCount; ++i) {
                RandomWeights.push_back(i * options.IntentWeightStep + options.StartIntentWeight);
                PredictionsForRandomWeights.push_back(TFloatType());
            }
        }
    }
};

struct TPoolFileNameInfo {
    TString FeaturesFileName;
    float PoolWeight;
};

struct TInstanceNumbersPair {
    ui32 BetterInstanceNumber;
    ui32 WorseInstanceNumber;
    float Weight;

    static TInstanceNumbersPair FromString(TStringBuf description) {
        TInstanceNumbersPair result;
        result.BetterInstanceNumber = ::FromString(description.NextTok('\t'));
        result.WorseInstanceNumber = ::FromString(description.NextTok('\t'));
        result.Weight = description ? ::FromString(description.NextTok('\t')) : 0.;
        return result;
    }
};

template <typename TFloatType>
class TPool {
private:
    TVector<TInstance<TFloatType> > Instances;
    size_t QueriesCount = 0;
    size_t PoolsCount = 0;

    TVector<TFloatType> PoolQueryWeights;

    TVector<TInstanceNumbersPair> Pairs;
public:
    Y_SAVELOAD_DEFINE(Instances, QueriesCount, PoolsCount, PoolQueryWeights, Pairs);

    void SaveToFeatures(IOutputStream& out) const {
        for (const TInstance<TFloatType>& instance : Instances) {
            out << instance.ToFeaturesString() << "\n";
        }
    }

    bool AddInstance(const TInstance<TFloatType>& instance) {
        if (!Instances.empty() && instance.Features.size() != Instances.front().Features.size()) {
            return false;
        }

        Instances.push_back(instance);
        return true;
    }

    const TVector<TInstance<TFloatType> >& GetInstances() const {
        return Instances;
    }

    TVector<TInstance<TFloatType> >& GetInstances() {
        return Instances;
    }

    const TVector<TInstanceNumbersPair>& GetPairs() const {
        return Pairs;
    }

    size_t GetInstancesCount() const {
        return Instances.size();
    }

    size_t GetPairsCount() const {
        return Pairs.size();
    }

    size_t GetFeaturesCount() const {
        return Instances.empty() ? (size_t) 0 : Instances.front().Features.size();
    }

    size_t GetQueriesCount() const {
        return QueriesCount;
    }

    size_t GetPoolsCount() const {
        return PoolsCount;
    }

    float GetPoolQueryWeight(size_t poolId) const {
        return PoolQueryWeights[poolId];
    }

    void AddInstances(IInputStream& featuresIn,
                      const TOptions& options,
                      float poolWeight = 1.f,
                      size_t poolId = 0)
    {
        THashMap<TString, size_t> queryMatchings;

        size_t oldInstancesCount = GetInstancesCount();
        size_t oldPairsCount = GetPairsCount();

        size_t lineNumber = 1;
        TString dataStr;
        while (featuresIn.ReadLine(dataStr)) {
            if (!AddInstance(TInstance<TFloatType>(dataStr, options, &queryMatchings, QueriesCount))) {
                ythrow yexception() << "wrong features count at line "
                                    << lineNumber
                                    << "in pool #"
                                    << poolId
                                    << "\n";
            }
            Instances.back().PoolId = poolId;
            ++lineNumber;
        }

        QueriesCount += queryMatchings.size();
        if (options.WeightByQueries) {
            WeightByQueries(Instances.begin() + oldInstancesCount, Instances.end());
        }

        for (TInstance<TFloatType>* instance = Instances.begin() + oldInstancesCount; instance != Instances.end(); ++instance) {
            instance->Weight *= options.FlatPoolWeights  ? poolWeight / (Instances.size() - oldInstancesCount) : poolWeight;
        }
        for (TInstanceNumbersPair* pair = Pairs.begin() + oldPairsCount; pair != Pairs.end(); ++pair) {
            pair->Weight *= options.FlatPoolWeights  ? poolWeight / (Pairs.size() - oldInstancesCount) : poolWeight;
        }

        ++PoolsCount;
        PoolQueryWeights.push_back(queryMatchings.size() && options.FlatPoolWeights ? poolWeight / queryMatchings.size() : poolWeight);
    }

    void AddInstances(const TPoolFileNameInfo& poolFileNameInfo, const TOptions& options, size_t poolId = 0) {
        if (options.LossFunctionName == "PAIRWISE") {
            AddPairs(poolFileNameInfo.FeaturesFileName + ".pairs");
        }
        TFileInput featuresIn(poolFileNameInfo.FeaturesFileName);
        AddInstances(featuresIn, options, poolFileNameInfo.PoolWeight, poolId);
    }

    void AddInstances(const TString& featuresFileName, const TOptions& options, size_t poolId = 0) {
        if (!featuresFileName) {
            return;
        }

        TPoolFileNameInfo poolFileNameInfo;
        poolFileNameInfo.FeaturesFileName = featuresFileName;
        poolFileNameInfo.PoolWeight = 1.f;

        if (options.LossFunctionName == "PAIRWISE") {
            AddPairs(featuresFileName + ".pairs");
        }
        AddInstances(poolFileNameInfo, options, poolId);
    }

    void AddInstances(const TVector<TPoolFileNameInfo>& poolFileNameInfos, const TOptions& options) {
        for (size_t poolId = 0; poolId < poolFileNameInfos.size(); ++poolId) {
            AddInstances(poolFileNameInfos[poolId], options, poolId);
        }
    }

    const TInstance<TFloatType>* begin() const {
        return Instances.begin();
    }

    TInstance<TFloatType>* begin() {
        return Instances.begin();
    }

    const TInstance<TFloatType>* end() const {
        return Instances.end();
    }

    TInstance<TFloatType>* end() {
        return Instances.end();
    }

    const TInstance<TFloatType>& operator[] (size_t index) const {
        return Instances[index];
    }

    TInstance<TFloatType>& operator[] (size_t index) {
        return Instances[index];
    }

    void Restore() {
        for (TInstance<TFloatType>& instance : *this) {
            instance.Goal = instance.OriginalGoal;
            instance.Prediction = TFloatType();
        }
    }

    void ClearIgnoredFeatures(const TOptions& options) {
        for (TInstance<TFloatType>& instance : *this) {
            ClearIgnoredFeatures(instance, options);
        }
    }

    void ModifyPositivesWeight(const TOptions& options) {
        if (options.PositivesFactor == 1.) {
            return;
        }

        for (TInstance<TFloatType>& instance : Instances) {
            if (instance.OriginalGoal > options.ClassificationThreshold) {
                instance.Weight *= options.PositivesFactor;
            }
        }
    }
private:
    static void ClearIgnoredFeatures(TInstance<TFloatType>& instance, const TOptions& options) {
        TFloatType* writePtr = instance.Features.begin();
        for (size_t featureNumber = 0; featureNumber < instance.Features.size(); ++featureNumber) {
            if (options.IgnoredFeatures.contains(featureNumber)) {
                continue;
            }
            *writePtr = instance.Features[featureNumber];
            ++writePtr;
        }
        instance.Features.erase(writePtr, instance.Features.end());
    }

    void AddPairs(const TString& pairsFileName) {
        size_t instancesCount = Instances.size();
        TFileInput pairsIn(pairsFileName);
        TString dataStr;
        while (pairsIn.ReadLine(dataStr)) {
            Pairs.push_back(TInstanceNumbersPair::FromString(dataStr));
            Pairs.back().BetterInstanceNumber += instancesCount;
            Pairs.back().WorseInstanceNumber += instancesCount;
        }
    }

    void WeightByQueries(TInstance<TFloatType>* begin, TInstance<TFloatType>* end) {
        TVector<size_t> querySizes(QueriesCount);
        for (const TInstance<TFloatType>* instance = begin; instance != end; ++instance) {
            ++querySizes[instance->QueryId];
        }
        for (TInstance<TFloatType>* instance = begin; instance != end; ++instance) {
            instance->Weight /= querySizes[instance->QueryId];
        }
    }
};

template <typename TFloatType>
struct TIteratorItem {
    size_t InstanceNumber;

    TFloatType Feature;

    TIteratorItem(const TPool<TFloatType>& pool, size_t instanceNumber, size_t featureNumber)
        : InstanceNumber(instanceNumber)
        , Feature(pool[instanceNumber].Features[featureNumber])
    {
    }

    bool operator < (const TIteratorItem& other) const {
        if (Feature != other.Feature) {
            return Feature < other.Feature;
        }
        return InstanceNumber < other.InstanceNumber;
    }
};

template <typename TFloatType>
class TFeatureIterator {
private:
    TVector<TIteratorItem<TFloatType> > Items;
    size_t FeatureNumber = 0;
public:
    void Build(const TPool<TFloatType>& pool, size_t featureNumber) {
        FeatureNumber = featureNumber;
        Items.reserve(pool.GetInstancesCount());

        bool isValid = false;
        for (size_t instanceNumber = 0; instanceNumber < pool.GetInstancesCount(); ++instanceNumber) {
            Items.push_back(TIteratorItem<TFloatType>(pool, instanceNumber, featureNumber));
            if (!isValid && Items.back().Feature != Items.front().Feature) {
                isValid = true;
            }
        }

        if (isValid) {
            Sort(Items.begin(), Items.end());
        } else {
            TVector<TIteratorItem<TFloatType> >().swap(Items);
        }
    }

    const TIteratorItem<TFloatType>* begin() const {
        return Items.begin();
    }

    const TIteratorItem<TFloatType>* end() const {
        return Items.end();
    }

    bool Empty() const {
        return Items.empty();
    }

    size_t GetFeatureNumber() const {
        return FeatureNumber;
    }
};

template <typename TFloatType>
TVector<TFeatureIterator<TFloatType> > BuildFeatureIterators(const TPool<TFloatType>& pool, const TOptions& options) {
    TVector<TFeatureIterator<TFloatType> > featureIterators(pool.GetFeaturesCount());
    THolder<IThreadPool> queue(CreateThreadPool(options.ThreadsCount));
    for (size_t featureNumber = 0; featureNumber < pool.GetFeaturesCount(); ++featureNumber) {
        queue->SafeAddFunc([&, featureNumber](){
            featureIterators[featureNumber].Build(pool, featureNumber);
        });
    }
    Delete(std::move(queue));
    return featureIterators;
}

}
