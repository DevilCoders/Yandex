#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_model.h>
#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/generic/hash.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/generic/ymath.h>

using namespace NVowpalWabbit;

class TLoggingCallback: public ICallbacks {
public:
    using TNgram2HashVal = THashMap<std::pair<size_t, size_t>, ui32>;

private:
    TNgram2HashVal Ngram2HashVal;

public:
    void OnCalcedHash(size_t idxSt, size_t idxEnd, ui32 hashVal) override {
        Ngram2HashVal[std::make_pair(idxSt, idxEnd)] = hashVal;
    }

    void OnCalcHashesStart() override {
        Ngram2HashVal.clear();
    }

    const TNgram2HashVal& GetNgram2HashVal() const {
        return Ngram2HashVal;
    }
};

struct TNgramWithWeight {
    size_t St;
    size_t End;
    float Wt;

    TNgramWithWeight(size_t st, size_t end_, float wt)
        : St(st)
        , End(end_)
        , Wt(wt)
    {
    }

    bool operator==(const TNgramWithWeight& rhs) const {
        return FuzzyEquals(Wt, rhs.Wt);
    }

    // sort descending by default
    bool operator<(const TNgramWithWeight& rhs) const {
        return Wt > rhs.Wt;
    }
};

template <>
void Out<TNgramWithWeight>(IOutputStream& os, const TNgramWithWeight& ngramWithWeight) {
    os << ngramWithWeight.St << '\t' << ngramWithWeight.End << '\t' << ngramWithWeight.Wt;
}

// features start at the 4th position of a vw-formatted pool
// https://github.com/JohnLangford/vowpal_wabbit/wiki/Input-format
static const size_t FeaturesStIdx = 4;

void PrintFeatures(const TString& pool, const TString& binModel, const TString& ns, size_t ngrams, const TStringBuf sep) {
    NVowpalWabbit::TModel model(TBlob::FromFile(binModel));
    TVowpalWabbitPredictor predictor(model);
    TLoggingCallback loggingCallback;
    const auto& ngram2HashValHandle = loggingCallback.GetNgram2HashVal();

    TString sample;
    TVector<TStringBuf> tokens;
    TVector<TNgramWithWeight> ngramsWithWeights;
    TFileInput inp(pool);

    while (inp.ReadLine(sample)) {
        Split(sample, " ", tokens);

        Cerr << sample << Endl
             << predictor.PredictWithCallbacks(ns, tokens, ngrams, &loggingCallback) << Endl;

        for (size_t i = FeaturesStIdx; i < tokens.size(); ++i) {
            for (size_t j = i + 1; j < Min(i + ngrams, tokens.size()) + 1; ++j) {
                if (const auto* hashVal = ngram2HashValHandle.FindPtr(std::make_pair(i, j))) {
                    float weight = model[*hashVal];
                    if (!FuzzyEquals(1.0f + weight, 1.0f))
                        ngramsWithWeights.emplace_back(i, j, weight);
                }
            }
        }
        std::sort(begin(ngramsWithWeights), end(ngramsWithWeights));

        for (const auto& ngramWithWeight : ngramsWithWeights) {
            Cerr << ngramWithWeight << '\t'
                 << JoinRange(sep, tokens.begin() + ngramWithWeight.St, tokens.begin() + ngramWithWeight.End) << Endl;
        }
        Cerr << "constant " << predictor.GetConstPrediction() << Endl;
    }
}

int ConvertReadableModel2ArcBin(int argc, const char** argv) {
    TString readable;
    TString arcBin;

    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    opts.AddCharOption('r', "readable").RequiredArgument("READABLE").Required().StoreResult(&readable);
    opts.AddCharOption('b', "arcbin").RequiredArgument("ARCBIN").Required().StoreResult(&arcBin);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    TModel::ConvertReadableModel(readable, arcBin);
    return 0;
}

int MergeTwoArcBinModels(int argc, const char** argv) {
    TString arcBin1;
    TString arcBin2;
    TString arcBinResult;

    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    opts.AddCharOption('1', "arcbin1").RequiredArgument("ARCBIN1").Required().StoreResult(&arcBin1);
    opts.AddCharOption('2', "arcbin2").RequiredArgument("ARCBIN2").Required().StoreResult(&arcBin2);
    opts.AddCharOption('o', "out").RequiredArgument("OUTPUT").Required().StoreResult(&arcBinResult);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    const TModel model1(arcBin1);
    const TModel model2(arcBin2);

    Y_VERIFY(model1.GetBits() == model2.GetBits());
    const float* data1 = reinterpret_cast<const float*>(model1.GetBlob().Data());
    const float* data2 = reinterpret_cast<const float*>(model2.GetBlob().Data());
    const size_t dataSize = model1.GetBlob().Size() / sizeof(float);
    TFixedBufferFileOutput out(arcBinResult);
    for (size_t i = 0; i < dataSize; ++i) {
        const float merged = (data1[i] + data2[i]) / 2;
        out.Write(&merged, sizeof(merged));
    }

    return 0;
}

int PrintFeaturesBySample(int argc, const char** argv) {
    TString pool;
    TString binModel;
    TString ns;
    size_t ngrams;
    TString delim;

    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    opts.AddCharOption('p', "pool file").RequiredArgument("POOL").Required().StoreResult(&pool);
    opts.AddCharOption('m', "model file").RequiredArgument("MODEL").Required().StoreResult(&binModel);
    opts.AddCharOption('n', "namespace").RequiredArgument("NAMESPACE").Required().StoreResult(&ns);
    opts.AddLongOption("ngrams", "ngrams").RequiredArgument("NGRAMS").Required().StoreResult(&ngrams);
    opts.AddLongOption("delim", "delimiter").RequiredArgument("DELIM").Required().StoreResult(&delim);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    PrintFeatures(pool, binModel, ns, ngrams, delim);
    return 0;
}

int ApplyModel(int argc, const char** argv) {
    TString inputId;
    TString outputId;
    TString modelFile;
    TString ns;
    size_t ngrams;
    TString delim;

    auto opts = NLastGetopt::TOpts::Default();
    opts.SetFreeArgsNum(0);

    opts.AddCharOption('i', "input").RequiredArgument("INPUT").DefaultValue("-").StoreResult(&inputId).Help("see library/cpp/streams/factory's OpenInput for details (default is stdin)");
    opts.AddCharOption('o', "input").RequiredArgument("INPUT").DefaultValue("-").StoreResult(&outputId).Help("see library/cpp/streams/factory's OpenOutput for details (default is stdout)");
    opts.AddCharOption('m', "model file").RequiredArgument("MODEL").Required().StoreResult(&modelFile);
    opts.AddCharOption('n', "namespace").RequiredArgument("NAMESPACE").Required().StoreResult(&ns);
    opts.AddLongOption("ngrams", "ngrams").RequiredArgument("NGRAMS").Required().StoreResult(&ngrams);
    opts.AddLongOption("delim", "delimiter").RequiredArgument("DELIM").Required().StoreResult(&delim);

    NLastGetopt::TOptsParseResult parsedOpts(&opts, argc, argv);

    NVowpalWabbit::TModel model(TBlob::FromFile(modelFile));
    TVowpalWabbitPredictor predictor(model);

    TString line;
    TVector<TStringBuf> tokens;

    auto input = OpenInput(inputId);
    auto output = OpenOutput(outputId);

    while (input->ReadLine(line)) {
        Split(line.data(), delim.data(), tokens);
        *output << predictor.Predict(ns, tokens, ngrams) + predictor.GetConstPrediction() << '\n';
    }

    return 0;
}

int ConvertFmlPool2VwPool(int argc, const char** argv);
int PackVowpalWabbitModel(int argc, const char** argv);

static const char* const Modes[] = {"readable2arc_bin", "features_by_sample", "merge_two_arc_bin_models", "apply_model"};
static const TMainFunctionPtr MainFunctions[] = {ConvertReadableModel2ArcBin, PrintFeaturesBySample, MergeTwoArcBinModels, ApplyModel};
static_assert(Y_ARRAY_SIZE(Modes) == Y_ARRAY_SIZE(MainFunctions), "Modes and MainFunctions have different lengths");

int main(int argc, const char** argv) {
    TModChooser modChooser;
    for (ui32 i = 0; i < sizeof(Modes) / sizeof(Modes[0]); ++i) {
        modChooser.AddMode(Modes[i], MainFunctions[i], Modes[i]);
    }
    modChooser.AddMode("fml_pool2vw_pool", ConvertFmlPool2VwPool, "Convert pool in FML format to vowpal wabbit pool format for binary classification.");
    modChooser.AddMode("pack_vw_model",  PackVowpalWabbitModel, "Make packed VW model - convert all weights from float to i8.");
    return modChooser.Run(argc, argv);
}
