#pragma once

#include "linear_regression.h"
#include "options.h"
#include "pool.h"
#include "prune.h"
#include "splitter.h"
#include "transform_features.h"

#include <util/generic/ylimits.h>
#include <util/string/printf.h>
#include <util/thread/pool.h>
#include <util/ysaveload.h>

namespace NRegTree {

template <typename TFloatType>
struct TStump {
    size_t FeatureNumber;
    TFloatType FeatureThreshold;

    Y_SAVELOAD_DEFINE(FeatureNumber, FeatureThreshold)

    explicit TStump(size_t factorNumber = 0, TFloatType factorThreshold = TFloatType())
        : FeatureNumber(factorNumber)
        , FeatureThreshold(factorThreshold)
    {
    }
};

struct TTreeNode {
    size_t Left;
    size_t Right;

    Y_SAVELOAD_DEFINE(Left, Right)

    TTreeNode(size_t left = Max<size_t>(), size_t right = Max<size_t>())
        : Left(left)
        , Right(right)
    {
    }
};

template <typename TSplitterBaseType,typename TFloatType>
class TParallelStumpsProducer : public IObjectInQueue {
private:
    const TOptions& Options;

    const TPool<TFloatType>& Pool;
    const TFeatureIterator<TFloatType>& FeatureIterator;

    TVector<TStump<TFloatType> >& BestStumps;
    TVector<TSplitter<TSplitterBaseType, TFloatType> >& BestSplitters;

    TVector<TSplitter<TSplitterBaseType, TFloatType> > Splitters;

    const TVector<size_t>& InstanceSplitterNumbers;

    size_t FirstLeafNumber;

    bool DoResetSplitters;

    size_t PoolToUseNumber;

    const TTransformerBase& FeatureTransformer;

    const size_t ActualFeatureNumber;
public:
    TParallelStumpsProducer(const TOptions& options,
                            const TPool<TFloatType>& pool,
                            const TFeatureIterator<TFloatType>& featureIterator,
                            TVector<TStump<TFloatType> >& bestStumps,
                            TVector<TSplitter<TSplitterBaseType, TFloatType> >& bestSplitters,
                            const TVector<size_t>& instanceSplitterNumbers,
                            size_t firstLeafNumber,
                            bool doResetSplitters,
                            size_t poolToUseNumber,
                            const TTransformerBase& featureTransformer,
                            const size_t actualFeatureNumber)
        : Options(options)
        , Pool(pool)
        , FeatureIterator(featureIterator)
        , BestStumps(bestStumps)
        , BestSplitters(bestSplitters)
        , Splitters(bestSplitters)
        , InstanceSplitterNumbers(instanceSplitterNumbers)
        , FirstLeafNumber(firstLeafNumber)
        , DoResetSplitters(doResetSplitters)
        , PoolToUseNumber(poolToUseNumber)
        , FeatureTransformer(featureTransformer)
        , ActualFeatureNumber(actualFeatureNumber)
    {
    }

    void ResetSplitters() {
        for (TSplitter<TSplitterBaseType, TFloatType>& splitter : Splitters) {
            splitter.Clear();
        }

        for (const TIteratorItem<TFloatType>& iteratorItem : FeatureIterator) {
            size_t splitterNumber = InstanceSplitterNumbers[iteratorItem.InstanceNumber];
            if (splitterNumber == Max<size_t>() || splitterNumber < FirstLeafNumber) {
                continue;
            }

            const TInstance<TFloatType>& instance = Pool[iteratorItem.InstanceNumber];
            if (PoolToUseNumber != (size_t) -1 && instance.PoolId != PoolToUseNumber) {
                continue;
            }

            Splitters[splitterNumber - FirstLeafNumber].Add(Pool[iteratorItem.InstanceNumber], FeatureTransformer.Transformation(iteratorItem.Feature));
        }

        TSplitter<TSplitterBaseType, TFloatType>* bestSplitter = BestSplitters.begin();
        for (TSplitter<TSplitterBaseType, TFloatType>& splitter : Splitters) {
            splitter.InitQuality();
            *bestSplitter = splitter;
        }
    }

    void Process(void*) override {
        THolder<TParallelStumpsProducer> self(this);

        if (DoResetSplitters) {
            ResetSplitters();
        }
        if (FeatureIterator.Empty()) {
            return;
        }

        TVector<TFloatType> lastFeatureValues(Splitters.size(), FeatureTransformer.Transformation(FeatureIterator.begin()->Feature));
        for (const TIteratorItem<TFloatType>& iteratorItem : FeatureIterator) {
            size_t splitterNumber = InstanceSplitterNumbers[iteratorItem.InstanceNumber];

            if (splitterNumber == Max<size_t>() || splitterNumber < FirstLeafNumber) {
                continue;
            }

            const TInstance<TFloatType>& instance = Pool[iteratorItem.InstanceNumber];
            if (PoolToUseNumber != (size_t) -1 && instance.PoolId != PoolToUseNumber) {
                continue;
            }

            size_t leafNumber = splitterNumber - FirstLeafNumber;

            TFloatType& lastFeatureValue = lastFeatureValues[leafNumber];
            TSplitter<TSplitterBaseType, TFloatType>& splitter = Splitters[leafNumber];

            TFloatType feature = FeatureTransformer.Transformation(iteratorItem.Feature);
            if (feature > lastFeatureValue + 1e-6 && splitter.IsCorrect(Options)) {
                splitter.UpdateSplitQuality();

                TSplitter<TSplitterBaseType, TFloatType>& bestSplitter = BestSplitters[leafNumber];
                if (splitter.SumSquaredErrors() < bestSplitter.SumSquaredErrors()) {
                    bestSplitter = splitter;

                    TStump<TFloatType>& bestStump = BestStumps[leafNumber];
                    bestStump.FeatureNumber = ActualFeatureNumber;
                    bestStump.FeatureThreshold = (feature + lastFeatureValue) / 2;
                }
            }

            splitter.Move(instance, feature);
            lastFeatureValue = feature;
        }
    }
};

template <typename TSplitterBaseType, typename TFloatType>
class TTreeLearner {
public:
    const TOptions& Options;

    typedef TSplitter<TSplitterBaseType, TFloatType> TSplitterType;
    TVector<TSplitterType> Splitters;

    TVector<TStump<TFloatType> > Stumps;
    TVector<TTreeNode> Nodes;

    TVector<size_t> InstancesSplitterNumbers;

    size_t InnerNodesCount;
    size_t TerminalNodesCount;
    size_t LeafsCount;

    bool DoResetSplitters;

    size_t PoolToUseNumber;

    TTreeLearner(const TPool<TFloatType>& pool,
                 const TOptions& options,
                 const size_t poolToUseNumber,
                 const THashSet<size_t>& testInstanceNumbers,
                 const TSplitterBaseType* splitterBase = nullptr)
        : Options(options)
        , InnerNodesCount(0)
        , TerminalNodesCount(0)
        , LeafsCount(0)
        , PoolToUseNumber(poolToUseNumber)
    {
        TSplitterBaseType mySplitterBase;
        if (!!splitterBase) {
            mySplitterBase = *splitterBase;
            mySplitterBase.UpdateGoals(pool, testInstanceNumbers);
        } else {
            mySplitterBase = TSplitterBaseType(pool, options, testInstanceNumbers);
        }

        DoResetSplitters = mySplitterBase.NeedsReset();

        PushSplitter(mySplitterBase);

        InnerNodesCount = 0;
        TerminalNodesCount = 0;
        LeafsCount = 1;

        TVector<size_t>(pool.GetInstancesCount(), 0).swap(InstancesSplitterNumbers);
        for (const size_t testInstanceNumber : testInstanceNumbers) {
            InstancesSplitterNumbers[testInstanceNumber] = Max<size_t>();
        }
    }

    template <typename TOtherSplitterBaseType>
    TTreeLearner(const TOptions& options,
                 const TTreeLearner<TOtherSplitterBaseType, TFloatType>& source,
                 const TSplitterBaseType& splitterBase,
                 bool needsReset)
        : Options(options)
        , DoResetSplitters(needsReset)
        , PoolToUseNumber(source.PoolToUseNumber)
    {
        Splitters.clear();
        Splitters.reserve(source.Splitters.size());

        for (size_t splitterNumber = 0; splitterNumber != source.Splitters.size(); ++splitterNumber) {
            Splitters.push_back(TSplitterType(splitterBase.CreateEmptyClone()));
        }

        Stumps = source.Stumps;
        Nodes = source.Nodes;

        InstancesSplitterNumbers = source.InstancesSplitterNumbers;

        InnerNodesCount = source.InnerNodesCount;
        TerminalNodesCount = source.TerminalNodesCount;
        LeafsCount = source.LeafsCount;
    }

    void GetTreeData(TVector<TStump<TFloatType> >& stumps, TVector<TTreeNode>& nodes) {
        stumps.swap(Stumps);
        nodes.swap(Nodes);
    }

    size_t GetTerminalLeafsCount() const {
        return TerminalNodesCount;
    }

    size_t GetLeafsCount() const {
        return LeafsCount;
    }

    bool BuildNextLevel(const TPool<TFloatType>& pool,
                        const TVector<TFeatureIterator<TFloatType> >& featureIterators,
                        IThreadPool& queue,
                        const TFeaturesTransformer& featuresTransformer,
                        bool forceSplittersReset = false)
    {
        size_t firstLeafNumber = InnerNodesCount + TerminalNodesCount;

        TVector<TSplitterType> bestSplitters(Splitters.begin() + firstLeafNumber, Splitters.end());
        TVector<TStump<TFloatType> > bestStumps(LeafsCount);

        GetBestSplitters(pool,
                         bestSplitters,
                         bestStumps,
                         firstLeafNumber,
                         featureIterators,
                         InstancesSplitterNumbers,
                         queue,
                         DoResetSplitters || forceSplittersReset,
                         featuresTransformer);

        size_t addedLeafs = 0;
        for (size_t leafNumber = 0; leafNumber < LeafsCount; ++leafNumber) {
            TSplitterType& bestSplitter = bestSplitters[leafNumber];
            if (!bestSplitter.IsCorrect(Options)) {
                ++TerminalNodesCount;
                continue;
            }

            ++InnerNodesCount;

            Splitters[leafNumber + firstLeafNumber] = bestSplitter;
            PushSplitter(bestSplitter.GetLeft());
            PushSplitter(bestSplitter.GetRight());

            size_t nodeNumber = firstLeafNumber + leafNumber;
            size_t left = firstLeafNumber + LeafsCount + addedLeafs++;
            size_t right = firstLeafNumber + LeafsCount + addedLeafs++;

            Nodes[nodeNumber].Left = left;
            Nodes[nodeNumber].Right = right;
            Stumps[nodeNumber] = bestStumps[leafNumber];
        }

        LeafsCount = addedLeafs;

        if (!addedLeafs) {
            return false;
        }

        for (size_t instanceNumber = 0; instanceNumber < InstancesSplitterNumbers.size(); ++instanceNumber) {
            size_t& splitterNumber = InstancesSplitterNumbers[instanceNumber];
            if (splitterNumber == Max<size_t>()) {
                continue;
            }

            const TTreeNode& node = Nodes[splitterNumber];
            if (node.Left == Max<size_t>() || node.Right == Max<size_t>()) {
                continue;
            }

            const TStump<TFloatType>& stump = Stumps[splitterNumber];
            const TInstance<TFloatType>& instance = pool[instanceNumber];
            splitterNumber = instance.Features[stump.FeatureNumber] > stump.FeatureThreshold
                             ? node.Right
                             : node.Left;
        }

        return true;
    }

    template <typename TRegressionSolver>
    void SetupLinearModels(const TPool<TFloatType>& pool,
                           TVector<TLinearModel<TFloatType> >& leafLinearModels,
                           IThreadPool& queue)
    {
        size_t leafsCount = LeafsCount + TerminalNodesCount;
        TVector<TLinearModel<TFloatType> >(leafsCount, TLinearModel<TFloatType>(pool.GetFeaturesCount())).swap(leafLinearModels);

        TVector<std::pair<size_t, size_t> > modelInstancesCount(leafsCount);
        for (size_t i = 0; i < leafsCount; ++i) {
            modelInstancesCount[i].second = i;
        }

        TVector<size_t> instanceLinearModelNumbers(pool.GetInstancesCount());
        {
            size_t nextLinearModel = 0;
            for (TTreeNode& node : Nodes) {
                if (node.Left >= Nodes.size() || node.Right >= Nodes.size()) {
                    node.Left = nextLinearModel;
                    node.Right = nextLinearModel;
                    ++nextLinearModel;
                }
            }
        }

        for (size_t instanceNumber = 0; instanceNumber < pool.GetInstancesCount(); ++instanceNumber) {
            if (PoolToUseNumber != (size_t) -1 && pool[instanceNumber].PoolId != PoolToUseNumber) {
                continue;
            }

            size_t splitterNumber = InstancesSplitterNumbers[instanceNumber];
            if (splitterNumber == Max<size_t>()) {
                instanceLinearModelNumbers[instanceNumber] = Max<size_t>();
                continue;
            }

            TTreeNode& node = Nodes[splitterNumber];

            ++modelInstancesCount[node.Left].first;
            instanceLinearModelNumbers[instanceNumber] = node.Left;
        }

        Sort(modelInstancesCount.begin(), modelInstancesCount.end(), TGreater<>());

        TVector<TRegressionSolver> leafRegressionSolvers(leafsCount, TRegressionSolver(pool, Options, THashSet<size_t>(), false));

        queue.Start(Options.ThreadsCount);
        for (size_t leafNumber = 0; leafNumber < leafsCount; ++leafNumber) {
            size_t modelNumber = modelInstancesCount[leafNumber].second;
            queue.SafeAddFunc([&, modelNumber](){
                TRegressionSolver& regressionSolver = leafRegressionSolvers[modelNumber];
                TLinearModel<TFloatType>& linearModel = leafLinearModels[modelNumber];

                const size_t* instanceLinearModelNumber = instanceLinearModelNumbers.begin();
                for (const TInstance<TFloatType>& instance : pool) {
                    if (PoolToUseNumber != (size_t) -1 && instance.PoolId != PoolToUseNumber) {
                        ++instanceLinearModelNumber;
                        continue;
                    }

                    if (*instanceLinearModelNumber != Max<size_t>() &&
                        (*instanceLinearModelNumber == modelNumber || modelNumber == Max<size_t>()))
                    {
                        regressionSolver.AddInstance(instance);
                    }
                    ++instanceLinearModelNumber;
                }

                regressionSolver.Solve(linearModel);
            });
        }
        queue.Stop();

        Prune(pool, leafRegressionSolvers, leafLinearModels, queue);
    }
private:
    template <typename TRegressionSolver>
    void Prune(const TPool<TFloatType>& pool,
               const TVector<TRegressionSolver>& leafRegressionSolvers,
               TVector<TLinearModel<TFloatType> >& leafLinearModels,
               IThreadPool& queue)
    {
        if (!Options.PruneFactor || Splitters.size() < 2) {
            return;
        }

        TVector<size_t> parents(Splitters.size(), (size_t) -1);
        TVector<bool> leafs(Splitters.size(), false);
        {
            for (const TTreeNode& node : Nodes) {
                if (node.Left == node.Right) {
                    leafs[&node - Nodes.begin()] = true;
                    continue;
                }

                parents[node.Left] = &node - Nodes.begin();
                parents[node.Right] = &node - Nodes.begin();
            }
        }

        TVector<TRegressionSolver> regressionSolvers(Splitters.size(), TRegressionSolver(pool, Options, THashSet<size_t>(), false));
        {
            size_t leafNumber = LeafsCount + TerminalNodesCount - 1;
            for (size_t nodeNumber = Splitters.size(); nodeNumber != 1; --nodeNumber) {
                if (leafs[nodeNumber - 1]) {
                    regressionSolvers[nodeNumber - 1] = leafRegressionSolvers[leafNumber--];
                }
                regressionSolvers[parents[nodeNumber - 1]] += regressionSolvers[nodeNumber - 1];
            }
        }

        TVector<TLinearModel<TFloatType> > linearModels(Splitters.size(), TLinearModel<TFloatType>(pool.GetFeaturesCount()));
        TVector<TTreeNodePruneInfo<TFloatType> > nodePruneInfos(Splitters.size());

        queue.Start(Options.ThreadsCount);
        for (size_t splitterNumber = 0; splitterNumber < Splitters.size(); ++splitterNumber) {
            queue.SafeAddFunc([&, splitterNumber](){
                TRegressionSolver& regressionSolver = regressionSolvers[splitterNumber];
                TLinearModel<TFloatType>& linearModel = linearModels[splitterNumber];

                regressionSolver.Solve(linearModel);
                nodePruneInfos[splitterNumber] = TTreeNodePruneInfo<TFloatType>(linearModel,
                                                                                regressionSolver.SumSquaredErrors(linearModel),
                                                                                regressionSolver.GetInstancesCount());
            });
        }
        queue.Stop();

        for (size_t nodeNumber = Splitters.size(); nodeNumber != 1; --nodeNumber) {
            if (leafs[nodeNumber - 1]) {
                continue;
            }

            const TTreeNode& node = Nodes[nodeNumber - 1];
            TTreeNodePruneInfo<TFloatType>& nodePruneInfo = nodePruneInfos[nodeNumber - 1];

            nodePruneInfo.SetChildren(nodePruneInfos[node.Left], nodePruneInfos[node.Right]);
            if (nodePruneInfo.NeedsPruning(Options.PruneFactor)) {
                nodePruneInfo.Prune();
            }
        }

        TVector<TLinearModel<TFloatType> > newLinearModels;
        for (size_t i = 0; i < Nodes.size(); ++i) {
            if (nodePruneInfos[i].Pruned || leafs[i]) {
                Nodes[i].Left = newLinearModels.size();
                Nodes[i].Right = newLinearModels.size();
                newLinearModels.push_back(linearModels[i]);
            }
        }
        newLinearModels.swap(leafLinearModels);
    }

    void PushSplitter(const TSplitterBaseType& splitterBase) {
        Splitters.push_back(TSplitterType(splitterBase));

        Stumps.push_back(TStump<TFloatType>());
        Nodes.push_back(TTreeNode());
    }

    void GetBestSplitters(const TPool<TFloatType>& pool,
                          TVector<TSplitterType>& bestSplitters,
                          TVector<TStump<TFloatType> >& bestStumps,
                          size_t firstLeafNumber,
                          const TVector<TFeatureIterator<TFloatType> >& iterators,
                          const TVector<size_t>& instanceSplitterNumbers,
                          IThreadPool& queue,
                          bool doResetSplitters,
                          const TFeaturesTransformer& featuresTransformer)
    {
        size_t iteratorsCount = iterators.size();

        TVector<TVector<TSplitterType> > threadLocalBestSplitters(iteratorsCount, bestSplitters);
        TVector<TVector<TStump<TFloatType> > > threadLocalBestStumps(iteratorsCount, bestStumps);

        queue.Start(Options.ThreadsCount);

        size_t actualFeatureNumber = 0;
        for (size_t i = 0; i < iteratorsCount; ++i) {
            if (Options.IgnoredFeatures.contains(iterators[i].GetFeatureNumber())) {
                continue;
            }

            queue.SafeAdd(new TParallelStumpsProducer<TSplitterBaseType, TFloatType>(Options,
                                                                                     pool,
                                                                                     iterators[i],
                                                                                     threadLocalBestStumps[i],
                                                                                     threadLocalBestSplitters[i],
                                                                                     instanceSplitterNumbers,
                                                                                     firstLeafNumber,
                                                                                     doResetSplitters,
                                                                                     PoolToUseNumber,
                                                                                     featuresTransformer[actualFeatureNumber],
                                                                                     actualFeatureNumber));
            ++actualFeatureNumber;
        }

        queue.Stop();

        size_t bestSplittersCount = bestSplitters.size();
        for (size_t i = 0; i < iteratorsCount; ++i) {
            if (iterators[i].Empty()) {
                continue;
            }

            TVector<TSplitterType>& splitters = threadLocalBestSplitters[i];
            TVector<TStump<TFloatType> >& stumps = threadLocalBestStumps[i];
            for (size_t j = 0; j < bestSplittersCount; ++j) {
                if (splitters[j].SumSquaredErrors() < bestSplitters[j].SumSquaredErrors()) {
                    bestSplitters[j] = splitters[j];
                    bestStumps[j] = stumps[j];
                }
            }
        }
    }
};

template <typename TFloatType>
class TLeastSquaresTree {
public:
    TVector<TStump<TFloatType> > Stumps;
    TVector<TTreeNode> Nodes;

    TVector<TLinearModel<TFloatType> > LinearModels;
public:
    Y_SAVELOAD_DEFINE(Stumps, Nodes, LinearModels)

    void InceptWeight(TFloatType weight) {
        for (TLinearModel<TFloatType>& linearModel : LinearModels) {
            linearModel.InceptWeight(weight);
        }
    }

    template <typename T>
    TFloatType Prediction(const TVector<T>& features) const {
        return LinearModels[Nodes[GetNodeNumber(features)].Left].Prediction(features);
    }

    void Learn(const TPool<TFloatType>& pool,
               const TVector<TFeatureIterator<TFloatType> >& featureIterators,
               const TOptions& options,
               const size_t poolToUseNumber,
               const TFeaturesTransformer& featuresTransformer,
               const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
    {
        if (options.SplitterType == DeviationSplitter) {
            ProcessTreeLearner<TDeviationSplitterBase<TFloatType> >(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer);
        } else if (options.SplitterType == SimpleLinearRegressionSplitter) {
            ProcessTreeLearner<TSimpleLinearRegressionSplitterBase<TFloatType> >(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer);
        } else if (options.SplitterType == BestSimpleLinearRegressionSplitter) {
            ProcessTreeLearner<TBestSimpleLinearRegressionSplitterBase<TFloatType> >(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer);
        } else if (options.SplitterType == PositionalSimpleLinearRegressionSplitter) {
            ProcessTreeLearner<TPositionalLinearRegressionSplitterBase<TFloatType> >(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer);
        } else if (options.SplitterType == LinearRegressionSplitter) {
            ProcessTreeLearner<TLinearRegressionSplitterBase<TFloatType> >(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer);
        }
    }

    template <typename TSplitterBaseType>
    void Learn(const TPool<TFloatType>& pool,
               const TVector<TFeatureIterator<TFloatType> >& featureIterators,
               const TOptions& options,
               const size_t poolToUseNumber,
               const TSplitterBaseType& splitterBase,
               const TFeaturesTransformer& featuresTransformer,
               const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>())
    {
        ProcessTreeLearner<TSplitterBaseType>(pool, featureIterators, options, poolToUseNumber, testInstanceNumbers, featuresTransformer, &splitterBase);
    }

    void Print(IOutputStream& out, const TString& preffix = "") const {
        Print(out, preffix, 0);
    }

    void Fix(const size_t featuresCount, const TSet<size_t>& ignoredFeatures) {
        for (TStump<TFloatType>& stump : Stumps) {
            for (const size_t ignoredFeature : ignoredFeatures) {
                if (ignoredFeature > stump.FeatureNumber) {
                    break;
                }
                ++stump.FeatureNumber;
            }
        }

        for (TLinearModel<TFloatType>& linearModel : LinearModels) {
            linearModel.Fix(featuresCount, ignoredFeatures);
        }
    }
private:
    template <typename TSplitterBaseType>
    void ProcessTreeLearner(const TPool<TFloatType>& pool,
                            const TVector<TFeatureIterator<TFloatType> >& featureIterators,
                            const TOptions& options,
                            const size_t poolToUseNumber,
                            const THashSet<size_t>& testInstanceNumbers,
                            const TFeaturesTransformer& featuresTransformer,
                            const TSplitterBaseType* splitterBase = NULL)
    {
        THolder<IThreadPool> queue(CreateThreadPool(options.ThreadsCount));

        if (!options.LastSplitOnly) {
            TTreeLearner<TSplitterBaseType, TFloatType> treeLearner(pool, options, poolToUseNumber, testInstanceNumbers, splitterBase);

            for (size_t level = 0; level < options.MaxDepth; ++level) {
                if (!treeLearner.BuildNextLevel(pool, featureIterators, *queue, featuresTransformer)) {
                    break;
                }
            }

            if (options.SolverType == ConstantSolver) {
                treeLearner.template SetupLinearModels<TDeviationSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == BestSimpleLinearRegressionSplitterSolver) {
                treeLearner.template SetupLinearModels<TBestSimpleLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == PositionalSimpleLinearRegressionSplitterSolver) {
                treeLearner.template SetupLinearModels<TPositionalLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == LinearRegressionSolver) {
                treeLearner.template SetupLinearModels<TLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            }

            treeLearner.GetTreeData(Stumps, Nodes);
        } else {
            TTreeLearner<TDeviationSplitterBase<TFloatType>, TFloatType> deviationTreeLearner(pool, options, poolToUseNumber, testInstanceNumbers);
            for (size_t level = 0; level + 1 < options.MaxDepth; ++level) {
                if (!deviationTreeLearner.BuildNextLevel(pool, featureIterators, *queue, featuresTransformer)) {
                    break;
                }
            }

            TSplitterBaseType mySplitterBase(pool, options, testInstanceNumbers, false);
            TTreeLearner<TSplitterBaseType, TFloatType> lastSplitTreeLearner(options, deviationTreeLearner, mySplitterBase, true);
            lastSplitTreeLearner.BuildNextLevel(pool, featureIterators, *queue, featuresTransformer, true);

            if (options.SolverType == ConstantSolver) {
                lastSplitTreeLearner.template SetupLinearModels<TDeviationSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == BestSimpleLinearRegressionSplitterSolver) {
                lastSplitTreeLearner.template SetupLinearModels<TBestSimpleLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == PositionalSimpleLinearRegressionSplitterSolver) {
                lastSplitTreeLearner.template SetupLinearModels<TPositionalLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            } else if (options.SolverType == LinearRegressionSolver) {
                lastSplitTreeLearner.template SetupLinearModels<TLinearRegressionSplitterBase<TFloatType> >(pool, LinearModels, *queue);
            }

            lastSplitTreeLearner.GetTreeData(Stumps, Nodes);
        }
    }

    template <typename T>
    size_t GetNodeNumber(const TVector<T>& features) const {
        size_t nodeNumber = 0;
        while(Nodes[nodeNumber].Left != Nodes[nodeNumber].Right) {
            nodeNumber = features[Stumps[nodeNumber].FeatureNumber] > Stumps[nodeNumber].FeatureThreshold
                         ? Nodes[nodeNumber].Right
                         : Nodes[nodeNumber].Left;
        }
        return nodeNumber;
    }

    void Print(IOutputStream& out, const TString& preffix, size_t nodeNumber) const {
        const TTreeNode& node = Nodes[nodeNumber];
        if (node.Left == node.Right) {
            LinearModels[node.Right].Print(out, preffix);
            return;
        }

        const TStump<TFloatType>& stump = Stumps[nodeNumber];
        out << preffix << "attr" << stump.FeatureNumber << " < " << Sprintf("%.7lf:\n", stump.FeatureThreshold);
        Print(out, preffix + "  ", node.Left);
        out << preffix << "attr" << stump.FeatureNumber << " > " << Sprintf("%.7lf:\n", stump.FeatureThreshold);
        Print(out, preffix + "  ", node.Right);
    }
};

}
