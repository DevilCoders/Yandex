#include <kernel/ethos/lib/cross_validation/cross_validation.h>
#include <kernel/ethos/lib/metrics/binary_classification_metrics.h>
#include <kernel/ethos/lib/text_classifier/binary_classifier.h>
#include <kernel/ethos/lib/text_classifier/classifier_features.h>
#include <kernel/ethos/lib/text_classifier/compositions.h>
#include <kernel/ethos/lib/text_classifier/document.h>
#include <kernel/ethos/lib/text_classifier/multi_classifier.h>
#include <kernel/ethos/lib/text_classifier/util.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>

#include <library/cpp/scheme/scheme.h>

#include <library/cpp/streams/factory/factory.h>

#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/printf.h>

#include <util/system/fs.h>
#include <util/system/yassert.h>

using namespace NEthos;

template <typename TBinaryClassificationModel, typename TItems>
TBcMetrics CalculateMetrics(const TBinaryClassificationModel& model, const TItems& items) {
    TBcMetricsCalculator metricsCalculator;

    for (auto&& item : items) {
        TBinaryLabelWithFloatPrediction prediction = model.Apply(item);
        metricsCalculator.Add(prediction, item.Label);
    }

    return metricsCalculator.AllMetrics();
}

void PrintMetrics(const TBcMetrics& metrics, bool jsonMetrics = false) {
    if (jsonMetrics) {
        Cout << metrics.ToTValue().ToJson(NSc::TValue::EJsonOpts::JO_SORT_KEYS) << "\n";
    } else {
        Cout << "    " << "Precision: " << Sprintf("%.5lf", metrics.Precision) << Endl;
        Cout << "    " << "Recall:    " << Sprintf("%.5lf", metrics.Recall) << Endl;
        Cout << "    " << "FPRate:    " << Sprintf("%.5lf", metrics.FalsePositiveRate) << Endl;
        Cout << "    " << "F1:        " << Sprintf("%.5lf", metrics.F1) << Endl;
        Cout << "    " << "BestF1:    " << Sprintf("%.5lf", metrics.BestF1) << Endl;
        Cout << "    " << "Accuracy:  " << Sprintf("%.5lf", metrics.Accuracy) << Endl;
        Cout << "    " << "AUC:       " << Sprintf("%.5lf", metrics.AUC) << Endl;
    }
}

template <typename TBinaryClassificationModel, typename TItems>
void PrintMetrics(const TBinaryClassificationModel& model, const TItems& items, bool jsonMetrics = false) {
    TBcMetrics metrics = CalculateMetrics(model, items);
    PrintMetrics(metrics, jsonMetrics);
}

struct TTextDocumentsReadingOptions {
    TString TextDocsFileName;
    TString BinDocsFileName;
    bool HasIdColumn = false;
    bool HasWeight = false;

    void AddOpts(NLastGetopt::TOpts& opts,
                 bool bothFilesAreRequired)
    {
        if (bothFilesAreRequired) {
            opts.AddLongOption("text-docs", "text documents file name")
                .Required()
                .StoreResult(&TextDocsFileName);
            opts.AddLongOption("bin-docs", "binary documents file name")
                .Required()
                .StoreResult(&BinDocsFileName);
        } else {
            opts.AddLongOption("text-docs", "text documents file name")
                .Optional()
                .StoreResult(&TextDocsFileName);
            opts.AddLongOption("bin-docs", "binary documents file name")
                .Optional()
                .StoreResult(&BinDocsFileName);
        }

        opts.AddLongOption("has-id", "first column is id")
            .Optional()
            .NoArgument()
            .StoreValue(&HasIdColumn, true);
        opts.AddLongOption("has-weight", "column after id contains weight")
            .Optional()
            .NoArgument()
            .StoreValue(&HasWeight, true);
    }

    void VerifyBothDocumentFileNamesExistance() {
        Y_VERIFY(BinDocsFileName || TextDocsFileName, "Provide at least one documents file (--bin-docs or --text-docs)");
    }
};

template <typename TBinaryClassificationModel>
void PrintMetrics(const TBinaryClassificationModel& model,
                  const TTextDocumentsReadingOptions& textDocumentsReadingOptions,
                  const TString& targetLabel,
                  bool printJsonMetrics = false)
{
    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(
        textDocumentsReadingOptions,
        *(model.GetDocumentFactory()),
        targetLabel);
    PrintMetrics(model, documents, printJsonMetrics);
}


template <typename TBinaryClassificationModel>
void ApplyModel(const TBinaryClassificationModel& model,
                const TTextDocumentsReadingOptions& textDocumentsReadingOptions,
                const TString& targetLabel,
                bool textInput,
                const TApplyOptions& applyOptions)
{
    if (textInput) {
        TString dataStr;
        while (Cin.ReadLine(dataStr)) {
            TStringBuf dataStrBuf(dataStr);

            TStringBuf text = dataStrBuf.NextTok('\t');
            if (textDocumentsReadingOptions.HasIdColumn) {
                text = dataStrBuf.NextTok('\t');
            }
            if (textDocumentsReadingOptions.HasWeight) {
                text = dataStrBuf.NextTok('\t');
            }

            TBinaryLabelWithFloatPrediction prediction = model.Apply(text, &applyOptions);
            Cout << text << "\t" << prediction.Prediction;
            if (targetLabel) {
                Cout << "\t" << (prediction.Label == EBinaryClassLabel::BCL_POSITIVE ? targetLabel : "");
            }
            Cout << "\n";
        }
        return;
    }

    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(
            textDocumentsReadingOptions,
            *(model.GetDocumentFactory()),
            targetLabel);

    for (const TBinaryLabelDocument& document : documents) {
        if (document.Id) {
            Cout << document.Id << "\t";
        }
        TBinaryLabelWithFloatPrediction prediction = model.Apply(document, &applyOptions);
        Cout << Sprintf("%.5lf", prediction.Prediction);
        if (targetLabel) {
            Cout << "\t" << (prediction.Label == EBinaryClassLabel::BCL_POSITIVE ? targetLabel : "");
        }
        Cout << Endl;
    }
}

template <typename TItems>
double Accuracy(const TMultiTextClassifierModel& model, const TItems& items) {
    size_t rightAnswersCount = 0;
    for (auto&& item : items) {
        rightAnswersCount += IsIn(item.Labels, model.BestPrediction(item).Label);
    }
    return rightAnswersCount ? (double) rightAnswersCount / items.size() : 0.;
}

TMultiLabelDocuments ReadMultiLabelTextDocuments(
        const TTextDocumentsReadingOptions& readingOptions,
        const TDocumentFactory& documentFactory,
        TVector<TString>* labelSet)
{
    TAutoPtr<IInputStream> input = OpenInput(readingOptions.TextDocsFileName);

    return ReadMultiLabelTextFromStream(*input, documentFactory, labelSet, readingOptions.HasIdColumn, readingOptions.HasWeight);
}

TBinaryLabelDocuments ReadBinaryLabelTextDocuments(
        const TTextDocumentsReadingOptions& readingOptions,
        const TDocumentFactory& documentFactory,
        const TString& targetLabel)
{
    TAutoPtr<IInputStream> input = OpenInput(readingOptions.TextDocsFileName);

    return ReadBinaryLabelTextFromStream(*input, documentFactory, targetLabel, readingOptions.HasIdColumn, readingOptions.HasWeight);
}

TBinaryLabelDocuments ReadBinOrTextBinaryLabelDocuments(
        const TTextDocumentsReadingOptions& textDocumentReadingOptions,
        const NEthos::TDocumentFactory& documentsFactory,
        const TString& targetLabel)
{
    TBinaryLabelDocuments result;
    if (!!textDocumentReadingOptions.BinDocsFileName) {
        result = ReadBinaryLabelHashesFromFile(textDocumentReadingOptions.BinDocsFileName, targetLabel);
    } else {
        result = ReadBinaryLabelTextDocuments(textDocumentReadingOptions,
                 documentsFactory,
                 targetLabel);
    }
    return result;
}

TBinaryLabelDocuments ReadBinOrTextBinaryLabelDocuments(
        const TTextDocumentsReadingOptions& textDocumentReadingOptions,
        const TTextClassifierOptions& textClassifierOptions)
{
    TBinaryLabelDocuments result = ReadBinOrTextBinaryLabelDocuments(textDocumentReadingOptions, *textClassifierOptions.ModelOptions.LemmerOptions.DocumentFactory, textClassifierOptions.TargetLabel);
    return result;
}

TMultiLabelDocuments ReadBinOrTextMultiLabelDocuments(
        const TTextDocumentsReadingOptions& textDocumentReadingOptions,
        const TDocumentFactory& documentFactory,
        TVector<TString>* labelSet)
{
    if (!!textDocumentReadingOptions.BinDocsFileName) {
        return ReadMultiLabelHashesFromFile(textDocumentReadingOptions.BinDocsFileName, labelSet);
    }
    return ReadMultiLabelTextDocuments(textDocumentReadingOptions, documentFactory, labelSet);
}

int DoSerialize(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentReadingOptions;
    NEthos::TLemmerOptions lemmerOptions;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentReadingOptions.AddOpts(opts, true);
        lemmerOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TMultiLabelDocuments documents = ReadMultiLabelTextDocuments(
            textDocumentReadingOptions, *lemmerOptions.DocumentFactory, nullptr);
    TFixedBufferFileOutput binDocsOut(textDocumentReadingOptions.BinDocsFileName);
    Save(&binDocsOut, documents);

    return 0;
}

int DoLearn(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    NEthos::TTextClassifierOptions textClassifierOptions;

    TString modelFileName;

    bool printJsonMetrics = false;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("model", "model file name")
            .Optional()
            .StoreResult(&modelFileName);

        opts.AddLongOption("json", "print metrics in json")
            .Optional()
            .NoArgument()
            .StoreValue(&printJsonMetrics, true);

        textClassifierOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    if (textClassifierOptions.TargetLabel) {
        TBinaryLabelFloatFeatureVectors learnItems;
        {
            TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(textDocumentsReadingOptions, textClassifierOptions);
            learnItems = FeatureVectorsFromBinaryLabelDocuments(documents.begin(), documents.end(), textClassifierOptions.ModelOptions);
        }

        TBinaryTextClassifierLearner classifierLearner(std::move(textClassifierOptions));
        TBinaryTextClassifierModel model = classifierLearner.Learn(learnItems.begin(), learnItems.end());

        model.MinimizeBeforeSaving();

        PrintMetrics(model, learnItems, printJsonMetrics);

        if (modelFileName) {
            model.SaveToFile(modelFileName);
            Cout << "Saved model version: " << model.Version << Endl;
        }
    } else {
        TVector<TString> allLabels;

        TMultiLabelDocuments documents = ReadBinOrTextMultiLabelDocuments(
                textDocumentsReadingOptions,
                *textClassifierOptions.ModelOptions.LemmerOptions.DocumentFactory,
                &allLabels);

        TMultiTextClassifierLearner classifierLearner(std::move(textClassifierOptions));
        TMultiTextClassifierModel model = classifierLearner.Learn<TMultiLabelDocument>(
                documents.begin(), documents.end(), allLabels);

        model.MinimizeBeforeSaving();

        Cout << "Accuracy: " << Accuracy(model, documents) << "\n";

        if (modelFileName) {
            model.SaveToFile(modelFileName);
            Cout << "Saved model version: " << model.Version << Endl;
        }
    }

    return 0;
}

int DoTest(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    TString modelFileName;

    TString featuresLearnerModelPath;
    TString predictorPath;

    TString targetLabel;

    bool printJsonMetrics = false;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("model", "model file name")
            .Optional()
            .StoreResult(&modelFileName);

        opts.AddLongOption("features-maker", "features maker index path")
            .Optional()
            .StoreResult(&featuresLearnerModelPath);
        opts.AddLongOption("predictor", "predictor path")
            .Optional()
            .StoreResult(&predictorPath);

        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Optional()
            .StoreResult(&targetLabel);

        opts.AddLongOption("json", "print metrics in json")
            .Optional()
            .NoArgument()
            .StoreValue(&printJsonMetrics, true);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    if (targetLabel) {
        if (modelFileName) {
            TBinaryTextClassifierModel model;
            model.LoadFromFile(modelFileName);

            Cerr << "Loaded model version: " << model.Version << Endl;

            PrintMetrics(model, textDocumentsReadingOptions, targetLabel, printJsonMetrics);
        } else if (featuresLearnerModelPath && predictorPath) {
            TCombinedTextClassifierModel model(featuresLearnerModelPath, predictorPath);
            PrintMetrics(model, textDocumentsReadingOptions, targetLabel, printJsonMetrics);
        } else {
            Cerr << "choose model file path(s)!\n";
            return 1;
        }
    } else {
        TMultiTextClassifierModel model;
        model.LoadFromFile(modelFileName);

        Cerr << "Loaded model version: " << model.Version << Endl;

        TVector<TString> allLabels;

        TMultiLabelDocuments documents = ReadBinOrTextMultiLabelDocuments(
                textDocumentsReadingOptions,
                *(model.GetDocumentFactory()),
                &allLabels);

        Cout << "Accuracy: " << Accuracy(model, documents) << "\n";
    }

    return 0;
}

int DoCrossValidate(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    NEthos::TTextClassifierOptions textClassifierOptions;

    NRegTree::TOptions regTreeOptions;
    regTreeOptions.OutputDetailedTreeInfo = false;

    bool printJsonMetrics = false;
    bool useCompositionModel = false;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("json", "print metrics in json")
            .Optional()
            .NoArgument()
            .StoreValue(&printJsonMetrics, true);

        opts.AddLongOption("composition", "use composition model")
            .Optional()
            .NoArgument()
            .StoreValue(&useCompositionModel, true);

        textClassifierOptions.AddOpts(opts);
        regTreeOptions.AddOpts(opts, true);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    if (textClassifierOptions.TargetLabel) {
        TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(textDocumentsReadingOptions, textClassifierOptions);
        THashMap<ui32, TString> hosts;
        if (textDocumentsReadingOptions.HasIdColumn) {
            hosts = HostsFromIdsAsUrls(documents);
        }

        TBinaryClassificationQFoldCV cv;
        std::pair<TBcMetrics, TBcMetrics> metrics;

        auto testCombinedLearner = [&](){
            TCombinedTextClassifierLearner learner(std::move(textClassifierOptions), regTreeOptions);

            if (textDocumentsReadingOptions.HasIdColumn) {
                return cv.CrossValidateByGroup(learner, documents, regTreeOptions.CVFoldsCount, hosts, regTreeOptions.ThreadsCount);
            }
            return cv.CrossValidateByIndex(learner, documents, regTreeOptions.CVFoldsCount, regTreeOptions.ThreadsCount);
        };

        auto testLinearLearner = [&](){
            TBinaryTextClassifierLearner learner(std::move(textClassifierOptions));

            TBinaryLabelFloatFeatureVectors learnItems = FeatureVectorsFromBinaryLabelDocuments(documents.begin(), documents.end(), learner.ModelOptions);
            if (textDocumentsReadingOptions.HasIdColumn) {
                return cv.CrossValidateByGroup(learner, learnItems, regTreeOptions.CVFoldsCount, hosts, regTreeOptions.ThreadsCount);
            }
            return cv.CrossValidateByIndex(learner, learnItems, regTreeOptions.CVFoldsCount, regTreeOptions.ThreadsCount);
        };

        if (useCompositionModel) {
            metrics = testCombinedLearner();
        } else {
            metrics = testLinearLearner();
        }

        Cout << "Average learn metrics:" << Endl;
        PrintMetrics(metrics.first, printJsonMetrics);
        Cout << Endl << "Average test metrics:" << Endl;
        PrintMetrics(metrics.second, printJsonMetrics);

    } else {
        ythrow yexception() << "Multi-label cross-validation not implemented yet";
    }

    return 0;
}

int DoApply(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    TString modelFileName;
    TString targetLabel;

    TString featuresLearnerModelPath;
    TString predictorPath;

    bool textInput = false;

    TApplyOptions applyOptions;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("model", "model file name")
            .Optional()
            .StoreResult(&modelFileName);

        opts.AddLongOption("features-maker", "features maker index path")
            .Optional()
            .StoreResult(&featuresLearnerModelPath);
        opts.AddLongOption("predictor", "predictor path")
            .Optional()
            .StoreResult(&predictorPath);

        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Optional()
            .StoreResult(&targetLabel);
        opts.AddLongOption("text", "input is just simple text")
            .Optional()
            .NoArgument()
            .StoreValue(&textInput, true);

        applyOptions.AddOpts(opts);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    if (targetLabel) {
        if (modelFileName) {
            TBinaryTextClassifierModel model;
            model.LoadFromFile(modelFileName);

            Cerr << "Loaded model version: " << model.Version << Endl;

            ApplyModel(model, textDocumentsReadingOptions, targetLabel, textInput, applyOptions);
        } else if (featuresLearnerModelPath && predictorPath) {
            TCombinedTextClassifierModel model(featuresLearnerModelPath, predictorPath);
            ApplyModel(model, textDocumentsReadingOptions, targetLabel, textInput, applyOptions);
        } else {
            Cerr << "choose model file path(s)!\n";
            return 1;
        }
    } else {
        TMultiTextClassifierModel model;
        model.LoadFromFile(modelFileName);

        Cerr << "Loaded model version: " << model.Version << Endl;

        TMultiLabelDocuments documents = ReadBinOrTextMultiLabelDocuments(
                textDocumentsReadingOptions,
                *(model.GetDocumentFactory()),
                nullptr);

        for (const TMultiLabelDocument& document : documents) {
            TVector<TStringLabelWithFloatPrediction> predictions =
                    model.Apply(document).PopTopNPositivePredictions(100);

            for (const auto& prediction : predictions) {
                Cout << prediction.Label << ": " << Sprintf("%.5lf", prediction.Prediction) << ", ";
            }

            Cout << Endl;
        }

    }

    return 0;
}

int DoApplyStorage(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    TString storagePath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("storage", "multi-language classification models storage path")
            .Required()
            .StoreResult(&storagePath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TBinaryTextClassifierModelsStorage storage;
    storage.LoadFromFile(storagePath);

    const TVector<ELanguage>& languages = storage.GetLanguages();

    TString text;
    while (Cin.ReadLine(text)) {
        if (!text) {
            continue;
        }

        for (const ELanguage language : languages) {
            Cout << NameByLanguage(language) << ": " << storage.Apply(language, text).Prediction << Endl;
        }
        Cout << Endl;
    }

    return 0;
}

int DoPrint(int argc, const char** argv) {
    TString modelFileName;
    bool shortExport = false;
    bool exportThreshold = false;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("model", "model file name")
            .Required()
            .StoreResult(&modelFileName);
        opts.AddLongOption("short", "short output mode")
            .Optional()
            .NoArgument()
            .StoreValue(&shortExport, true);
        opts.AddLongOption("threshold", "output model threshold")
            .Optional()
            .NoArgument()
            .StoreValue(&exportThreshold, true);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TBinaryTextClassifierModel model;
    model.LoadFromFile(modelFileName);

    Cerr << "Loaded model version: " << model.Version << Endl;

    if (exportThreshold) {
        Cout << model.GetThreshold() << "\n";
    }
    model.PrintDictionary(Cout, shortExport);

    return 0;
}

int DoPack(int argc, const char** argv) {
    TVector<TString> languageWithModelStrings;
    TString storagePath;
    TString defaultModelPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("models", "languages with models (lang:modelPath,lang:modelPath...)")
            .Required()
            .SplitHandler(&languageWithModelStrings, ',');
        opts.AddLongOption("storage", "resulting storage path")
            .Required()
            .StoreResult(&storagePath);
        opts.AddLongOption("default-model", "default model path")
            .Required()
            .StoreResult(&defaultModelPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TVector<std::pair<TString, TString> > languageWithModels;
    for (const TString& languageWithModelString : languageWithModelStrings) {
        TStringBuf languageWithModelStringBuf(languageWithModelString);
        TStringBuf language, modelPath;
        languageWithModelStringBuf.Split(':', language, modelPath);
        if (!language || !modelPath) {
            Cerr << "wrong format: " << languageWithModelString << Endl;
            return 1;
        }

        languageWithModels.push_back(std::make_pair(ToString(language), ToString(modelPath)));
    }

    TBinaryTextClassifierModelsStorage modelsStorage(languageWithModels, defaultModelPath);
    modelsStorage.SaveToFile(storagePath);

    return 0;
}

int DoAppendFeatures(int argc, const char** argv) {
    TString languageName;
    TVector<TString> modelStoragePaths;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("storages", "language model storage paths)")
            .Required()
            .SplitHandler(&modelStoragePaths, ',');
        opts.AddLongOption("language", "language to use")
            .Required()
            .StoreResult(&languageName);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    ELanguage language = LanguageByName(languageName);

    TVector<TBinaryTextClassifierModelsStorage> storages(modelStoragePaths.size());
    for (size_t storageNumber = 0; storageNumber < modelStoragePaths.size(); ++storageNumber) {
        storages[storageNumber].LoadFromFile(modelStoragePaths[storageNumber]);
    }

    TString dataStr;
    while (Cin.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);

        dataStrBuf.NextTok('\t'); // skip query id
        dataStrBuf.NextTok('\t'); // skip goal

        const TString query = ToString(dataStrBuf.NextTok('\t'));

        TVector<double> predictions;
        for (const TBinaryTextClassifierModelsStorage& modelsStorage : storages) {
            predictions.push_back(modelsStorage.Apply(language, query).Prediction);
        }

        Cout << dataStr << "\t" << JoinSeq("\t", predictions) << "\n";
    }

    return 0;
}

template <typename TModelType>
void ResetThresholds(const TVector<TString>& modelPaths) {
    for (const TString& modelPath : modelPaths) {
        TModelType model;
        model.LoadFromFile(modelPath);
        model.ResetThreshold();

        TString tmpPath = modelPath + ".no.intercept";

        model.SaveToFile(tmpPath);
        NFs::Rename(tmpPath, modelPath);
    }
}

int DoResetThresholds(int argc, const char** argv) {
    TVector<TString> storagePaths;
    TVector<TString> modelPaths;
    TVector<TString> multiModelPaths;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("storages", "storage paths")
            .Optional()
            .SplitHandler(&storagePaths, ',');
        opts.AddLongOption("models", "model paths")
            .Optional()
            .SplitHandler(&modelPaths, ',');
        opts.AddLongOption("multi-models", "multiclassification model paths")
            .Optional()
            .SplitHandler(&modelPaths, ',');

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    ResetThresholds<TBinaryTextClassifierModelsStorage>(storagePaths);
    ResetThresholds<TBinaryTextClassifierModel>(modelPaths);
    ResetThresholds<TMultiTextClassifierModel>(multiModelPaths);

    return 0;
}

int DoLearnCombinedModel(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    NEthos::TTextClassifierOptions textClassifierOptions;
    NRegTree::TOptions regTreeOptions;

    TString featuresMakerIndexPath;
    TString predictorPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);
        textClassifierOptions.AddOpts(opts);
        regTreeOptions.AddOpts(opts, true);

        opts.AddLongOption("features-maker", "resulting features maker index path")
            .Required()
            .StoreResult(&featuresMakerIndexPath);
        opts.AddLongOption("predictor", "resulting predictor formula path")
            .Required()
            .StoreResult(&predictorPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(textDocumentsReadingOptions, textClassifierOptions);

    TCombinedTextClassifierLearner learner(textClassifierOptions, regTreeOptions);
    TCombinedTextClassifierLearner::TModel model = learner.LearnAndSave(documents.begin(),
                                                                        documents.end(),
                                                                        featuresMakerIndexPath,
                                                                        predictorPath);

    PrintMetrics(model, documents);

    return 0;
}

int DoMakePool(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    NEthos::TTextClassifierOptions textClassifierOptions;
    size_t threadsCount = 5;

    TString featuresMakerIndexPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);
        textClassifierOptions.AddOpts(opts);

        opts.AddLongOption("features-maker", "resulting features maker index path")
            .Required()
            .StoreResult(&featuresMakerIndexPath);

        opts.AddLongOption("threads", "learn threads count")
            .Optional()
            .StoreResult(&threadsCount);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(textDocumentsReadingOptions, textClassifierOptions);

    TTextClassifierFeaturesMaker featuresMaker;
    featuresMaker.LoadFromFile(featuresMakerIndexPath);

    for (const TBinaryLabelDocument& document : documents) {
        const TString& id = document.Id;
        NRegTree::TRegressionModel::TInstanceType instance = featuresMaker.ToInstance(document);
        Cout << instance.ToFeaturesString(id) << "\n";
    }

    return 0;
}

int DoLearnPool(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    NEthos::TTextClassifierOptions textClassifierOptions;
    size_t threadsCount = 5;

    TString featuresMakerIndexPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);
        textClassifierOptions.AddOpts(opts);

        opts.AddLongOption("features-maker", "resulting features maker index path")
            .Required()
            .StoreResult(&featuresMakerIndexPath);

        opts.AddLongOption("threads", "learn threads count")
            .Optional()
            .StoreResult(&threadsCount);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(textDocumentsReadingOptions, textClassifierOptions);

    TTextClassifierFeaturesMaker featuresMaker;
    NRegTree::TRegressionModel::TPoolType pool = featuresMaker.Learn(textClassifierOptions,
                                                                     documents.begin(),
                                                                     documents.end(),
                                                                     threadsCount);

    featuresMaker.SaveToFile(featuresMakerIndexPath);

    for (size_t documentNumber = 0; documentNumber < documents.size(); ++documentNumber) {
        const TString id = documents[documentNumber].Id;
        const NRegTree::TRegressionModel::TInstanceType& instance = pool[documentNumber];
        Cout << instance.ToFeaturesString(id) << "\n";
    }

    return 0;
}

int StoreCombinedModel(int argc, const char** argv) {
    TLinearBinaryClassifierModel linearModel;
    TString linearModelPath;
    TString compositeModelPath;

    size_t binsCount = 50;
    size_t topWordsCount = 100;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("linear-model", "input linear model path")
            .Required()
            .StoreResult(&linearModelPath);
        opts.AddLongOption("composite-model", "output composite model path")
            .Required()
            .StoreResult(&compositeModelPath);

        opts.AddLongOption("bins-count", "number of feature bins")
            .Optional()
            .StoreResult(&binsCount);
        opts.AddLongOption("top-words", "number of tofeature bins")
            .Optional()
            .StoreResult(&topWordsCount);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    {
        TFileInput linearModelIn(linearModelPath);
        int version;
        Load(&linearModelIn, version);
        linearModel.Load(&linearModelIn);
    }

    TNewCombinedTextClassifierModel newModel;
    newModel.Init(linearModel.GetModelOptions().LemmerOptions, linearModel.GetWeights(), binsCount, topWordsCount);

    {
        TFileOutput compositeModelOut(compositeModelPath);
        newModel.Save(&compositeModelOut);
    }

    return 0;
}

int DumpCombinedPool(int argc, const char** argv) {
    TTextDocumentsReadingOptions textDocumentsReadingOptions;
    TString compositeModelPath;
    TString targetLabel;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        textDocumentsReadingOptions.AddOpts(opts, false);

        opts.AddLongOption("model", "model file name")
            .Required()
            .StoreResult(&compositeModelPath);
        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Required()
            .StoreResult(&targetLabel);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TNewCombinedTextClassifierModel newModel;
    {
        TFileInput compositeModelIn(compositeModelPath);
        newModel.Load(&compositeModelIn);
    }

    TBinaryLabelDocuments documents = ReadBinOrTextBinaryLabelDocuments(
        textDocumentsReadingOptions,
        newModel.GetDocumentsFactory(),
        targetLabel);

    for (size_t documentIdx = 0; documentIdx < documents.size(); ++documentIdx) {
        const TBinaryLabelDocument& document = documents[documentIdx];
        if (document.Label == EBinaryClassLabel::BCL_UNKNOWN) {
            continue;
        }

        Cout << documentIdx << "\t"
             << (document.Label == EBinaryClassLabel::BCL_POSITIVE) << "\t"
             << (document.Id ? document.Id : "") << "\t"
             << 1 << "\t"
             << JoinSeq("\t", newModel.CreateFeatures(document)) << Endl;
    }

    return 0;
}

int ApplyNewCombinedModel(int argc, const char** argv) {
    TString compositeModelPath;
    TString matrixnetModelPath;

    TString targetLabel;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("composite-model", "text model file name")
            .Required()
            .StoreResult(&compositeModelPath);
        opts.AddLongOption("matrixnet-model", "matrixnet model file name")
            .Required()
            .StoreResult(&matrixnetModelPath);
        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Required()
            .StoreResult(&targetLabel);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    TNewCombinedTextClassifierModel newModel;
    newModel.Load(compositeModelPath, matrixnetModelPath);

    TString dataStr;
    while (Cin.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);
        const TString text = TString{dataStrBuf.NextTok('\t')};
        TBinaryLabelWithFloatPrediction prediction = newModel.Apply(text);
        Cout << text << "\t" << prediction.Prediction << "\t" << (prediction.Label == EBinaryClassLabel::BCL_POSITIVE ? targetLabel : "") << "\n";
    }

    return 0;
}

int SetupNewCompositeModelOffset(int argc, const char** argv) {
    TString compositeModelPath;

    double offset = 0.;
    TString offsetPath;

    {
        NLastGetopt::TOpts opts;
        opts.SetFreeArgsMax(0);

        opts.AddLongOption("composite-model", "text model file name")
            .Required()
            .StoreResult(&compositeModelPath);
        opts.AddLongOption("offset", "offset to set")
            .Optional()
            .StoreResult(&offset);
        opts.AddLongOption("offset-file", "file with resulting offset")
            .Optional()
            .StoreResult(&offsetPath);

        NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    }

    if (offsetPath) {
        TFileInput offsetIn(offsetPath);
        offsetIn >> offset;
    }

    TNewCombinedTextClassifierModel newModel;
    newModel.LoadWithoutMatrixnet(compositeModelPath);
    newModel.SetupOffset(offset);

    {
        TFileOutput compositeModelOut(compositeModelPath);
        newModel.Save(&compositeModelOut);
    }

    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode("serialize", DoSerialize, "serialize text documents to binary format");
    modChooser.AddMode("learn", DoLearn, "learn text classification model");
    modChooser.AddMode("test", DoTest, "calculate metrics for text classification model");
    modChooser.AddMode("cv", DoCrossValidate, "do cross-validation");
    modChooser.AddMode("apply", DoApply, "apply text classification model");
    modChooser.AddMode("apply-storage", DoApplyStorage, "apply models from multi-language storage");
    modChooser.AddMode("print", DoPrint, "print unigram model as a dictionary (for lemmas_merger-based models only!)");
    modChooser.AddMode("pack", DoPack, "make multi-language models storage");
    modChooser.AddMode("learn-pool", DoLearnPool, "make matrixnet learning pool and text classification models");
    modChooser.AddMode("make-pool", DoMakePool, "make matrixnet learning pool and text classification models");
    modChooser.AddMode("learn-composition", DoLearnCombinedModel, "learn composition model");
    modChooser.AddMode("append-features", DoAppendFeatures, "append text features to matrixnet pool (assuming 3rd field to contain the query)");
    modChooser.AddMode("reset-thresholds", DoResetThresholds, "set model thresholds to zero");

    modChooser.AddMode("new-combined-model", StoreCombinedModel, "convert model to composite format");
    modChooser.AddMode("new-combined-pool", DumpCombinedPool, "produce new composite model learning pool");
    modChooser.AddMode("new-combined-apply", ApplyNewCombinedModel, "set model thresholds to zero");
    modChooser.AddMode("new-combined-offset", SetupNewCompositeModelOffset, "setup offset for the new brand cool model");

    return modChooser.Run(argc, argv);
}
