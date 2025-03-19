#pragma once

#include "least_squares_tree.h"
#include "loss_functions.h"
#include "options.h"
#include "transform_features.h"

#include "quad.h"
#include "logistic.h"
#include "logistic_query_reg.h"
#include "pairwise.h"
#include "rank.h"
#include "surplus.h"
#include "surplus_new.h"

#include <kernel/ethos/lib/reg_tree_applier_lib/model.h>

#include <kernel/matrixnet/mn_dynamic.h>

#include <util/datetime/cputimer.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/memory/blob.h>
#include <util/stream/file.h>
#include <util/string/join.h>
#include <util/system/fs.h>

#include <util/ysaveload.h>

namespace NRegTree {

static const ui32 ModelFormatVersion = 4;

template <typename TFloatType>
inline TAutoPtr<TLossFunctionBase<TFloatType> > LossFunctionByName(const TString& name) {
    if (name == "QUAD") {
        return new TQuadLossFunction<TFloatType>();
    } else if (name == "LOGISTIC") {
        return new TLogLossFunction<TFloatType>();
    } else if (name == "QUERY_REGULARIZED_LOGISTIC") {
        return new TQueryRegularizedLogLossLossFunction<TFloatType>();
    } else if (name == "PAIRWISE") {
        return new TPairwiseLossFunction<TFloatType>();
    } else if (name == "RANK") {
        return new TRankLossFunction<TFloatType>();
    } else if (name == "SURPLUS") {
        return new TSurplusLossFunction<TFloatType>();
    } else if (name == "NEW_SURPLUS") {
        return new TNewSurplusLossFunction<TFloatType>();
    }
    ythrow yexception() << "bad loss function name";
}

template <typename TFloatType>
class TBoosting {
public:
    TVector<TLeastSquaresTree<TFloatType> > Trees;
    TSimpleSharedPtr<TLossFunctionBase<TFloatType> > LossFunction;

    TString PositiveMark;

    TFloatType Bias;

    TFeaturesTransformer FeaturesTransformer;
public:
    typedef TFloatType TFeatureType;
    typedef TInstance<TFloatType> TInstanceType;
    typedef TPool<TFloatType> TPoolType;

    TBoosting() {
    }

    TBoosting(const TBoosting& source) {
        *this = source;
    }

    TBoosting<TFloatType>& operator = (const TBoosting& source) {
        Trees = source.Trees;
        if (!!source.LossFunction) {
            LossFunction = LossFunctionByName<TFloatType>(source.LossFunction->Name());
        }

        PositiveMark = source.PositiveMark;

        Bias = source.Bias;

        return *this;
    }

    TVector<TVector<TMetricWithValues> > LearnMutable(TPool<TFeatureType>& mutableLearnPool,
                                                      const TVector<TFeatureIterator<TFeatureType> >& featureIterators,
                                                      TOptions options,
                                                      const TString& outFileName = TString(),
                                                      const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>(),
                                                      TPool<TFeatureType>* mutableTestPool = nullptr)
    {
        TSimpleTimer timer;

        size_t originalFeaturesCount = mutableLearnPool.GetFeaturesCount();
        mutableLearnPool.ClearIgnoredFeatures(options);
        if (mutableTestPool) {
            mutableTestPool->ClearIgnoredFeatures(options);
        }

        options.SetupDependentValues(mutableLearnPool.GetInstancesCount() - testInstanceNumbers.size(), mutableLearnPool.GetFeaturesCount());

        mutableLearnPool.ModifyPositivesWeight(options);
        if (mutableTestPool) {
            mutableTestPool->ModifyPositivesWeight(options);
        }

        PositiveMark = options.PositiveMark;

        LossFunction = LossFunctionByName<TFloatType>(options.LossFunctionName).Release();

        FeaturesTransformer.Learn(mutableLearnPool, options, testInstanceNumbers);
        FeaturesTransformer.TransformPool(mutableLearnPool);
        if (!!mutableTestPool) {
            FeaturesTransformer.TransformPool(*mutableTestPool);
        }
        FeaturesTransformer.Fix(originalFeaturesCount, options.IgnoredFeatures);

        LossFunction->InitializeGoals(mutableLearnPool, options);

        Bias = 0.;

        TVector<TVector<TMetricWithValues> > metrics;
        if (options.SplitterType == DeviationSplitter) {
            metrics = BuildTrees<TDeviationSplitterBase<TFloatType> >(options, mutableLearnPool, featureIterators, testInstanceNumbers, mutableTestPool, outFileName, originalFeaturesCount);
        } else if (options.SplitterType == SimpleLinearRegressionSplitter) {
            metrics = BuildTrees<TSimpleLinearRegressionSplitterBase<TFloatType> >(options, mutableLearnPool, featureIterators, testInstanceNumbers, mutableTestPool, outFileName, originalFeaturesCount);
        } else if (options.SplitterType == BestSimpleLinearRegressionSplitter) {
            metrics = BuildTrees<TBestSimpleLinearRegressionSplitterBase<TFloatType> >(options, mutableLearnPool, featureIterators, testInstanceNumbers, mutableTestPool, outFileName, originalFeaturesCount);
        } else if (options.SplitterType == PositionalSimpleLinearRegressionSplitter) {
            metrics = BuildTrees<TPositionalLinearRegressionSplitterBase<TFloatType> >(options, mutableLearnPool, featureIterators, testInstanceNumbers, mutableTestPool, outFileName, originalFeaturesCount);
        } else if (options.SplitterType == LinearRegressionSplitter) {
            metrics = BuildTrees<TLinearRegressionSplitterBase<TFloatType> >(options, mutableLearnPool, featureIterators, testInstanceNumbers, mutableTestPool, outFileName, originalFeaturesCount);
        }

        Bias += options.Bias;

        if (options.OutputDetailedTreeInfo) {
            Cout << "learned in " << timer.Get() << "\n";
        }

        return metrics;
    }

    void SetupBias(const double bias) {
        Bias = bias;
    }

    TVector<TVector<TMetricWithValues> > Learn(const TPool<TFeatureType>& learnPool,
                                               const TVector<TFeatureIterator<TFeatureType> >& featureIterators,
                                               const TOptions& options,
                                               const TString& outFileName = TString(),
                                               const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>(),
                                               TPool<TFeatureType>* testPool = nullptr)
    {
        TPool<TFeatureType> learnPoolCopy = learnPool; // copy is needed here
        THolder<TPool<TFeatureType> > testPoolCopy;
        if (!!testPool) {
            testPoolCopy.Reset(new TPool<TFeatureType>(*testPool));
        }
        return LearnMutable(learnPoolCopy, featureIterators, options, outFileName, testInstanceNumbers, testPoolCopy.Get());
    }

    TVector<TVector<TMetricWithValues> > Learn(const TPool<TFeatureType>& learnPool,
                                               const TOptions& options,
                                               const TString& outFileName = TString(),
                                               const THashSet<size_t>& testInstanceNumbers = THashSet<size_t>(),
                                               TPool<TFeatureType>* testPool = nullptr)
    {
        return Learn(learnPool, BuildFeatureIterators(learnPool, options), options, outFileName, testInstanceNumbers, testPool);
    }

    template <typename TSplitterBaseType>
    TVector<TVector<TMetricWithValues> > BuildTrees(const TOptions& options,
                                                    TPool<TFeatureType>& learnPool,
                                                    const TVector<TFeatureIterator<TFeatureType> >& featureIterators,
                                                    const THashSet<size_t>& testInstanceNumbers,
                                                    TPool<TFeatureType>* testPool,
                                                    const TString& outFileName,
                                                    const size_t originalFeaturesCount)
    {
        TVector<TVector<TMetricWithValues> > metrics;

        TSplitterBaseType baseSplitter(learnPool, options, testInstanceNumbers);

        size_t poolToUseNumber = options.AlternatePools ? 0 : (size_t) -1;
        for (size_t iteration = 0; iteration < options.BoostingIterationsCount; ++iteration) {
            TSimpleTimer timer;

            TLeastSquaresTree<TFeatureType> tree;

            if (options.AlternatePools) {
                do {
                    poolToUseNumber = (poolToUseNumber + 1) % learnPool.GetPoolsCount();
                } while (learnPool.GetPoolQueryWeight(poolToUseNumber) == 0);
            }

            tree.Learn(learnPool, featureIterators, options, poolToUseNumber, baseSplitter, FeaturesTransformer, testInstanceNumbers);
            metrics = LossFunction->ProcessAllPools(learnPool, tree, options, testInstanceNumbers, testPool, poolToUseNumber, iteration == 0, &Bias);

            tree.Fix(originalFeaturesCount, options.IgnoredFeatures);
            Trees.push_back(tree);

            if (options.AlternatePools) {
                Trees.back().InceptWeight(learnPool.GetPoolQueryWeight(poolToUseNumber));
            }

            if (!!outFileName) {
                {
                    TFixedBufferFileOutput modelOut(outFileName + ".new");
                    Save(&modelOut);
                }
                NFs::Rename(outFileName + ".new", outFileName);
            }

            if (options.OutputDetailedTreeInfo) {
                if (poolToUseNumber == (size_t) -1) {
                    Cout << "tree #" << (iteration + 1) << ": " << timer.Get() << "\n";
                } else {
                    Cout << "tree #" << (iteration + 1) << " for pool #" << poolToUseNumber << ": " << timer.Get() << "\n";
                }
                PrintMetrics(Cout, metrics);
                Cout << Endl;
            }
        }

        if (!!outFileName) {
            {
                TFixedBufferFileOutput modelOut(outFileName + ".new");
                Save(&modelOut);
            }
            NFs::Rename(outFileName + ".new", outFileName);
        }

        return metrics;
    }

    TVector<TMetricWithValues> Test(TPool<TFeatureType>& testPool, const TOptions& options) {
        for (TInstance<TFeatureType>& instance : testPool) {
            instance.Prediction = NonTransformedPrediction(instance.Features);
        }
        TVector<TMetricWithValues> metrics = LossFunction->GetMetrics(testPool, options);
        testPool.Restore();
        return metrics;
    }

    template <typename T>
    TFeatureType NonTransformedPrediction(const TVector<T>& features) const {
        if (FeaturesTransformer.IsFake()) {
            return SimplePrediction(features);
        }

        TVector<T> featuresCopy(features);
        FeaturesTransformer.TransformFeatures(featuresCopy);

        return SimplePrediction(featuresCopy);
    }

    template <typename T>
    TFeatureType Prediction(const TVector<T>& features) const {
        return LossFunction->TransformPrediction(NonTransformedPrediction(features));
    }

    void Save(IOutputStream* out) const {
        ::Save(out, TString("REGTREE"));

        ::Save(out, ModelFormatVersion);
        ::Save(out, Trees);
        ::Save(out, LossFunction->Name());
        ::Save(out, PositiveMark);
        ::Save(out, Bias);

        ::Save(out, FeaturesTransformer);
    }

    void Load(IInputStream* in) {
        TString name;
        ::Load(in, name);

        Y_ASSERT(name == "REGTREE");

        ui32 modelFormatVersion;
        ::Load(in, modelFormatVersion);

        if (modelFormatVersion == 3) {
            ::Load(in, Trees);

            TString lossFunctionName;
            ::Load(in, lossFunctionName);

            LossFunction.Reset(LossFunctionByName<TFloatType>(lossFunctionName).Release());

            ::Load(in, PositiveMark);

            bool logify;
            ::Load(in, logify);
            ::Load(in, Bias);

            return;
        }

        if (modelFormatVersion != ModelFormatVersion) {
            ythrow yexception() << "Bad reg_tree model format version: " << modelFormatVersion
                                << ", current version is: " << ModelFormatVersion;
        }

        ::Load(in, Trees);

        TString lossFunctionName;
        ::Load(in, lossFunctionName);

        LossFunction.Reset(LossFunctionByName<TFloatType>(lossFunctionName).Release());

        ::Load(in, PositiveMark);
        ::Load(in, Bias);

        ::Load(in, FeaturesTransformer);
    }

    void LoadFromFile(const TString& filename) {
        TFileInput in(filename);
        Load(&in);
    }

    void SaveToFile(const TString& filename) const {
        TFixedBufferFileOutput out(filename);
        Save(&out);
    }

    void Print(IOutputStream& out, const TString& preffix = "") const {
        for (const TLeastSquaresTree<TFloatType>& tree : Trees) {
            tree.Print(out, preffix);
            out << "\n";
        }
        out << "bias: " << Sprintf("%.7lf\n", Bias);
    }
private:
    template <typename T>
    TFeatureType SimplePrediction(const TVector<T>& features) const {
        TFeatureType result = Bias;
        for (const TLeastSquaresTree<TFeatureType>* tree = Trees.begin(); tree != Trees.end(); ++tree) {
            result += tree->Prediction(features);
        }
        return result;
    }
};

typedef TBoosting<double> TRegressionModel;
typedef TBoosting<float> TFloatRegressionModel;

struct TCompactStumpInfo {
    const TCompactStumpInfo* LeftChild;
    const float* LinearModel;

    ui32 FeatureNumber;
    float FeatureThreshold;
};

class TCompactBoosting : public NMatrixnet::IRelevCalcer {
private:
    ui32 FeaturesCount;
    ui32 TreesCount;

    TVector<TCompactStumpInfo> Stumps;
    TVector<TCompactStumpInfo*> Roots;

    TVector<float> LinearModels;

    TString PositiveMark;

    float Bias;

    TFeaturesTransformer FeaturesTransformer;

    TSimpleSharedPtr<TLossFunctionBase<float> > LossFunction;
public:
    TCompactModel ToCompactModel() const {
        TCompactModel compactModel;

        compactModel.Bias = Bias;
        compactModel.FeaturesCount = FeaturesCount;
        compactModel.LinearModels = LinearModels;

        for (size_t stumpId = 0; stumpId < Stumps.size(); ++stumpId) {
            TSimpleCompactStumpInfo simpleStump;
            if (Stumps[stumpId].LeftChild) {
                simpleStump.LeftChildIdx = Stumps[stumpId].LeftChild - Stumps.begin();
            }
            simpleStump.FeatureNumber = Stumps[stumpId].FeatureNumber;
            simpleStump.FeatureThreshold = Stumps[stumpId].FeatureThreshold;
            simpleStump.LinearModelOffset = Stumps[stumpId].LinearModel - LinearModels.begin();
            compactModel.Stumps.push_back(simpleStump);
        }

        for (const TCompactStumpInfo* root : Roots) {
            compactModel.RootIds.push_back(root - Stumps.begin());
        }

        return compactModel;
    }

    double DoCalcRelev(const float* features) const override {
        return NonTransformedPrediction(features);
    }

    size_t GetNumFeats() const override {
        return FeaturesCount;
    }

    TString ToPythonCode(const TString& suffix = TString()) const {
        TStringStream pythonCode;

        pythonCode << "modelBias" << suffix << " = " << Bias << "\n";
        pythonCode << "modelFeaturesCount" << suffix << " = " << FeaturesCount << "\n";
        pythonCode << "modelTreesCount" << suffix << " = " << TreesCount << "\n";
        pythonCode << "\n";

        pythonCode << "modelStumps" << suffix << " = [\n    ";
        for (size_t stumpNumber = 0; stumpNumber < Stumps.size(); ++stumpNumber) {
            const TCompactStumpInfo& stump = Stumps[stumpNumber];

            size_t featureNumber = stump.FeatureNumber;
            float featureThreshold = stump.FeatureThreshold;

            size_t leftChildNumber = stump.LeftChild ? stump.LeftChild - Stumps.begin() : 0;
            size_t linearModelElementNumber = stump.LinearModel ? stump.LinearModel - LinearModels.begin() : 0;

            pythonCode << "[" << featureNumber << ", "
                              << featureThreshold << ", "
                              << leftChildNumber << ", "
                              << linearModelElementNumber << "],";
            if ((stumpNumber + 1) % 10 == 0) {
                pythonCode << "\n    ";
            } else {
                pythonCode << " ";
            }
        }
        pythonCode << "\n]\n\n";

        pythonCode << "modelLinearModels" << suffix << " = [\n    ";
        for (size_t linearModelCoefficientNumber = 0;
             linearModelCoefficientNumber < LinearModels.size();
             ++linearModelCoefficientNumber)
        {
            pythonCode << LinearModels[linearModelCoefficientNumber] << ",";
            if ((linearModelCoefficientNumber + 1) % 10 == 0) {
                pythonCode << "\n    ";
            } else {
                pythonCode << " ";
            }
        }
        pythonCode << "\n]\n\n";

        pythonCode << "modelRoots" << suffix << " = [\n";
        for (const TCompactStumpInfo* root : Roots) {
            pythonCode << "" << (root - Stumps.begin()) << ",\n";
        }
        pythonCode << "]\n";
        pythonCode << "\n";

        pythonCode << "def ModelPrediction" << suffix << "(features):\n";
        pythonCode << "prediction = modelBias" << suffix << "\n";
        pythonCode << "for treeNumber in range(modelTreesCount" << suffix << "):\n";
        pythonCode << "    stumpNumber = modelRoots" << suffix << "[treeNumber]\n";
        pythonCode << "    while modelStumps" << suffix << "[stumpNumber][2]:\n";
        pythonCode << "        if features[modelStumps" << suffix << "[stumpNumber][0]] > modelStumps" << suffix << "[stumpNumber][1]:\n";
        pythonCode << "            stumpNumber = modelStumps" << suffix << "[stumpNumber][2] + 1\n";
        pythonCode << "        else:\n";
        pythonCode << "            stumpNumber = modelStumps" << suffix << "[stumpNumber][2]\n";
        pythonCode << "    linearModelElementNumber = modelStumps" << suffix << "[stumpNumber][3]\n";
        pythonCode << "    prediction += modelLinearModels" << suffix << "[linearModelElementNumber]\n";
        pythonCode << "    for featureNumber in range(modelFeaturesCount" << suffix << "):\n";
        pythonCode << "        prediction += modelLinearModels" << suffix << "[linearModelElementNumber + featureNumber + 1] * features[featureNumber]\n";
        pythonCode << "return prediction\n";

        return pythonCode.Str();
    }

    TString ToCppCode() const {
        TStringStream cppCode;

        cppCode << "const double modelBias = " << Bias << ";\n";
        cppCode << "const size_t modelFeaturesCount = " << FeaturesCount << ";\n";
        cppCode << "const size_t modelTreesCount = " << TreesCount << ";\n";
        cppCode << "\n";

        cppCode << "const double modelStumps[" << Stumps.size() << "][" << 4 << "] = {\n";
        for (size_t stumpNumber = 0; stumpNumber < Stumps.size(); ++stumpNumber) {
            const TCompactStumpInfo& stump = Stumps[stumpNumber];

            size_t featureNumber = stump.FeatureNumber;
            float featureThreshold = stump.FeatureThreshold;

            size_t leftChildNumber = stump.LeftChild ? stump.LeftChild - Stumps.begin() : 0;
            size_t linearModelElementNumber = stump.LinearModel ? stump.LinearModel - LinearModels.begin() : 0;

            cppCode << "{" << featureNumber << ", "
                           << featureThreshold << ", "
                           << leftChildNumber << ", "
                           << linearModelElementNumber << "},";
            if ((stumpNumber + 1) % 10 == 0) {
                cppCode << "\n";
            }
        }
        cppCode << "\n};\n\n";

        cppCode << "const double modelLinearModels[] = {\n";
        for (size_t linearModelCoefficientNumber = 0;
             linearModelCoefficientNumber < LinearModels.size();
             ++linearModelCoefficientNumber)
        {
            cppCode << " " << LinearModels[linearModelCoefficientNumber] << ",";
            if ((linearModelCoefficientNumber + 1) % 10 == 0) {
                cppCode << "\n";
            }
        }
        cppCode << "\n};\n\n";

        cppCode << "const size_t modelRoots[] = {\n";
        for (const TCompactStumpInfo* root : Roots) {
            cppCode << "" << (root - Stumps.begin()) << ",\n";
        }
        cppCode << "};\n";
        cppCode << "\n";

        cppCode << "double prediction = modelBias;\n";
        cppCode << "for (size_t treeNumber = 0; treeNumber < modelTreesCount; ++treeNumber) {\n";
        cppCode << "    size_t stumpNumber = modelRoots[treeNumber];\n";
        cppCode << "    while (modelStumps[stumpNumber][2]) {\n";
        cppCode << "        if (features[modelStumps[stumpNumber][0]] > modelStumps[stumpNumber][1]) {\n";
        cppCode << "            stumpNumber = modelStumps[stumpNumber][2] + 1;\n";
        cppCode << "        } else {\n";
        cppCode << "            stumpNumber = modelStumps[stumpNumber][2];\n";
        cppCode << "        }\n";
        cppCode << "    }\n";
        cppCode << "    const size_t linearModelElementNumber = modelStumps[stumpNumber][3];\n";
        cppCode << "    prediction += modelLinearModels[linearModelElementNumber];\n";
        cppCode << "    for (size_t featureNumber = 0; featureNumber < modelFeaturesCount; ++featureNumber) {\n";
        cppCode << "        prediction += modelLinearModels[linearModelElementNumber + featureNumber + 1] * features[featureNumber];\n";
        cppCode << "    }\n";
        cppCode << "}\n";
        return cppCode.Str();
    }

    TCompactBoosting() {
    }

    TCompactBoosting(const TString& modelFileName) {
        Build(modelFileName);
    }

    template <typename TFloatType>
    TCompactBoosting(const TBoosting<TFloatType>& regressionModel) {
        Build(regressionModel);
    }

    void Build(const TString& modelFileName) {
        TRegressionModel model;
        TFileInput modelIn(modelFileName);
        model.Load(&modelIn);
        Build(model);
    }

    template <typename TFloatType>
    void Build(const TBoosting<TFloatType>& regressionModel) {
        FeaturesTransformer = regressionModel.FeaturesTransformer;

        LossFunction = LossFunctionByName<float>(regressionModel.LossFunction->Name()).Release();
        PositiveMark = regressionModel.PositiveMark;
        Bias = regressionModel.Bias;

        FeaturesCount = regressionModel.Trees.front().LinearModels.front().Coefficients.size();
        TreesCount = regressionModel.Trees.size();

        size_t totalStumpsCount = 0;
        size_t totalLinearModelsCount = 0;
        for (const TLeastSquaresTree<TFloatType>* tree = regressionModel.Trees.begin(); tree != regressionModel.Trees.end(); ++tree) {
            totalStumpsCount += tree->Stumps.size();
            totalLinearModelsCount += tree->LinearModels.size();
        }

        LinearModels.reserve(totalLinearModelsCount * (FeaturesCount + 1));
        Stumps.resize(totalStumpsCount);

        size_t linearModelsOffset = 0;
        size_t stumpsOffset = 0;
        for (const TLeastSquaresTree<TFloatType>* tree = regressionModel.Trees.begin(); tree != regressionModel.Trees.end(); ++tree) {
            for (const TLinearModel<TFloatType>* linearModel = tree->LinearModels.begin(); linearModel != tree->LinearModels.end(); ++linearModel) {
                LinearModels.push_back(linearModel->Intercept);
                LinearModels.insert(LinearModels.end(), linearModel->Coefficients.begin(), linearModel->Coefficients.end());
            }

            {
                const TVector<TStump<TFloatType> >& stumps = tree->Stumps;
                const TVector<TTreeNode>& nodes = tree->Nodes;

                for (size_t stumpNumber = 0; stumpNumber < tree->Stumps.size(); ++stumpNumber) {
                    const TStump<TFloatType>& stump = stumps[stumpNumber];
                    const TTreeNode& node = nodes[stumpNumber];

                    TCompactStumpInfo& compactStump = Stumps[stumpsOffset + stumpNumber];

                    compactStump.FeatureNumber = stump.FeatureNumber;
                    compactStump.FeatureThreshold = stump.FeatureThreshold;
                    compactStump.LeftChild = node.Left == node.Right
                        ? nullptr
                        : &Stumps[stumpsOffset + node.Left];
                    compactStump.LinearModel = node.Left == node.Right
                        ? &LinearModels[(FeaturesCount + 1) * (node.Left + linearModelsOffset)]
                        : nullptr;
                }
            }

            Roots.push_back(&Stumps[stumpsOffset]);

            linearModelsOffset += tree->LinearModels.size();
            stumpsOffset += tree->Stumps.size();
        }
    }

    template <typename T>
    double NonTransformedPrediction(const T* features) const {
        if (FeaturesTransformer.IsFake()) {
            return SimplePrediction(features);
        }

        TVector<T> featuresCopy(features, features + FeaturesTransformer.Size());
        T* featureCopy = featuresCopy.begin();
        for (; featureCopy != featuresCopy.end(); ++featureCopy, ++features) {
            *featureCopy = *features;
        }
        FeaturesTransformer.TransformFeatures(featuresCopy);

        return SimplePrediction(featuresCopy.begin());
    }

    template <typename T>
    double NonTransformedPrediction(const TVector<T>& features) const {
        return NonTransformedPrediction(features.begin());
    }

    template <typename T>
    double Prediction(const T* features) const {
        return LossFunction->TransformPrediction(NonTransformedPrediction(features));
    }

    template <typename T>
    double Prediction(const TVector<T>& features) const {
        return Prediction(features.begin());
    }

    void Cut(size_t treesToUseCount) {
        if (Roots.size() > treesToUseCount) {
            Roots.erase(Roots.begin() + treesToUseCount, Roots.end());
        }
    }

    const TString& GetPositiveMark() const {
        return PositiveMark;
    }
private:
    template <typename T>
    double SimplePrediction(const T* features) const {
        double prediction = Bias;

        for (size_t i = 0; i < Roots.size(); ++i) {
            const TCompactStumpInfo* stump = Roots[i];
            while (stump->LeftChild) {
                stump = stump->LeftChild + (features[stump->FeatureNumber] > stump->FeatureThreshold);
            }
            const float* begin = stump->LinearModel;
            const float* end = begin + FeaturesCount + 1;

            prediction += *begin + std::inner_product(begin + 1, end, features, 0.);
        }

        return prediction;
    }
};

typedef TCompactBoosting TCompactRegressionModel;

class TTCompactRegressionModelLoader : public NMatrixnet::IRelevCalcerLoader {
public:
    NMatrixnet::IRelevCalcer* Load(IInputStream *in) const override {
        TRegressionModel model;
        model.Load(in);
        return new TCompactRegressionModel(model);
    }
};

class TPredictor : public NMatrixnet::IRelevCalcer {
private:
    TSimpleSharedPtr<TLossFunctionBase<float> > LossFunction;

    TSimpleSharedPtr<TCompactBoosting> RegTreeModel;
    TSimpleSharedPtr<NMatrixnet::TMnSseDynamic> MatrixnetModel;

    enum EModelType {
        RegTree,
        Matrixnet,
    };

    EModelType ModelType;
public:
    double DoCalcRelev(const float* features) const override {
        return NonTransformedPrediction(features);
    }

    void DoCalcRelevs(const float* const* docs_features, double* result_relev, const size_t num_docs) const override {
        if (MatrixnetModel) {
            MatrixnetModel->DoCalcRelevs(docs_features, result_relev, num_docs);
        }
        if (RegTreeModel) {
            NMatrixnet::IRelevCalcer::DoCalcRelevs(docs_features, result_relev, num_docs);
        }
    }

    size_t GetNumFeats() const override {
        return ModelType == RegTree ? RegTreeModel->GetNumFeats() : MatrixnetModel->GetNumFeats();
    }

    TPredictor() {
    }

    TPredictor(const TString& modelFileName, const TOptions& options = TOptions()) {
        SetModelType(modelFileName);

        if (ModelType == RegTree) {
            RegTreeModel.Reset(new TCompactBoosting(modelFileName));
        } else {
            auto data = TBlob::FromFileContent(modelFileName);
            TMemoryInput modelIn(data.AsCharPtr(), data.Size());
            MatrixnetModel.Reset(new NMatrixnet::TMnSseDynamic());
            MatrixnetModel->Load(&modelIn);
        }

        LossFunction = LossFunctionByName<float>(options.LossFunctionName).Release();
    }

    TPredictor(const TRegressionModel& regressionModel) {
        LossFunction = LossFunctionByName<float>(regressionModel.LossFunction->Name()).Release();

        RegTreeModel.Reset(new TCompactBoosting(regressionModel));
        MatrixnetModel.Reset(nullptr);

        ModelType = RegTree;
    }

    TPredictor(const NMatrixnet::TMnSseDynamic& mxNetModel, const TOptions& options = TOptions()) {
        LossFunction = LossFunctionByName<float>(options.LossFunctionName).Release();

        MatrixnetModel.Reset(new NMatrixnet::TMnSseDynamic());
        MatrixnetModel->CopyFrom(mxNetModel);

        RegTreeModel.Reset(nullptr);

        ModelType = Matrixnet;
    }

    double NonTransformedPrediction(const float* features) const {
        return ModelType == RegTree ? RegTreeModel->NonTransformedPrediction(features) : MatrixnetModel->DoCalcRelev(features);
    }

    double NonTransformedPrediction(const TVector<float>& features) const {
        return NonTransformedPrediction(features.data());
    }

    double Prediction(const TVector<float>& features) const {
        if (ModelType == RegTree) {
            return RegTreeModel->Prediction(features);
        }
        return LossFunction->TransformPrediction(MatrixnetModel->CalcRelev(features));
    }
private:
    void SetModelType(const TString& filename) {
        TUnbufferedFileInput in(filename);
        char buf[30];
        size_t filled = in.Load(buf, 30);
        TStringBuf start(buf, filled);
        if (start.find("REGTREE") != TStringBuf::npos) {
            ModelType = RegTree;
        } else {
            ModelType = Matrixnet;
        }
    }
};

static inline double GetBestIntentWeight(const NMatrixnet::IRelevCalcer* model,
                                         const float* features,
                                         const float step,
                                         const size_t stepsCount,
                                         IIntentWeightChooser& iwChooser,
                                         const float startIntentWeight = 0.f,
                                         bool deflateIntentWeight = false)
{
    if (!model) {
        return startIntentWeight;
    }

    TVector<float> baseFeatures;
    baseFeatures.push_back(0);
    baseFeatures.insert(baseFeatures.end(), features, features + model->GetNumFeats() - 1);

    TVector<TVector<float> > featuresWithIW(stepsCount, baseFeatures);
    for (size_t stepNumber = 0; stepNumber < stepsCount; ++stepNumber) {
        featuresWithIW[stepNumber][0] = startIntentWeight + step * stepNumber;
    }

    TVector<double> predictions;
    model->CalcRelevs(featuresWithIW, predictions);

    for (size_t stepNumber = 0; stepNumber < stepsCount; ++stepNumber) {
        const float iw = startIntentWeight + step * stepNumber;
        double prediction = predictions[stepNumber];

        iwChooser.Add(iw, prediction);
    }

    TMaybe<double> bestIW = iwChooser.GetBestIW();
    return bestIW ? *bestIW : deflateIntentWeight ? -10 : startIntentWeight;
}

static inline double GetBestIntentWeight(const NMatrixnet::IRelevCalcer* model,
                                         const float* features,
                                         const float step,
                                         const size_t stepsCount,
                                         const double threshold,
                                         const TStringBuf chooserType,
                                         const float startIntentWeight = 0.f,
                                         bool deflateIntentWeight = false)
{
    THolder<IIntentWeightChooser> iwChooser = CreateIntentWeightChooser(chooserType, threshold, startIntentWeight);
    return GetBestIntentWeight(model, features, step, stepsCount, *iwChooser, startIntentWeight, deflateIntentWeight);
}

static inline double GetBestIntentWeight(const NMatrixnet::IRelevCalcer* model,
                                         const float* features,
                                         const NSc::TValue& cfg)
{
    const double step = cfg["step"].GetNumber();
    const size_t stepsCount = cfg["steps_count"].GetIntNumber();
    const double threshold = cfg["threshold"].GetNumber();
    const TStringBuf type = cfg["clickregtree_type"].GetString();
    const double startIntentWeight = cfg["start_intent_weight"].GetNumber();
    const bool deflateIntentWeight = cfg["deflate_intent_weight"].GetBool();

    return GetBestIntentWeight(model, features, step, stepsCount, threshold, type, startIntentWeight, deflateIntentWeight);
}

}
