#include "compress.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <util/generic/xrange.h>
#include <util/generic/ymath.h>
#include <util/string/vector.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>


namespace NNeuralNetApplier {

    TVector<float> CalcQuantiles(const double gridStep) {
        constexpr double quantilesUpperBound = 0.2;
        constexpr double startQuantileValue = 1e-9;

        TVector<float> res = { 0 };
        double quantile = startQuantileValue;
        while (quantile < quantilesUpperBound) {
            res.push_back(quantile);
            quantile *= gridStep;
        }
        res.push_back(quantilesUpperBound);
        return res;
    }

    template <class TVectorOfVectors>
    double GetMse(const TVectorOfVectors& trueValues, const TVectorOfVectors& predictValues) {
        double sum = 0;
        double count = 0;
        Y_ENSURE(trueValues.size() == predictValues.size());
        for (auto i : xrange(trueValues.size())) {
            Y_ENSURE(trueValues[i].size() == predictValues[i].size());
            for (auto j : xrange(trueValues[i].size())) {
                sum += Sqr(trueValues[i][j] - predictValues[i][j]);
            }
            count += trueValues[i].size();
        }
        return sum / count;
    }

    THashMap<TString, size_t> GetVariablesDependencies(const TModel& model, const TVector<TString>& variables) {
        THashMap<TString, size_t> res;
        for (size_t layerIdx : xrange(model.Layers.size())) {
            const ILayerPtr& layer = model.Layers[layerIdx];
            for (const TString& output : layer->GetOutputs()) {
                for (TString variable : variables) {
                    variable = SplitString(variable, ":[")[0];
                    if (variable == output) {
                        res.emplace(variable, layerIdx);
                    }
                }
            }
        }
        return res;
    }

    TModelPtr GetCompressedModel(const TModel& model, const THashMap<TString, size_t>& dependentVariables2LayerIdx,
        const TString& variable, const float lowerBound, const float upperBound, const size_t numBits, const size_t numBins)
    {
        TStringStream stringStream;
        model.Save(&stringStream);
        TBlob blob = TBlob::FromStream(stringStream);
        TModelPtr compressedModel = new TModel();
        compressedModel->Load(blob);
        TVector<ILayerPtr> compressors;
        compressors.push_back(new TCompressorLayer(variable + "$before_compress",
            variable + "$compressed", numBins, lowerBound, upperBound, numBits));
        compressors.push_back(new TDecompressorLayer(
            variable + "$compressed", variable, numBins, lowerBound, upperBound, numBits));
        size_t srcLayerIdx = dependentVariables2LayerIdx.at(variable);
        compressedModel->Layers[srcLayerIdx]->RenameVariable(variable, variable + "$before_compress");
        compressedModel->Layers.insert(compressedModel->Layers.begin() + srcLayerIdx + 1,
            compressors.begin(), compressors.end());
        compressedModel->Init();
        return compressedModel;
    }

    TModelPtr GetCompressedModel(const TModel& model, const TString& variable, const float quantile, float varPercentsToSave) {
        TStringStream stringStream;
        model.Save(&stringStream, { variable }, quantile, varPercentsToSave);
        TBlob blob = TBlob::FromStream(stringStream);
        TModelPtr compressedModel = new TModel();
        compressedModel->Load(blob);
        return compressedModel;
    }

    std::pair<TModelPtr, THashMap<TString, TString>> DoCompress(const TModel& model, const TCompressionConfig& config)
    {
        TModelPtr currentModel = new TModel(model);

        InitGlobalLog2Console();

        if (config.Verbose) {
            INFO_LOG << "Computing the true predictions" << Endl;
        }
        TVector<TVector<float>> trueValues;
        ApplyModel(*currentModel, config.Dataset, config.HeaderFields, { config.TargetVariable }, trueValues);

        TVector<float> quantiles = CalcQuantiles(config.GridStep);

        if (config.Verbose) {
            INFO_LOG << "Creating (copying) current model" << Endl;
        }
        if (config.Verbose) {
            INFO_LOG << "Computing the dependencies of the variables" << Endl;
        }

        if (config.Verbose) {
            INFO_LOG << "Started" << Endl;
        }
    
        THashMap<TString, TString> variableHint;

        for (TString var : config.Variables) {
            const THashMap<TString, size_t> dependentVariables2LayerIdx = GetVariablesDependencies(*currentModel, config.Variables);

            double bestLoss = Max<double>();

            TMutex mutex;
            NPar::TLocalExecutor executor;
            executor.RunAdditionalThreads(config.ThreadsCount - 1);
            
            Cerr << "Processing " << var;

            TString varWithoutBounds = SplitString(var, ":")[0];
            if (dependentVariables2LayerIdx.contains(varWithoutBounds)) {
                Cerr << " (adding compression modules)" << Endl;

                bool givenBounds = false;
                float givenLowerBound = 0;
                float givenUpperBound = 0;
                {
                    size_t pos = varWithoutBounds.size();
                    if (pos != var.size()) {
                        givenBounds = true;
                        TString boundsStr = var.substr(pos + 2);

                        TVector<TString> items = SplitString(boundsStr, ",");
                        Y_ENSURE(items.size() == 2, "Invalid bounds: " + var);
                        Y_ENSURE(!items[1].empty(), "Invalid bounds: " + var);
                        givenLowerBound = FromString<float>(items[0]);
                        givenUpperBound = FromString<float>(items[1].substr(0, items[1].size() - 1));
                        var = varWithoutBounds;
                    }
                }

                size_t prevOffset = Max<size_t>();
                TVector<std::pair<float, float>> bounds;
                if (givenBounds) {
                    bounds.push_back({ givenLowerBound, givenUpperBound });
                } else {
                    if (config.Verbose) {
                        INFO_LOG << "... Applying the model & computing the quantiles" << Endl;
                    }
                    TVector<float> values;
                    TVector<TVector<float>> samplesValues;
                    ApplyModel(*currentModel, config.Dataset, config.HeaderFields, { var }, samplesValues);
                    for (const TVector<float>& otherValues : samplesValues) {
                        values.insert(values.end(), otherValues.begin(), otherValues.end());
                    }
                    Sort(values);

                    bounds.push_back({ -1, 1 });
                    bounds.push_back({ 0, 1 });
                    for (const float quantile : quantiles) {
                        size_t offset = static_cast<size_t>(values.size() * quantile);
                        if (offset == prevOffset) {
                            continue;
                        }
                        float lowerBound = values[offset];
                        float upperBound = values[values.size() - 1 - offset];
                        bounds.push_back({ lowerBound, upperBound });
                        prevOffset = offset;
                    }
                }

                float bestLowerBound = 0;
                float bestUpperBound = 0;

                const auto calcLossForBounds = [&](int boundsIdx) {
                    float lowerBound = bounds[boundsIdx].first;
                    float upperBound = bounds[boundsIdx].second;
                    if (config.Verbose) {
                        with_lock (mutex) {
                            INFO_LOG << "... ... Creating a compressed model for bounds ["
                                << lowerBound << "," << upperBound << "]" << Endl;
                        }
                    }

                    TModelPtr compressedModel = GetCompressedModel(*currentModel, dependentVariables2LayerIdx, var,
                        lowerBound, upperBound, config.NumBits, config.NumBins);

                    if (config.Verbose) {
                        with_lock (mutex) {
                            INFO_LOG << "... ... Applying model compressed with bounds ["
                                << lowerBound << "," << upperBound << "]" << Endl;
                        }
                    }
                    TVector<TVector<float>> candidateValues;
                    ApplyModel(*compressedModel, config.Dataset, config.HeaderFields, { config.TargetVariable }, candidateValues);

                    if (config.Verbose) {
                        with_lock (mutex) {
                            INFO_LOG << "... ... Scoring for bounds [" << lowerBound << "," << upperBound << "]" << Endl;
                        }
                    }
                    double loss = GetMse(trueValues, candidateValues);
                    with_lock (mutex) {
                        Cerr << "... [" << lowerBound << "," << upperBound << "]\tloss: " << sqrt(loss);
                        if (loss < bestLoss) {
                            Cerr << " best";
                            bestLowerBound = lowerBound;
                            bestUpperBound = upperBound;
                            bestLoss = loss;
                        }
                        Cerr << Endl;
                    }
                };

                executor.ExecRangeWithThrow(calcLossForBounds, 0, bounds.size(), NPar::TLocalExecutor::WAIT_COMPLETE);

                Cerr << " ... selected range [" << bestLowerBound << "," << bestUpperBound <<
                    "] with loss " << sqrt(bestLoss) << Endl;
                if (config.Verbose) {
                    INFO_LOG << "... Saving the best model" << Endl;
                }

                currentModel = GetCompressedModel(*currentModel, dependentVariables2LayerIdx, var,
                        bestLowerBound, bestUpperBound, config.NumBits, config.NumBins);

                if (config.GetOptimizedConfigs) {
                    variableHint[var] = var + ":[" + ToString(bestLowerBound) + "," + ToString(bestUpperBound) + "]";
                } 
            } else {
                Y_ENSURE(currentModel->Parameters.has(varWithoutBounds), "Unknown variable: " + varWithoutBounds);
        
                bool givenQuantile = false;
                float bestQuantile = 0;

                {
                    size_t pos = varWithoutBounds.size();
                    if (pos != var.size()) {
                        givenQuantile = true;
                        TString quantileStr = var.substr(pos + 1);
                        bestQuantile = FromString<float>(quantileStr);
                        var = varWithoutBounds;
                    }
                }
                
                if (!givenQuantile) {
                    Cerr << " (Quantizing parameters)" << Endl;

                    const auto calcLossForQuantile = [&](int quantileIdx) {
                        if (config.Verbose) {
                            with_lock (mutex) {
                                INFO_LOG << "... ... Creating a compressed model for quantile "
                                    << quantiles[quantileIdx] << Endl;
                            }
                        }

                        TModelPtr compressedModel = GetCompressedModel(
                            *currentModel, var, quantiles[quantileIdx],
                            config.SmallValuesToZeroSettings.Value(var, 0));

                        if (config.Verbose) {
                            with_lock (mutex) {
                                INFO_LOG << "... ... Applying model compressed with quantile "
                                    << quantiles[quantileIdx] << Endl;
                            }
                        }
                        TVector<TVector<float>> candidateValues;
                        ApplyModel(*compressedModel, config.Dataset, config.HeaderFields, { config.TargetVariable }, candidateValues);

                        if (config.Verbose) {
                            with_lock (mutex) {
                                INFO_LOG << "... ... Scoring for quantile " << quantiles[quantileIdx] << Endl;
                            }
                        }
                        double loss = GetMse(trueValues, candidateValues);
                        with_lock (mutex) {
                            Cerr << "... " << quantiles[quantileIdx] << "\tloss: " << sqrt(loss);
                            if (loss < bestLoss) {
                                Cerr << " best";
                                bestQuantile = quantiles[quantileIdx];
                                bestLoss = loss;
                            }
                            Cerr << Endl;
                        }
                    };

                    executor.ExecRangeWithThrow(calcLossForQuantile, 0, quantiles.size(), NPar::TLocalExecutor::WAIT_COMPLETE);

                    Cerr << " ... selected quantile " << bestQuantile << " with loss " << sqrt(bestLoss) << Endl;
                    if (config.Verbose) {
                        INFO_LOG << "... Saving the best model" << Endl;
                    }
                } else {
                     Cerr << " ... using quantile " << bestQuantile << " from the config " << Endl;
                    if (config.Verbose) {
                        INFO_LOG << "... Saving the best model" << Endl;
                    }
                }
                currentModel = GetCompressedModel(*currentModel, var, bestQuantile, config.SmallValuesToZeroSettings.Value(var, 0));
                if (config.GetOptimizedConfigs) {
                    variableHint[var] = var + ":" + ToString(bestQuantile);
                } 
            }
        }

        if (config.Verbose) {
            INFO_LOG << "Validating the result" << Endl;
        }
        TVector<TVector<float>> candidateValues;
        ApplyModel(*currentModel, config.Dataset, config.HeaderFields, { config.TargetVariable }, candidateValues);
        Cerr << "The final model has approximation error " << sqrt(GetMse(trueValues, candidateValues)) << Endl;

        if (config.Verbose) {
            INFO_LOG << "Finished" << Endl;
        }
        return {currentModel, variableHint};
    }
} // namespace NNeuralNetApplier
