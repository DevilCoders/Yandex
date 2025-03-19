#include <kernel/ethos/lib/reg_tree/compositions.h>
#include <kernel/ethos/lib/reg_tree/cross_validation.h>
#include <kernel/ethos/lib/reg_tree/least_squares_tree.h>
#include <kernel/ethos/lib/reg_tree/linear_regression.h>
#include <kernel/ethos/lib/reg_tree/pool.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/reverse.h>

TVector<NRegTree::TPoolFileNameInfo> learnPoolFileNameInfos;
TVector<std::pair<TString, double> > predictorFileNameWithWeights;

double GetPrediction(const NRegTree::TInstance<float>& instance,
                     const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights)
{
    double prediction = 0.;
    for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
        prediction += predictorWithWeight.first.Prediction(instance.Features) * predictorWithWeight.second;
    }
    return prediction;
}

double GetNonTransformedPrediction(const NRegTree::TInstance<float>& instance,
                                   const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights)
{
    double prediction = 0.;
    for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
        prediction += predictorWithWeight.first.NonTransformedPrediction(instance.Features) * predictorWithWeight.second;
    }
    return prediction;
}

void AddFeaturesFileName(const NLastGetopt::TOptsParser* p) {
    TStringBuf featureFileNamesStrBuf(p->CurVal());
    TVector<TStringBuf> split;
    StringSplitter(featureFileNamesStrBuf.begin(), featureFileNamesStrBuf.end()).Split(',').AddTo(&split);

    for (TStringBuf poolInfo : split) {
        TStringBuf fileNameStrBuf;
        TStringBuf weightStrBuf;
        poolInfo.Split('@', fileNameStrBuf, weightStrBuf);

        NRegTree::TPoolFileNameInfo poolFileNameInfo;
        poolFileNameInfo.FeaturesFileName = ToString(fileNameStrBuf);
        poolFileNameInfo.PoolWeight = !!weightStrBuf ? FromString<float>(weightStrBuf) : 1.f;

        learnPoolFileNameInfos.push_back(poolFileNameInfo);
    }
}

void AddPredictor(const NLastGetopt::TOptsParser* p) {
    TStringBuf predictorsBuf(p->CurVal());
    TVector<TStringBuf> split;
    StringSplitter(predictorsBuf.begin(), predictorsBuf.end()).Split(',').AddTo(&split);

    for (TStringBuf predictorInfo : split) {
        TStringBuf predictorStrBuf;
        TStringBuf weightStrBuf;
        predictorInfo.Split('@', predictorStrBuf, weightStrBuf);

        TString predictorFileName = ToString(predictorStrBuf);
        double weight = !!weightStrBuf ? FromString<double>(weightStrBuf) : 1.;

        predictorFileNameWithWeights.push_back(std::make_pair(predictorFileName, weight));
    }
}

void SetupRandomIWPredictions(NRegTree::TPool<float>& learnPool,
                              NRegTree::TPool<float>& testPool,
                              const NRegTree::TOptions& options,
                              const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights)
{
    auto processFeatures = [&predictorWithWeights, &options](TVector<NRegTree::TInstance<float> >& instances) {
        THolder<IThreadPool> queue = CreateThreadPool(options.ThreadsCount);
        for (size_t threadNumber = 0; threadNumber < options.ThreadsCount; ++threadNumber) {
            queue->SafeAddFunc([&, threadNumber]() {
                for (size_t instanceNumber = threadNumber; instanceNumber < instances.size(); instanceNumber += options.ThreadsCount) {
                    NRegTree::TInstance<float>& instance = instances[instanceNumber];
                    instance.Prediction = GetNonTransformedPrediction(instance, predictorWithWeights);

                    if (options.LossFunctionName.find("SURPLUS") != TString::npos || options.LossFunctionName == "QUERY_REGULARIZED_LOGISTIC") {
                        TVector<TVector<float> > differentFeatures(instance.RandomWeights.size(), instance.Features);
                        for (size_t rwNumber = 0; rwNumber < instance.RandomWeights.size(); ++rwNumber) {
                            differentFeatures[rwNumber][0] = instance.RandomWeights[rwNumber];
                        }

                        TVector<double> predictions;
                        for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
                            predictorWithWeight.first.CalcRelevs(differentFeatures, predictions);

                            for (size_t i = 0; i < instance.RandomWeights.size(); ++i) {
                                instance.PredictionsForRandomWeights[i] += predictions[i] * predictorWithWeight.second;
                            }
                        }
                    }
                }
            });
        }
    };

    processFeatures(learnPool.GetInstances());
    processFeatures(testPool.GetInstances());
}

TVector<TVector<NRegTree::TMetricWithValues> > Test(NRegTree::TPool<float>& learnPool,
                                                    NRegTree::TPool<float>& testPool,
                                                    const NRegTree::TOptions& options,
                                                    const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights)
{
    SetupRandomIWPredictions(learnPool, testPool, options, predictorWithWeights);

    THolder<NRegTree::TLossFunctionBase<float> > lossFunction(NRegTree::LossFunctionByName<float>(options.LossFunctionName));

    TVector<TVector<NRegTree::TMetricWithValues> > metrics;
    for (size_t poolNumber = 0; poolNumber < learnPool.GetPoolsCount(); ++poolNumber) {
        metrics.push_back(lossFunction->GetMetrics(learnPool, options, poolNumber));
    }

    TVector<NRegTree::TMetricWithValues> testMetrics = lossFunction->GetMetrics(testPool, options);
    for (size_t i = 0; i < testMetrics.size(); ++i) {
        metrics[0][i].TestValue = testMetrics[i].LearnValue;
    }

    learnPool.Restore();
    testPool.Restore();

    return metrics;
}

void ApplySurplusModelDetailed(IInputStream& featuresIn,
                               IOutputStream& featuresOut,
                               const NRegTree::TPredictor& predictor,
                               const NRegTree::TOptions& options)
{
    TString featuresStr;
    while (featuresIn.ReadLine(featuresStr)) {
        NRegTree::TInstance<float> instance(featuresStr, options);

        TStringBuf featuresStrBuf(featuresStr);

        TString queryId = ToString(featuresStrBuf.NextTok('\t'));
        TString goal = ToString(featuresStrBuf.NextTok('\t'));
        TString url = ToString(featuresStrBuf.NextTok('\t'));
        TString weight = ToString(featuresStrBuf.NextTok('\t'));

        TVector<TVector<float> > allFeatures(options.IntentWeightStepsCount, instance.Features);
        for (size_t step = 0; step < options.IntentWeightStepsCount; ++step) {
            allFeatures[step].front() = options.StartIntentWeight + step * options.IntentWeightStep;
        }

        TVector<double> predictions;
        predictor.CalcRelevs(allFeatures, predictions);

        for (size_t step = 0; step < options.IntentWeightStepsCount; ++step) {
            featuresOut << queryId << "\t"
                        << goal << "\t"
                        << url << "\t"
                        << weight << "\t"
                        << instance.Features.front() << "\t"
                        << allFeatures[step].front() << "\t"
                        << predictions[step] << "\n";
        }
    }
}

void ApplyRegressionModel(IInputStream& featuresIn,
                          IOutputStream& featuresOut,
                          const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights,
                          const NRegTree::TOptions& options,
                          const bool appendFeature)
{
    if (options.DetailedSurplus) {
        if (predictorWithWeights.size() != 1) {
            ythrow yexception() << "exactly one predictor is needed for this mode";
        }

        ApplySurplusModelDetailed(featuresIn, featuresOut, predictorWithWeights.front().first, options);
        return;
    }

    THashMap<TString, size_t> queryMatchings;
    TString featuresStr;
    while (featuresIn.ReadLine(featuresStr)) {
        NRegTree::TInstance<float> instance(featuresStr, options, &queryMatchings);

        TStringBuf featuresStrBuf(featuresStr);
        TString queryId = ToString(featuresStrBuf.NextTok('\t'));
        TString goal = ToString(featuresStrBuf.NextTok('\t'));
        TString url = ToString(featuresStrBuf.NextTok('\t'));
        TString weight = ToString(featuresStrBuf.NextTok('\t'));

        double prediction = 0.;
        if (options.LossFunctionName.find("SURPLUS") != TString::npos ||
            options.LossFunctionName.find("QUERY_REGULARIZED_LOGISTIC") != TString::npos)
        {
            double sumWeights = 0.;
            TVector<float> featuresWithoutIW(instance.Features.begin() + 1, instance.Features.end());
            for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
                prediction += NRegTree::GetBestIntentWeight(&predictorWithWeight.first,
                                                            featuresWithoutIW.data(),
                                                            options.IntentWeightStep,
                                                            options.IntentWeightStepsCount,
                                                            options.SurplusThreshold,
                                                            options.SurplusIWChooserType,
                                                            options.StartIntentWeight,
                                                            options.DeflateIntentWeight) * predictorWithWeight.second;
                sumWeights += predictorWithWeight.second;
            }
            prediction = prediction ? prediction / sumWeights : prediction;
        } else {
            for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
                prediction += predictorWithWeight.first.Prediction(instance.Features) * predictorWithWeight.second;
            }
        }

        if (options.ExportOnlyPrefictions) {
            featuresOut << prediction << "\n";
        } else {
            featuresOut << queryId << "\t"
                        << goal << "\t"
                        << url << "\t"
                        << weight;
            if (options.LossFunctionName.find("SURPLUS") != TString::npos) {
                featuresOut << "\t" << instance.Features.front();
            }
            featuresOut << "\t" << prediction;

            if (appendFeature) {
                featuresOut << "\t" << featuresStrBuf;
            }
            featuresOut << "\n";
        }
    }
}


void PrintResidualsPool(IInputStream& featuresIn,
                        IOutputStream& featuresOut,
                        const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights,
                        const NRegTree::TOptions& options)
{
    TString featuresStr;
    while (featuresIn.ReadLine(featuresStr)) {
        NRegTree::TInstance<float> instance(featuresStr, options);

        TStringBuf featuresStrBuf(featuresStr);
        TString queryId = ToString(featuresStrBuf.NextTok('\t'));
        const double goal = FromString(featuresStrBuf.NextTok('\t'));
        TString url = ToString(featuresStrBuf.NextTok('\t'));
        TString weight = ToString(featuresStrBuf.NextTok('\t'));

        double prediction = 0.;
        for (const std::pair<NRegTree::TPredictor, double>& predictorWithWeight : predictorWithWeights) {
            prediction += predictorWithWeight.first.Prediction(instance.Features) * predictorWithWeight.second;
        }

        featuresOut << queryId << "\t"
                    << (goal - prediction) << "\t"
                    << url << "\t"
                    << weight << "\t"
                    << prediction << "\t"
                    << featuresStrBuf << "\n";
    }
}

void PrintResidualsPool(const TVector<NRegTree::TPoolFileNameInfo>& learnPoolFileNameInfosLocal,
                        const TString& testFeaturesFileName,
                        const TString& suffix,
                        const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights,
                        const NRegTree::TOptions& options)
{
    if (!suffix) {
        for (const NRegTree::TPoolFileNameInfo& learnPoolFilNameInfo : learnPoolFileNameInfosLocal) {
            TFileInput learnFeaturesIn(learnPoolFilNameInfo.FeaturesFileName);
            PrintResidualsPool(learnFeaturesIn, Cout, predictorWithWeights, options);
        }
        if (!!testFeaturesFileName) {
            TFileInput testFeaturesIn(testFeaturesFileName);
            PrintResidualsPool(testFeaturesIn, Cout, predictorWithWeights, options);
        }
        return;
    }

    for (const NRegTree::TPoolFileNameInfo& learnPoolFilNameInfo : learnPoolFileNameInfosLocal) {
        TFileInput learnFeaturesIn(learnPoolFilNameInfo.FeaturesFileName);
        TFixedBufferFileOutput learnFeaturesOut(learnPoolFilNameInfo.FeaturesFileName + "." + suffix);
        PrintResidualsPool(learnFeaturesIn, learnFeaturesOut, predictorWithWeights, options);
    }
    if (!!testFeaturesFileName) {
        TFileInput testFeaturesIn(testFeaturesFileName);
        TFixedBufferFileOutput testFeaturesOut(testFeaturesFileName + "." + suffix);
        PrintResidualsPool(testFeaturesIn, testFeaturesOut, predictorWithWeights, options);
    }
}

void ApplyRegressionModel(const TVector<NRegTree::TPoolFileNameInfo>& learnPoolFileNameInfosLocal,
                          const TString& testFeaturesFileName,
                          const TString& suffix,
                          const TVector<std::pair<NRegTree::TPredictor, double> >& predictorWithWeights,
                          const NRegTree::TOptions& options,
                          const bool filter)
{
    if (!suffix) {
        for (const NRegTree::TPoolFileNameInfo& learnPoolFilNameInfo : learnPoolFileNameInfosLocal) {
            TFileInput learnFeaturesIn(learnPoolFilNameInfo.FeaturesFileName);
            ApplyRegressionModel(learnFeaturesIn, Cout, predictorWithWeights, options, filter);
        }
        if (!!testFeaturesFileName) {
            TFileInput testFeaturesIn(testFeaturesFileName);
            ApplyRegressionModel(testFeaturesIn, Cout, predictorWithWeights, options, filter);
        }
        return;
    }

    for (const NRegTree::TPoolFileNameInfo& learnPoolFilNameInfo : learnPoolFileNameInfosLocal) {
        TFileInput learnFeaturesIn(learnPoolFilNameInfo.FeaturesFileName);
        TFixedBufferFileOutput learnFeaturesOut(learnPoolFilNameInfo.FeaturesFileName + "." + suffix);
        ApplyRegressionModel(learnFeaturesIn, learnFeaturesOut, predictorWithWeights, options, filter);
    }
    if (!!testFeaturesFileName) {
        TFileInput testFeaturesIn(testFeaturesFileName);
        TFixedBufferFileOutput testFeaturesOut(testFeaturesFileName + "." + suffix);
        ApplyRegressionModel(testFeaturesIn, testFeaturesOut, predictorWithWeights, options, filter);
    }
}

TVector<std::pair<NRegTree::TPredictor, double> > ReadPredictors(const NRegTree::TOptions& options) {
    TVector<std::pair<NRegTree::TPredictor, double> > predictors;
    for (const std::pair<TString, double>& predictorFileNameWithWeight : predictorFileNameWithWeights) {
        NRegTree::TPredictor predictor(predictorFileNameWithWeight.first, options);
        predictors.push_back(std::make_pair(predictor, predictorFileNameWithWeight.second));
    }
    return predictors;
}

void CompactifyModel(const TString& modelPath) {
    NRegTree::TCompactBoosting compactBoosting;
    compactBoosting.Build(modelPath);

    const NRegTree::TCompactModel compactModel = compactBoosting.ToCompactModel();
    {
        TFixedBufferFileOutput modelOut(modelPath);
        compactModel.Save(&modelOut);
    }
}

void CalculateFeatureStrengths(const NRegTree::TRegressionModel::TPoolType& pool, const NRegTree::TOptions& options) {
    THolder<IThreadPool> queue = CreateThreadPool(options.ThreadsCount);

    TVector<std::pair<double, size_t> > featureStrengths(pool.GetFeaturesCount());
    for (size_t threadNumber = 0; threadNumber < options.ThreadsCount; ++threadNumber) {
        queue->SafeAddFunc([&, threadNumber](){
            for (size_t featureNumber = threadNumber;
                 featureNumber < pool.GetFeaturesCount();
                 featureNumber += options.ThreadsCount)
            {
                NRegTree::TSimpleLinearRegressionSplitterBase<double> slrSolver;
                for (const NRegTree::TRegressionModel::TInstanceType& instance : pool) {
                    slrSolver.Add(instance.Goal, instance.Weight, instance.Features[featureNumber]);
                }
                featureStrengths[featureNumber].first = slrSolver.DeterminationCoefficient();
                featureStrengths[featureNumber].second = featureNumber;
            }
        });
    }
    Delete(std::move(queue));

    Sort(featureStrengths.begin(), featureStrengths.end(), TGreater<>());

    for (const std::pair<double, size_t>& featureWithStrength : featureStrengths) {
        Cout << featureWithStrength.second << "\t" << featureWithStrength.first << "\t" << featureWithStrength.first << "\t" << featureWithStrength.first << "\n";
    }
}

const double ConvertFactor = 1e-6;

const TString Encode(const double score) {
    const ui64 encodedValue = Max<ui64>() - (ui64) (score / ConvertFactor);

    TString encodedStr = ToString(encodedValue);
    ReverseInPlace(encodedStr);
    while (encodedStr.length() < 30) {
        encodedStr.append('0');
    }
    ReverseInPlace(encodedStr);

    return encodedStr;
}

double Decode(const TString& encodedScore) {
    return (Max<ui64>() - FromString<ui64>(encodedScore)) * ConvertFactor;
}


int main(int argc, const char** argv) {
    TString runMode = "learn";

    TString modelFileName;

    NRegTree::TOptions options;
    TString testFeaturesFileName;

    TString suffix;

    // select features stuff
    TVector<size_t> featuresForEval;
    TVector<size_t> deletedFeatures;

    TString secondModelFileName;

    try {
        NLastGetopt::TOpts opts;

        opts.AddCharOption('f', "features filename")
            .Handler(&AddFeaturesFileName);

        opts.AddCharOption('t', "test features filename")
            .StoreResult(&testFeaturesFileName);

        opts.AddLongOption("learn", "learn mode, is default")
            .NoArgument().StoreValue(&runMode, "learn");
        opts.AddLongOption("suffix", "apply learned model to learn and test features and use this suffix")
            .StoreResult(&suffix);

        opts.AddLongOption("test", "test mode")
            .NoArgument().StoreValue(&runMode, "test");
        opts.AddLongOption("cv", "cross-validation mode")
            .NoArgument().StoreValue(&runMode, "cv");
        opts.AddLongOption("qv", "cross-validation mode")
            .NoArgument().StoreValue(&runMode, "cv").StoreValue(&options.UseQueries, true);
        opts.AddLongOption("apply", "model applying mode")
            .NoArgument().StoreValue(&runMode, "apply");
        opts.AddLongOption("append-feature", "append prediction as a first feature mode")
            .NoArgument().StoreValue(&runMode, "append-feature");
        opts.AddLongOption("test-new-surplus", "test new surplus mode")
            .StoreValue(&runMode, "test-new-surplus").StoreResult(&secondModelFileName).StoreValue(&options.LossFunctionName, "SURPLUS");
        opts.AddLongOption("filter", "filter pool mode")
            .NoArgument().StoreValue(&runMode, "filter");
        opts.AddLongOption("print", "print model in human-readable format")
            .NoArgument().StoreValue(&runMode, "print");
        opts.AddLongOption("print-python", "print python code for model")
            .NoArgument().StoreValue(&runMode, "print-python");
        opts.AddLongOption("print-cpp", "print cpp code for model")
            .NoArgument().StoreValue(&runMode, "print-cpp");
        opts.AddLongOption("select-features", "select features mode")
            .NoArgument().StoreValue(&runMode, "select-features");
        opts.AddLongOption("residuals", "print residuals mode")
            .NoArgument().StoreValue(&runMode, "residuals");
        opts.AddLongOption("iw-pool", "dump intent weights pool (secret mode for alex-sh@ only)")
            .NoArgument().StoreValue(&runMode, "iw-pool");
        opts.AddLongOption("fstr", "calculate simple linear regression feature strengths mode")
            .NoArgument().StoreValue(&runMode, "fstr");
        opts.AddLongOption("queries", "use queries")
            .NoArgument().StoreValue(&options.UseQueries, true);
        opts.AddLongOption("compactify", "compactify model mode")
            .NoArgument().StoreValue(&runMode, "compactify");

        opts.AddLongOption('m', "model", "model filename")
            .StoreResult(&modelFileName)
            .Handler(&AddPredictor);

        opts.AddLongOption("eval-features", "features for eval set for select-features mode")
            .Optional()
            .RangeSplitHandler(&featuresForEval, ',', '-');
        opts.AddLongOption("deleted-features", "features to be deleted from pool for select-features mode")
            .Optional()
            .RangeSplitHandler(&deletedFeatures, ',', '-');

        options.AddOpts(opts);

        opts.SetFreeArgsMax(0);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    } catch (yexception& ex) {
        Cerr << "Error parsing command line options: " << ex.what() << "\n";
        return 1;
    }

    if (runMode == "learn") {
        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        NRegTree::TRegressionModel::TPoolType learnPool;
        learnPool.AddInstances(learnPoolFileNameInfos, options);

        NRegTree::TRegressionModel::TPoolType testPool;
        testPool.AddInstances(testFeaturesFileName, options);

        NRegTree::TRegressionModel regressionModel;
        const TVector<TVector<NRegTree::TMetricWithValues>> metrics = regressionModel.LearnMutable(learnPool, BuildFeatureIterators(learnPool, options), options, modelFileName, THashSet<size_t>(), &testPool);

        if (!!suffix) {
            TVector<std::pair<NRegTree::TPredictor, double> > fakePredictorsVector(1, std::make_pair(NRegTree::TPredictor(regressionModel), 1.));
            ApplyRegressionModel(learnPoolFileNameInfos, testFeaturesFileName, suffix, fakePredictorsVector, options, false);
        }

        if (options.JsonMetricsOutPath) {
            TFixedBufferFileOutput jsonMetricsOut(options.JsonMetricsOutPath);
            PrintMetricsJson(jsonMetricsOut, metrics, learnPoolFileNameInfos, testFeaturesFileName);
        }
    } else if (runMode == "cv") {
        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        if ((options.LossFunctionName == "PAIRWISE" || options.LossFunctionName == "RANK") && !options.UseQueries) {
            Cerr << "cross-validation is not supported for rank/pairwise loss function, use --qv!\n";
            return 1;
        }

        NRegTree::TRegressionModel::TPoolType pool;
        pool.AddInstances(learnPoolFileNameInfos, options);

        options.UseQueries ? QrossValidation(pool, BuildFeatureIterators(pool, options), options, learnPoolFileNameInfos.size() > 1)
            : CrossValidation(pool, BuildFeatureIterators(pool, options), options, learnPoolFileNameInfos.size() > 1);
    } else if (runMode == "compactify") {
        CompactifyModel(modelFileName);
    } else if (runMode == "select-features") {
        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        options.OutputDetailedTreeInfo = false;

        NRegTree::TRegressionModel::TPoolType pool;
        pool.AddInstances(learnPoolFileNameInfos, options);

        SelectFeatures(pool, BuildFeatureIterators(pool, options), options, featuresForEval, deletedFeatures);
    }  else if (runMode == "iw-pool") {
        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        NRegTree::TPool<float> learnPool, testPool;
        learnPool.AddInstances(learnPoolFileNameInfos, options);

        SetupRandomIWPredictions(learnPool, testPool, options, ReadPredictors(options));

        for (NRegTree::TInstance<float>& instance : learnPool) {
            instance.Features.erase(instance.Features.begin() + 1, instance.Features.end());
            instance.Features.insert(instance.Features.begin() + 1,
                                     instance.PredictionsForRandomWeights.begin(),
                                     instance.PredictionsForRandomWeights.end());
            Cout << instance.ToFeaturesString() << "\n";
        }
    } else if (runMode == "test") {
        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        if (learnPoolFileNameInfos.empty()) {
            Cerr << "choose features file name (-f <featuresFileName>)!\n";
            return 1;
        }

        NRegTree::TPool<float> learnPool;
        learnPool.AddInstances(learnPoolFileNameInfos, options);

        NRegTree::TPool<float> testPool;
        testPool.AddInstances(testFeaturesFileName, options);

        TVector<TVector<NRegTree::TMetricWithValues> > metrics = Test(learnPool, testPool, options, ReadPredictors(options));

        NRegTree::PrintMetrics(Cout, metrics, learnPoolFileNameInfos, testFeaturesFileName, options);
    } else if (runMode == "apply") {
        if (learnPoolFileNameInfos.empty() && !testFeaturesFileName) {
            Cerr << "choose features file name (-f <featuresFileName> or -t <featuresFileName>)!\n";
            return 1;
        }

        ApplyRegressionModel(learnPoolFileNameInfos, testFeaturesFileName, suffix, ReadPredictors(options), options, false);
    } else if (runMode == "append-feature") {
        if (learnPoolFileNameInfos.empty() && !testFeaturesFileName) {
            Cerr << "choose features file name (-f <featuresFileName> or -t <featuresFileName>)!\n";
            return 1;
        }

        ApplyRegressionModel(learnPoolFileNameInfos, testFeaturesFileName, suffix, ReadPredictors(options), options, true);
    } else if (runMode == "test-new-surplus") {
        NRegTree::TPool<float> pool;
        pool.AddInstances(learnPoolFileNameInfos, options);

        NRegTree::TPredictor predictor(modelFileName);
        NRegTree::TPredictor predictor2(secondModelFileName);

        NRegTree::TSurplusMetricsCalculator<float> surplusMetricsCalculator(options);
        for (NRegTree::TInstance<float>& instance : pool) {
            TVector<TVector<float> > allFeatures(options.IntentWeightStepsCount, instance.Features);
            for (size_t step = 0; step < options.IntentWeightStepsCount; ++step) {
                allFeatures[step][0] = options.StartIntentWeight + step * options.IntentWeightStep;
                const double prediction = predictor.Prediction(allFeatures[step]);
                allFeatures[step].insert(allFeatures[step].begin(), prediction);
            }

            TVector<double> secondPredictions;
            predictor2.CalcRelevs(allFeatures, secondPredictions);

            for (size_t i = 0; i < secondPredictions.size(); ++i) {
                instance.PredictionsForRandomWeights[i] = secondPredictions[i];
            }

            surplusMetricsCalculator.Add(instance);
        }

        Cout << "    Surplus   " << surplusMetricsCalculator.Surplus() << "\n";
        Cout << "    WinsPart  " << surplusMetricsCalculator.WinsFraction() << "\n";
        Cout << "    Wins      " << surplusMetricsCalculator.Wins() << "\n";
        Cout << "    Losses    " << surplusMetricsCalculator.Losses() << "\n";
    } else if (runMode == "residuals") {
        if (learnPoolFileNameInfos.empty() && !testFeaturesFileName) {
            Cerr << "choose features file name (-f <featuresFileName> or -t <featuresFileName>)!\n";
            return 1;
        }

        if (predictorFileNameWithWeights.empty()) {
            Cerr << "choose model(s) to apply (--model)!\n";
            return 1;
        }

        PrintResidualsPool(learnPoolFileNameInfos, testFeaturesFileName, suffix, ReadPredictors(options), options);
    } else if (runMode == "print") {
        if (predictorFileNameWithWeights.size() != 1) {
            Cerr << "choose exactly one model to print (--model)!\n";
            return 1;
        }

        NRegTree::TRegressionModel regressionModel;
        {
            TFileInput modelIn(predictorFileNameWithWeights[0].first);
            regressionModel.Load(&modelIn);
        }

        regressionModel.Print(Cout);
    } else if (runMode == "fstr") {
        NRegTree::TRegressionModel::TPoolType pool;
        pool.AddInstances(learnPoolFileNameInfos, options);
        CalculateFeatureStrengths(pool, options);
    } else if (runMode == "print-python") {
        NRegTree::TCompactBoosting compactModel(predictorFileNameWithWeights[0].first);
        Cout << compactModel.ToPythonCode(suffix) << "\n";
    } else if (runMode == "print-cpp") {
        NRegTree::TCompactBoosting compactModel(predictorFileNameWithWeights[0].first);
        Cout << compactModel.ToCppCode() << "\n";
    }

    return 0;
}
