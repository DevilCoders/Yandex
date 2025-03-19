#include "grads.h"

#include <kernel/dssm_applier/nn_applier/lib/compress.h>
#include <kernel/dssm_applier/nn_applier/lib/concat.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/dssm_applier/nn_applier/lib/optimizations.h>
#include <kernel/dssm_applier/nn_applier/lib/remap.h>
#include <kernel/dssm_applier/nn_applier/lib/tokenizer_builder.h>
#include <kernel/dssm_applier/nn_applier/lib/load_params.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <util/folder/path.h>
#include <util/generic/xrange.h>
#include <util/memory/blob.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/string/vector.h>
#include <util/thread/pool.h>
#include <util/datetime/cputimer.h>
#include <util/system/rusage.h>
#include <util/stream/format.h>

#include <ctime>

using namespace NLastGetopt;
using namespace NNeuralNetApplier;

void LoadModelFromFile(const TString& modelFile, TModel& model) {
    TBlob b = TBlob::FromFile(modelFile);
    model.Load(b);
    model.Init();
}

template <class T2>
void Print(IOutputStream& ss, const T2& val) {
    ss << val;
}

template <>
inline void Print<float>(IOutputStream& ss, const float& val) {
    ss << Sprintf("%.9g", val);
}

template <>
inline void Print<double>(IOutputStream& ss, const double& val) {
    ss << Sprintf("%.17g", val);
}

int AddCompressionModules(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .RequiredArgument()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "Dst model file")
        .Required()
        .RequiredArgument()
        .StoreResult(&dstModelFile);

    TString variable;
    opts.AddCharOption('v', "variable to compress")
        .Required()
        .RequiredArgument()
        .StoreResult(&variable);

    TString compressionType;
    opts.AddCharOption('t', "Compression type (onehot, grid, adagrid)")
        .Required()
        .RequiredArgument()
        .StoreResult(&compressionType);

    size_t numBins;
    opts.AddLongOption("bins", "Number of bins per item")
        .Required()
        .RequiredArgument()
        .StoreResult(&numBins);

    size_t numBits;
    opts.AddLongOption("bits", "Number of bits per item")
        .Optional()
        .DefaultValue("0")
        .RequiredArgument()
        .StoreResult(&numBits);

    TString dataset;
    opts.AddCharOption('f', "File with data (used to compute min-max for adagrid)")
            .Optional()
            .StoreResult(&dataset);

    TString header;
    opts.AddLongOption("header", "Names of the fields in the input dataset")
        .Optional()
        .StoreResult(&header);

    TOptsParseResult res(&opts, argc, argv);

    TBlob b = TBlob::FromFile(modelFile);
    TModel model;
    model.Load(b);

    size_t srcLayerIdx = Max<size_t>();
    for (size_t i = 0; i < model.Layers.size(); ++i) {
        for (const auto& output: model.Layers[i]->GetOutputs()) {
            if (output == variable) {
                srcLayerIdx = i;
                break;
            }
        }
    }

    if (srcLayerIdx == Max<size_t>()) {
        Cerr << "Cannot find layer producing variable " << variable << Endl;
        Y_VERIFY(false);
    }

    TVector<ILayerPtr> compressors;
    if (compressionType == "grid" || compressionType == "adagrid") {
        double min = -1;
        double max = 1;
        if (compressionType == "adagrid") {
            if (header.size() == 0) {
                Cerr << "You should specify header for adagrid" << Endl;
                Y_VERIFY(false);
            }
            TVector<TString> headerFields = SplitString(header, ",", 0, KEEP_EMPTY_TOKENS);
            model.Init();
            TFileInput f(dataset);
            TString line;
            min = 1e100;
            max = -1e100;
            while (f.ReadLine(line)) {
                TVector<TString> values;
                StringSplitter(line).Split('\t').AddTo(&values);
                Y_VERIFY(values.size() <= headerFields.size());
                TSample* sample = new TSample(headerFields, values);
                TVector<float> result;
                model.Apply(sample, {variable}, result);
                min = Min(min, (double)*MinElement(result.begin(), result.end()));
                max = Max(max, (double)*MaxElement(result.begin(), result.end()));
            }
            Cerr << "Detected data range: [" << min << ", " << max << "]" << Endl;
        }
        compressors.push_back(new TCompressorLayer(variable + "$before_compress",
            variable + "$compressed", numBins, min, max, numBits));
        compressors.push_back(new TDecompressorLayer(variable + "$compressed",
            variable, numBins, min, max, numBits));
    } else if (compressionType == "onehot") {
        compressors.push_back(new TOneHotCompressorLayer(variable + "$before_compress",
            variable + "$compressed", numBins));
        compressors.push_back(new TOneHotDecompressorLayer(variable + "$compressed", variable, numBins));
    } else {
        Cerr << "Unknown compression mode: " << compressionType << Endl;
        Y_VERIFY(false);
    }
    model.Layers[srcLayerIdx]->RenameVariable(variable, variable + "$before_compress");
    model.Layers.insert(model.Layers.begin() + srcLayerIdx + 1, compressors.begin(), compressors.end());
    {
        TFixedBufferFileOutput f(dstModelFile + "~");
        model.Save(&f);
    }
    TFsPath(dstModelFile + "~").RenameTo(dstModelFile);

    return 0;
}

int ConcatModels(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TVector<TString> modelFiles;
    opts.AddLongOption('m', "models", "model files, 'zero' for zero output")
        .Required()
        .SplitHandler(&modelFiles, ',');

    TVector<TString> suffixes;
    opts.AddLongOption('s', "suffixes", "suffixes of model layer names")
        .Required()
        .SplitHandler(&suffixes, ',');

    TVector<TString> keep;
    opts.AddLongOption('k', "keep", "keep variable, which would not be renamed")
        .Optional()
        .SplitHandler(&keep, ',');

    TVector<TString> outputs;
    opts.AddLongOption("outputs", "models outputs, empty for joint_output")
        .Optional()
        .SplitHandler(&outputs, ',');
    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);
    bool noSoftsign;
    opts.AddLongOption("no_softsign")
        .Optional()
        .NoArgument()
        .SetFlag(&noSoftsign);
    bool ignoreVersionsDiff;
    opts.AddLongOption("ignore_versions_diff")
        .Optional()
        .NoArgument()
        .SetFlag(&ignoreVersionsDiff);
    TString metaData;
    opts.AddLongOption("meta-data", "Set model metadata to this string (copy from the first source by default)")
        .Optional()
        .RequiredArgument()
        .StoreResult(&metaData);
    TOptsParseResult res(&opts, argc, argv);

    Y_VERIFY(modelFiles.size() == suffixes.size(), "There should be as many suffixes as models");
    Y_VERIFY(!outputs.size() || outputs.size() == modelFiles.size(), "There should be as many outputs as models");

    if (!outputs.size()) {
        outputs.resize(modelFiles.size());
    }

    TVector<TModel> models(modelFiles.size());

    bool hasZeroLayer = false;

    auto createZeroModel = [&]() {
        TModel model;
        model.Layers.push_back(new TZeroLayer("input", "zero"));
        return model;
    };

    for (ui64 i = 0; i < modelFiles.size(); ++i) {
        if (modelFiles[i] == "zero") {
            if (!hasZeroLayer) {
                models.push_back(createZeroModel());
                hasZeroLayer = true;
            }
            outputs[i] = "zero";
        } else {
            LoadModelFromFile(modelFiles[i], models[i]);
            if (suffixes[i].size()) {
                AddSuffix(models[i], suffixes[i], keep);
            }
            outputs[i] = (outputs[i].size() ? outputs[i] : "joint_output") + suffixes[i];
        }
    }

    TModelPtr resultModel = MergeModels(models, ignoreVersionsDiff);

    resultModel->Layers.push_back(new TConcatLayer(outputs, TString("joint_output") + (noSoftsign ? "" : "_unnormalized")));
    if (!noSoftsign) {
        resultModel->Layers.push_back(new TSoftSignLayer(
            "joint_output_unnormalized", "joint_output", 1.0, 0.5, 0.5));
    }

    if (res.Has("meta-data")) {
        resultModel->SetMetaData(metaData);
    } else if (!models.empty()) {
        resultModel->SetMetaData(models[0].GetMetaData());
    }

    TFixedBufferFileOutput outputFile(outputModelFile);
    resultModel->Save(&outputFile);
    return 0;
}

using TRenameMap = THashMap<TString, TString>;

template<>
TRenameMap FromStringImpl<TRenameMap>(const char* s, size_t len) {
    TStringBuf buf(s, len);
    TRenameMap remap;
    for (TStringBuf tok = buf.NextTok(','); !tok.empty(); tok = buf.NextTok(',')) {
        TStringBuf from, to;
        Split(tok, ':', from, to);
        Y_ENSURE(!remap.contains(from), "Variable \"" << from << "\" occured muliple times");
        remap[from] = to;
    }

    return remap;
}

int Rename(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TRenameMap remap;
    opts.AddLongOption('r', "remap", "rename map in format var_before:var_after,...")
        .Required()
        .StoreResult(&remap);

    bool renameHeader;
    opts.AddLongOption("header", "Rename header fields")
        .Optional()
        .NoArgument()
        .SetFlag(&renameHeader);
    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    if (renameHeader) {
        for (auto& rule : remap) {
            bool success = false;
            for (auto& layer : model.Layers) {
                auto* l = dynamic_cast<TFieldExtractorLayer*>(layer.Get());
                success |= l && l->RenameField(rule.first, rule.second);
            }
            if (!success) {
                Cerr << "No such variable " << rule.first << "\n";
                Y_VERIFY(false);
            }
        }
    } else {
        TVector<TString> allVariables = model.AllVariables();
        for (auto& rule : remap) {
            if (Find(allVariables.begin(), allVariables.end(), rule.first) != allVariables.end()) {
                model.RenameVariable(rule.first, rule.second);
            } else {
                Cerr << "No such variable " << rule.first << "\n";
                Y_VERIFY(false);
            }
        }
    }

    TFixedBufferFileOutput outputFile(outputModelFile);
    model.Save(&outputFile);
    return 0;
}

int Remap(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString toModelFile;
    opts.AddLongOption("to_model", "model, to which distribution do remap")
        .Required()
        .StoreResult(&toModelFile);

    TString fromModelFile;
    opts.AddLongOption("from_model", "model, from which distribution do remap")
        .Required()
        .StoreResult(&fromModelFile);

    TString inputFile;
    opts.AddLongOption('i', "input", "input file, will be read two times")
        .Required()
        .StoreResult(&inputFile);

    TString fromOutput;
    opts.AddLongOption("from_output", "from_model output to remap")
        .Optional()
        .DefaultValue("joint_output")
        .StoreResult(&fromOutput);

    TString toOutput;
    opts.AddLongOption("to_output", "to_model output to remap")
        .Optional()
        .DefaultValue("joint_output")
        .StoreResult(&toOutput);

    TString fromHeader;
    opts.AddLongOption("from_header", "from_model header")
        .Required()
        .StoreResult(&fromHeader);

    TString toHeader;
    opts.AddLongOption("to_header", "to_model header")
        .Required()
        .StoreResult(&toHeader);

    ui64 numPoints;
    opts.AddCharOption('n', "number of map points")
        .Required()
        .StoreResult(&numPoints);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TVector<TString> fromHeaderFields = SplitString(fromHeader, ",", 0, KEEP_EMPTY_TOKENS);
    TVector<TString> toHeaderFields = SplitString(toHeader, ",", 0, KEEP_EMPTY_TOKENS);

    TModel fromModel;
    LoadModelFromFile(fromModelFile, fromModel);

    TModel toModel;
    LoadModelFromFile(toModelFile, toModel);

    TModelPtr remappedModel = DoRemap(fromModel, toModel, inputFile, fromOutput, toOutput, fromHeaderFields, toHeaderFields, numPoints);

    auto subModel = remappedModel->GetSubmodel(fromOutput);

    Cerr << "Saving new model\n";
    TFixedBufferFileOutput outputFile(outputModelFile);
    subModel->Save(&outputFile);
    return 0;
}

int Compress(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    TCompressionConfig config;

    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "Dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    opts.AddCharOption('v', "variables to compress")
        .Required()
        .RequiredArgument()
        .AppendTo(&config.Variables);

    opts.AddCharOption('t', "variable to optimize mse difference")
        .Required()
        .RequiredArgument()
        .StoreResult(&config.TargetVariable);

    opts.AddLongOption("step", "Grid search step size")
        .DefaultValue("2")
        .Optional()
        .RequiredArgument()
        .StoreResult(&config.GridStep);

    opts.AddCharOption('f', "Dataset (used to tune quantization quantiles)")
        .Required()
        .RequiredArgument()
        .StoreResult(&config.Dataset);

    opts.AddLongOption("bins", "Number of bins per item (implemented only for dependent variables)")
        .DefaultValue("256")
        .Optional()
        .RequiredArgument()
        .StoreResult(&config.NumBins);

    opts.AddLongOption("bits", "Number of bits per item, calculated automatically if zero. (It may be usefull, if you want 8 bits instead of 5 for faster multiplication")
        .Optional()
        .DefaultValue("0")
        .RequiredArgument()
        .StoreResult(&config.NumBits);

    TString header;
    opts.AddLongOption("header", "Names of the fields in the input dataset")
        .Required()
        .RequiredArgument()
        .StoreResult(&header);

    opts.AddLongOption("verbose", "Print some debug information")
        .Optional()
        .NoArgument()
        .SetFlag(&config.Verbose);

    opts.AddLongOption('j', "threads-count", "Number of quantiles to process in parallel")
        .Optional()
        .DefaultValue(1u)
        .StoreResult(&config.ThreadsCount);

    opts.AddLongOption("small-values-deletion", "add variable (must be data of sparseToEmbedding type) to small values optimization politic")
        .RequiredArgument("Variable:Percent")
        .Optional()
        .Handler1T<TString>([&](TString x) {
            TString name;
            float percent = 0;
            StringSplitter(x).Split(':').CollectInto(&name, &percent);
            config.SmallValuesToZeroSettings[name] = percent;
        });

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    config.HeaderFields = SplitString(header, ",", 0, KEEP_EMPTY_TOKENS);

    TModelPtr compressedModel = DoCompress(model, config).first;

    TFileOutput file(dstModelFile + "~");
    compressedModel->Save(&file);
    TFsPath(dstModelFile + "~").RenameTo(dstModelFile);

    return 0;
}

// Some models have a united query field extractor and document field extractor
// It causes troubles when we want to compute just a query vector or a document vector
int SplitExtractor(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "Dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TVector<TString> queryFields;
    opts.AddCharOption('q', "query fields")
        .Required()
        .RequiredArgument()
        .AppendTo(&queryFields);
    TOptsParseResult res(&opts, argc, argv);

    TBlob b = TBlob::FromFile(modelFile);
    TModelPtr model = new TModel();
    model->Load(b);

    size_t layerIdx = 0;
    for (const auto& layer : model->Layers) {
        if (dynamic_cast<TFieldExtractorLayer*>(layer.Get()) != nullptr) {
            const auto& l = dynamic_cast<TFieldExtractorLayer*>(layer.Get());
            THashMap<TString, TString> queryFieldsMap;
            THashMap<TString, TString> docFieldsMap;
            for (const auto& item : l->GetAnnotations()) {
                if (Find(queryFields.begin(), queryFields.end(), item.first) != queryFields.end()) {
                    queryFieldsMap[item.first] = item.second;
                } else {
                    docFieldsMap[item.first] = item.second;
                }
            }
            if (!queryFieldsMap.empty() && !docFieldsMap.empty()) {
                const TString inputVar = l->GetInputs()[0];
                model->Layers[layerIdx] = new TFieldExtractorLayer(inputVar, queryFieldsMap);
                model->Layers.insert(model->Layers.begin() + layerIdx,
                    new TFieldExtractorLayer(inputVar, docFieldsMap));
            }
            break;
        }
        ++layerIdx;
    }
    TFsPath(TFsPath(dstModelFile).Dirname()).MkDirs();
    TFixedBufferFileOutput f(dstModelFile);
    model->Save(&f);
    return 0;
}

int ExtractSubmodel(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "Dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TVector<TString> variables;
    opts.AddCharOption('v', "Extract graph for these variables")
        .Required()
        .RequiredArgument()
        .AppendTo(&variables);

    TVector<TString> terminalInputs;
    opts.AddCharOption('t', "Extract graph, assuming these inputs are terminal")
        .Optional()
        .RequiredArgument()
        .AppendTo(&terminalInputs);

    TString metaData;
    opts.AddLongOption("meta-data", "Set model metadata to this string (copy from source by default)")
        .Optional()
        .RequiredArgument()
        .StoreResult(&metaData);

    TOptsParseResult res(&opts, argc, argv);

    NNeuralNetApplier::TModel model;
    TBlob b = TBlob::FromFile(modelFile);
    model.Load(b);
    auto subModel = model.GetSubmodel({variables.begin(), variables.end()}, {terminalInputs.begin(), terminalInputs.end()});
    if (res.Has("meta-data")) {
        subModel->SetMetaData(metaData);
    } else {
        subModel->SetMetaData(model.GetMetaData());
    }
    TFsPath(TFsPath(dstModelFile).Dirname()).MkDirs();
    TFixedBufferFileOutput f(dstModelFile);
    subModel->Save(&f);
    return 0;
}

int Apply(int argc, const char** argv) {
    TDuration load_time, apply_time;
    size_t processed_line_cnt = 0;

    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        // For compatibility with dssm
        .AddShortName('d')
        .Required()
        .RequiredArgument()
        .StoreResult(&modelFile);

    TVector<TString> outputVariables;
    opts.AddCharOption('o', "variable to output")
        .Optional()
        .DefaultValue("joint_output")
        .RequiredArgument()
        .AppendTo(&outputVariables);

    TString header;
    opts.AddLongOption("header", "Names of the fields in the input")
        .Required()
        .RequiredArgument()
        .StoreResult(&header);

    size_t batch_size = 1;
    opts.AddCharOption('b', "batch size")
        .Optional()
        .DefaultValue("1")
        .RequiredArgument()
        .StoreResult(&batch_size);

    NNeuralNetApplier::TLoadParams load_params;
    opts.AddLongOption("hash", "Use HashMap in tokenizers")
        .Optional()
        .NoArgument()
        .SetFlag(&load_params.UseHashMapTokenizersOptimization);

    bool memLoad = false;
    opts.AddLongOption("mem", "Copy model to memory before load")
        .Optional()
        .NoArgument()
        .SetFlag(&memLoad);

    TVector<TString> fields;
    opts.AddCharOption('f', "Field from input to output (useful for yt streaming)")
        .RequiredArgument()
        .AppendTo(&fields);
    TOptsParseResult res(&opts, argc, argv);

    if (!outputVariables) {
        outputVariables.push_back("joint_output");
    }

    TBlob b;
    if (memLoad) {
        b = TBlob::FromFileContent(modelFile);
    } else {
        b = TBlob::FromFile(modelFile);
    }
    TModel model;
    TSimpleTimer timer;
    model.Load(b, load_params);
    load_time = timer.Get();
    model.Init();

    size_t memory_consumed = TRusage::GetCurrentRSS();

    TVector<TString> headerFields = SplitString(header, ",", 0, KEEP_EMPTY_TOKENS);
    TVector<size_t> fieldsIdxs;
    for (const auto& field : fields) {
        if (Find(headerFields.begin(), headerFields.end(), field) == headerFields.end()) {
            Cerr << "Cannot find " << field << " in the header" << Endl;
            Y_VERIFY(false);
        }
        fieldsIdxs.push_back(Find(headerFields.begin(), headerFields.end(), field) - headerFields.begin());
    }

    TVector<TAtomicSharedPtr<ISample>> samples;
    samples.reserve(batch_size);
    TVector<TVector<TString>> values;
    values.resize(batch_size);
    TVector<TVector<float>> results;

    auto FlushBatch = [&]() {
        timer.Reset();
        model.Apply(samples, outputVariables, results);
        apply_time += timer.Get();

        samples.clear();

        for (size_t k = 0; k < results.size(); ++k) {

            for (const auto& idx : fieldsIdxs) {
                Cout << values[k][idx] << "\t";
            }
            values[k].clear();

            for (size_t i = 0; i < results[k].size(); ++i) {
                if (i) {
                    Cout << " ";
                }
                Print(Cout, results[k][i]);
            }
            Cout << Endl;
        }
    };

    TString line;
    while (Cin.ReadLine(line)) {
        size_t pos = samples.size();
        StringSplitter(line).Split('\t').AddTo(&(values[pos]));
        if (values[pos].size() < headerFields.size()) {
            Cerr << "Bad line: " << line << Endl;
            Y_VERIFY(headerFields.size() <= values[pos].size());
        }
        samples.emplace_back(new TSample(headerFields, values[pos]));
        ++processed_line_cnt;

        if (samples.size() >= batch_size) {
            FlushBatch();
        }
    }
    if (!samples.empty()) {
        FlushBatch();
    }

    Cerr << "Processed lines: " << processed_line_cnt << "\n";
    Cerr << "Memory consumed:" << HumanReadableSize(memory_consumed, SF_BYTES) << "\n";
    Cerr << "Load time: " << load_time.SecondsFloat() << "s\n";
    Cerr << "Apply time: " << apply_time.SecondsFloat() << "s\n";
    return 0;
}

int Describe(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Optional().StoreResult(&modelFile);

    bool graphviz = false;
    opts.AddCharOption('g', NO_ARGUMENT, "describe in graphviz (.dot) format")
        .Optional().StoreResult(&graphviz, true);
    bool printMetaData = false;
    opts.AddLongOption("print-meta", "print meta data")
        .NoArgument().Optional().StoreResult(&printMetaData, true);

    TOptsParseResult res(&opts, argc, argv);

    TBlob b = TBlob::FromFile(modelFile);
    TModel model;
    model.Load(b);
    model.Init();
    Cout << (graphviz ? model.ModelGraphDotString() : model.ModelGraphString()) << Endl;
    if (printMetaData) {
        Cout << "Version Range: [" << model.GetSupportedVersions().GetBegin() << " : " << model.GetSupportedVersions().GetEnd() << "]" << Endl;
        Cout << "MetaData: " << model.GetMetaData() << Endl;
    }
    return 0;
}

TString CheckModelInputs(TModel& model) {
    TSet<TString> outputs;
    for (const auto& layer : model.Layers) {
        for (const TString& output : layer->GetOutputs()) {
            /*if (outputs.has(output)) {
                return TStringBuilder() << "duplicate output: " << output;
            }*/
            outputs.insert(output);
        }
    }
    for (const auto& layer : model.Layers) {
        for (const TString& input : layer->GetInputs()) {
            if (!outputs.contains(input) && !model.Parameters.has(input) && input != "input") {
                return TStringBuilder() << "missed source for input " << input;
            }
        }
    }
    return "";
}

int AddGradientsStatistics(int argc, const char** argv) {
    TOpts opts;

    opts.AddLongOption('h', "help", "Print this help message and exit")
        .Handler(&PrintUsageAndExit)
        .NoArgument();

    TString modelFile;
    opts.AddCharOption('m', "model file").Required().StoreResult(&modelFile);
    TString dstModelFile;
    opts.AddCharOption('d', "dst model file").Required().StoreResult(&dstModelFile);

    opts.AddLongOption("word-weight", "weight of original word in aggregation").DefaultValue("1.0");
    opts.AddLongOption("trigram3-weight", "weight of trigrams fully inside word").DefaultValue("1.0");
    opts.AddLongOption("trigram2-weight", "weight of trigrams intersects word by 2 letters").DefaultValue("0.0");
    opts.AddLongOption("trigram1-weight", "weight of intersects word by 1 letter").DefaultValue("0.0");
    opts.AddLongOption("bigram-weight", "weight of bigram containing word").DefaultValue("1.0");
    opts.AddLongOption("header", "header for numeric checks");

    opts.AddLongOption("concat-output", "concatenate all statistics to this output");
    opts.AddLongOption("add-to",  "append concat-output to given_variable");
    opts.AddLongOption("softsign", "apply softsign to concat-output").HasArg(NLastGetopt::NO_ARGUMENT);

    opts.SetFreeArgsMin(1);
    opts.SetFreeArgDefaultTitle(
        "model_input:model_output:aggregation:name",
        "aggregation is one of: max, min, std (example: query:joint_output:max:max_grad__joint_output_by_query)"
    );

    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    TWeightsJoinOptions joinOptions{
        res.Get<float>("word-weight"),
        res.Get<float>("trigram3-weight"),
        res.Get<float>("trigram2-weight"),
        res.Get<float>("trigram1-weight"),
        res.Get<float>("bigram-weight"),
    };

    struct TGradAggregationParams {
        TString Input;
        TString Aggregation;
        TString Name;
    };
    //output -> [(input, aggregation)]
    THashMap<TString, TVector<TGradAggregationParams>> targets;

    for (const auto& s : res.GetFreeArgs()) {
        TVector<TString> fields;
        StringSplitter(s).Split(':').SkipEmpty().Collect(&fields);
        Y_VERIFY(fields.size() == 4);
        targets[fields[1]].push_back(TGradAggregationParams{fields[0], fields[2], fields[3]});
    }

    bool needConcat = res.Has("concat-output");
    bool addTo = res.Has("add-to");
    bool applySoftsign = res.Has("softsign");
    if (!needConcat && addTo) {
        Cerr << "can't add statistics directly to joint_output" << Endl;
        return 1;
    }

    NNeuralNetApplier::TModel model;
    TBlob b = TBlob::FromFile(modelFile);
    model.Load(b);

    TVector<TString> addedStats;
    for (const auto& output : targets) {
        TSet<TString> inputs;
        for (const auto& x : output.second) {
            inputs.insert(x.Input);
        }
        AddBackProp(model, output.first, TVector<TString>(inputs.begin(), inputs.end()), joinOptions);
        for (const auto& x : output.second) {
            TString sparseGradOut = GetGradDataName(output.first, x.Input);
            addedStats.push_back(x.Name);
            if (x.Aggregation == "max") {
                model.Layers.push_back(new TSparseMatrixStatisticLayer<NAggregateFunctions::TMax>(sparseGradOut, x.Name));
            } else if (x.Aggregation == "min") {
                model.Layers.push_back(new TSparseMatrixStatisticLayer<NAggregateFunctions::TMin>(sparseGradOut, x.Name));
            } else if (x.Aggregation == "std") {
                model.Layers.push_back(new TSparseMatrixStatisticLayer<NAggregateFunctions::TStd>(sparseGradOut, x.Name));
            } else if (x.Aggregation == "mean") {
                model.Layers.push_back(new TSparseMatrixStatisticLayer<NAggregateFunctions::TMean>(sparseGradOut, x.Name));
            } else if (x.Aggregation == "moment3_central") {
                model.Layers.push_back(new TSparseMatrixStatisticLayer<NAggregateFunctions::TMoment3Central>(sparseGradOut, x.Name));
            }
        }
    }

    if (needConcat) {
        const TString concatOutput = res.Get<TString>("concat-output");
        const TString resultOutput = res.Get<TString>("add-to");
        model.Layers.push_back(new TConcatLayer(addedStats, concatOutput));
        if (addTo) {
            if (applySoftsign) {
                const TString concatOutputSoftsign = concatOutput + "_softsign";
                model.Layers.push_back(new TSoftSignLayer(concatOutput, concatOutputSoftsign, 1.0, 0.5, 0.5));
            }
            TVector<TString> jointInputs;
            for (auto it = model.Layers.begin(); it != model.Layers.end(); ++it) {
                if (it->Get()->GetOutputs()[0] == resultOutput) {
                    if (it->Get()->GetTypeName() == "TConcatLayer") {
                        jointInputs = it->Get()->GetInputs();
                        model.Layers.erase(it);
                        break;
                    } else {
                        model.RenameVariable(resultOutput, resultOutput + "_model");
                        jointInputs.push_back(resultOutput + "_model");
                    }
                }
            }
            jointInputs.push_back(concatOutput + (applySoftsign ? "_softsign" : ""));
            model.Layers.push_back(new TConcatLayer(jointInputs, resultOutput));
        }
    }

    auto correctness = CheckModelInputs(model);
    if (!correctness.empty()) {
        ythrow yexception() << "model incorrect now: " << correctness;
    }

    if (res.Has("header")) {
        model.Init();
        TVector<TString> headerFields = SplitString(res.Get("header"), ",", 0, KEEP_EMPTY_TOKENS);
        TString line;
        TVector<TSample> samples;
        while (Cin.ReadLine(line)) {
            TVector<TString> values;
            StringSplitter(line).Split('\t').AddTo(&values);
            Y_VERIFY(headerFields.size() <= values.size());
            samples.emplace_back(headerFields, values);
        }
        for (const auto& output : targets) {
            for (const auto& x : output.second) {
                if (!CheckGrads(model, output.first, x.Input, samples)) {
                    ythrow yexception() << "incorrect gradient " << x.Input << " -> " << output.first;
                }
            }
        }
    }

    TFixedBufferFileOutput f(dstModelFile);
    model.Save(&f);
    return 0;
}

int MergeTries(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    DoMergeTries(model);

    TFixedBufferFileOutput f(dstModelFile);
    model.Save(&f);
    return 0;
}

int AddTrigramIndex(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    DoAddTrigramIndex(model);

    TFixedBufferFileOutput f(dstModelFile);
    model.Save(&f);

    return 0;
}

int PerformOptimizations(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    DoMergeTries(model);
    DoAddTrigramIndex(model);

    TFixedBufferFileOutput f(dstModelFile);
    model.Save(&f);

    return 0;
}

int SetConstDocPart(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddCharOption('m', "model file")
        .Required()
        .StoreResult(&modelFile);

    TString dstModelFile;
    opts.AddCharOption('d', "dst model file")
        .Required()
        .StoreResult(&dstModelFile);

    TString outputVariable;
    opts.AddCharOption('o', "output variable (default: \"joint_output\")")
        .Optional()
        .DefaultValue("joint_output")
        .StoreResult(&outputVariable);

    TString docEmbeddingVariable;
    opts.AddCharOption('e', "doc embedding variable (default: \"doc_embedding\")")
        .Optional()
        .DefaultValue("doc_embedding")
        .StoreResult(&docEmbeddingVariable);

    TVector<TString> docPartFields;
    opts.AddCharOption('f', "doc fields (default: \"lr\")")
        .Optional()
        .DefaultValue("lr")
        .AppendTo(&docPartFields);

    TVector<TString> docPartConsts;
    opts.AddCharOption('c', "inputs for doc part fields (default: \"0\")")
        .Optional()
        .DefaultValue("0")
        .AppendTo(&docPartConsts);

    TOptsParseResult res(&opts, argc, argv);

    TBlob b = TBlob::FromFile(modelFile);
    TModel model;
    model.Load(b);

    auto docPart = model.GetSubmodel(docEmbeddingVariable);
    TVector<float> docEmbedding;
    docPart->Apply(
        new NNeuralNetApplier::TSample(docPartFields, docPartConsts),
        {docEmbeddingVariable},
        docEmbedding
    );

    auto queryOnlyModel = model.GetSubmodel(outputVariable, {docEmbeddingVariable});
    queryOnlyModel->Parameters[docEmbeddingVariable] = new TMatrix(1, docEmbedding.size(), docEmbedding);

    TFixedBufferFileOutput f(dstModelFile);
    queryOnlyModel->Save(&f);

    return 0;
}

int InheritVersion(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddLongOption("model", "model file")
        .Required()
        .StoreResult(&modelFile);

    TString fromModelFile;
    opts.AddLongOption("from_model", "model, which version to take")
        .Required()
        .StoreResult(&fromModelFile);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    TModel fromModel;
    LoadModelFromFile(fromModelFile, fromModel);

    model.SetSupportedVersions(fromModel.GetSupportedVersions());

    TFixedBufferFileOutput outputFile(outputModelFile);
    model.Save(&outputFile);
    return 0;
}

int IncreaseVersion(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddLongOption("model", "model file")
        .Required()
        .StoreResult(&modelFile);

    bool supportPrevious = false;
    opts.AddLongOption("support_previous", "support previous version")
        .NoArgument()
        .SetFlag(&supportPrevious);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    model.IncreaseVersion(supportPrevious);

    TFixedBufferFileOutput outputFile(outputModelFile);
    model.Save(&outputFile);
    return 0;
}

int SetVersion(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddLongOption("model", "model file")
        .Required()
        .StoreResult(&modelFile);

    TString version;
    opts.AddLongOption("version", "model version")
        .Required()
        .StoreResult(&version);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    model.SetSupportedVersions(TVersionRange(FromString<ui32>(version)));

    TFixedBufferFileOutput outputFile(outputModelFile);
    model.Save(&outputFile);
    return 0;
}

int RemoveDict(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString modelFile;
    opts.AddLongOption('m', "model", "model file")
        .Required()
        .StoreResult(&modelFile);

    TString outputModelFile;
    opts.AddLongOption('o', "output", "output model file")
        .Required()
        .StoreResult(&outputModelFile);

    TOptsParseResult res(&opts, argc, argv);

    TModel model;
    LoadModelFromFile(modelFile, model);

    for (const auto& layer : model.Layers) {
        TStringToSparseMatrixLayer* sparsifierLayer = dynamic_cast<TStringToSparseMatrixLayer*>(layer.Get());
        if (sparsifierLayer != nullptr) {
            TSparsifier* sparsifier = sparsifierLayer->GetSparsifier();
            if (sparsifier != nullptr) {
                *sparsifier = TSparsifier();
            }
        }

        TSparseMatrixToEmbeddingLayer* toEmbeddingLayer = dynamic_cast<TSparseMatrixToEmbeddingLayer*>(layer.Get());
        if (toEmbeddingLayer != nullptr) {
            TMatrix* embeddingMatrix = toEmbeddingLayer->GetEmbeddingMatrix();
            if (embeddingMatrix != nullptr) {
                *embeddingMatrix = TMatrix(0, embeddingMatrix->GetNumColumns());
            }
            TCharMatrix *charEmbeddingMatrix = toEmbeddingLayer->GetCharEmbeddingMatrix();
            if (charEmbeddingMatrix != nullptr) {
                *charEmbeddingMatrix = TCharMatrix(0, charEmbeddingMatrix->GetNumColumns());
            }
        }
    }

    TFixedBufferFileOutput outputFile(outputModelFile);
    model.Save(&outputFile);
    return 0;
}

int MoveParameter(int argc, const char** argv) {
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    opts.AddHelpOption('h');

    TString toModelFile;
    opts.AddLongOption("to_model", "from which to take everything else")
        .Required()
        .StoreResult(&toModelFile);

    TString fromModelFile;
    opts.AddLongOption("from_model", "from which to take the parameter")
        .Required()
        .StoreResult(&fromModelFile);

    TString destinationPath;
    opts.AddLongOption("destination_path", "where to write the resulting model")
        .Required()
        .StoreResult(&destinationPath);

    TString sourceParamName;
    opts.AddLongOption("source_param", "what param to take")
        .Required()
        .StoreResult(&sourceParamName);

    TString destParamName;
    opts.AddLongOption("dest_param", "where to put the param")
        .Required()
        .StoreResult(&destParamName);

    TOptsParseResult res(&opts, argc, argv);

    TModel fromModel;
    LoadModelFromFile(fromModelFile, fromModel);

    TModel toModel;
    LoadModelFromFile(toModelFile, toModel);

    toModel.Parameters[destParamName] = fromModel.Parameters.at(sourceParamName);

    TFixedBufferFileOutput destinationFile(destinationPath);
    toModel.Save(&destinationFile);
    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode(
        "apply",
        Apply,
        "-- Normalize urls in input stream"
        );
    modChooser.AddMode(
        "describe",
        Describe,
        "-- Describe model"
        );
    modChooser.AddMode(
        "add_compression",
        AddCompressionModules,
        "-- Add compression modules"
        );
    modChooser.AddMode(
        "compress",
        Compress,
        "-- Compress given matrices or variables (requires a dataset for tuning)"
        );
    modChooser.AddMode(
        "submodel",
        ExtractSubmodel,
        "-- Extract submodel"
        );
    modChooser.AddMode(
        "concat",
        ConcatModels,
        "-- Concat given models (create a model, producing vector of outputs of given models)"
    );
    modChooser.AddMode(
        "rename",
        Rename,
        "-- Rename variables"
    );
    modChooser.AddMode(
        "remap",
        Remap,
        "-- Remap one model output in distribution of another model"
    );
    modChooser.AddMode(
        "split_extractor",
        SplitExtractor,
        "-- Split the field extractor into a query field extractor and a document field extractor"
    );
    modChooser.AddMode(
        "add_gradient_statistics",
        AddGradientsStatistics,
        "-- Calculate gradient-based statistics for the sparse inputs"
    );
    modChooser.AddMode(
        "merge_tries",
        MergeTries,
        "-- Merge tries and tokenizers to have one for each input"
    );
    modChooser.AddMode(
        "add_trigram_index",
        AddTrigramIndex,
        "-- Replace all occurrences of TTrigramTokenizer with TCachedTrigramTokenizer (with trigrams index)"
    );
    modChooser.AddMode(
        "perform_optimizations",
        PerformOptimizations,
        "-- combination of merge_tries and add_trigram_index"
    );
    modChooser.AddMode(
        "set_const_doc_part",
        SetConstDocPart,
        "-- Place constant document part into model parameters and remove unnecessary layers"
    );
    modChooser.AddMode(
        "inherit_version",
        InheritVersion,
        "-- Set model version as another model version"
    );
    modChooser.AddMode(
        "increase_version",
        IncreaseVersion,
        "-- Increase model version"
    );
    modChooser.AddMode(
        "set_version",
        SetVersion,
        "-- Set model version"
    );
    modChooser.AddMode(
        "remove_dict",
        RemoveDict,
        "-- Remove dict from model (e.g. to make mock for test)"
    );

    modChooser.AddMode(
        "move_parameter",
        MoveParameter,
        "-- Move parameter from one model to another"
    );

    return modChooser.Run(argc, argv);
}
