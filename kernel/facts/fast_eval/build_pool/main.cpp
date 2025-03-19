#include <kernel/facts/features_calculator/calculator_data.h>
#include <kernel/facts/common/normalize_text.h>
#include <kernel/facts/classifiers/online_alias_classifier.h>
#include <kernel/facts/factors_info/factors_gen.h>

#include <kernel/facts/common_features/query_model.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/tokenizer/split.h>

#include <util/charset/wide.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/join.h>
#include <util/thread/pool.h>

/**
 * run with full resources:
 * ./build_pool  -p ~/pool.tsv -o ~/new_pool.tsv -j 48 -f FI_WORD_DIST_URL_5_NORM_QUERY -f FI_WORD_DIST_URL_10_NORM_QUERY
 *
 *
 * run with minimum resources:
 * ./build_pool  -p ~/pool.tsv -o ~/new_pool.tsv -j 48 -c ./empty_resources.json  -d . -f FI_WORD_DIST_URL_5_NORM_QUERY -f FI_WORD_DIST_URL_10_NORM_QUERY
 *
 *
 */

using namespace NUnstructuredFeatures;

void BuildEvalFeatures(
        NUnstructuredFeatures::TQueryCalculator& lhs,
        NUnstructuredFeatures::TQueryCalculator& rhs,
        TStringBuf answer,
        const TUtf16String& answerNorm,
        TFactFactorStorage& features) {
    try {
        Y_UNUSED(lhs);
        Y_UNUSED(rhs);
        Y_UNUSED(answer);
        Y_UNUSED(answerNorm);
        Y_UNUSED(features);

        // YOUR CODE HERE
    } catch(yexception& ex) {
        Cerr << "Error during calculate factors: " << ex << Endl;
        _exit(1);
    }
}

const TString DEFAULT_PATH_TO_RESOURCES = "../../../../quality/functionality/facts/classifier_resources/online-alias-resources";

void ProcessPool(const TString& line,
                 TAtomic& processed,
                 TMutex& mutex,
                 const TCalculatorData& calculatorData,
                 const TVector<EFactorId>& featuresId,
                 TFileOutput& resultPool) {
    try {
        TVector<TStringBuf> parts = StringSplitter(line).Split('\t');
        NSc::TValue meta = NSc::TValue::FromJsonThrow(parts[2]);
        TStringBuf answer = meta["answer"].GetString();
        TString query = TString{meta["query"].GetString()};
        TString alias = TString{meta["alias"].GetString()};
        TUtf16String answerWide = NormalizeText(UTF8ToWide<true>(answer));

        TQueryCalculator lhs(calculatorData, query);
        TQueryCalculator rhs(calculatorData, alias);

        TFactFactorStorage features;

        BuildEvalFeatures(lhs, rhs, answer, answerWide, features);

        if ((AtomicIncrement(processed) % 20000) == 0) {
            (Cout << " processed: " << processed << '\r').Flush();
        }

        TVector<float> featuresToAdd;
        featuresToAdd.reserve(featuresId.size());
        for (auto feature: featuresId) {
            featuresToAdd.push_back(features[feature]);
        }

        TStringStream res;
        res << line;
        if (!featuresToAdd.empty()) {
            res << '\t' << JoinSeq("\t", featuresToAdd);
        }
        res << Endl;

        mutex.Acquire();
        resultPool << res.Str();
        mutex.Release();
    } catch(yexception& ex) {
        Cerr << "Error during parsing pool: " << ex << Endl;
        _exit(1);
    }
}


int main(int argc, char** argv) {
    TString factsDir;
    TString modelConf;
    NLastGetopt::TOpts opts;
    TString pool;
    size_t version;
    TString output;
    size_t threads;
    TVector<TString> factors;

    opts.SetTitle("Calculate distance between queries based on word distances");
    opts.AddCharOption('d', "Directory with runtime aliases resources")
            .DefaultValue(DEFAULT_PATH_TO_RESOURCES)
            .StoreResult(&factsDir);
    opts.AddCharOption('c', "Config file with paths to all resources")
            .DefaultValue("online_aliases.config")
            .StoreResult(&modelConf);
    opts.AddCharOption('g', "Version from config")
            .DefaultValue(NFactClassifiers::ONLINE_ALIASES_VERSION)
            .StoreResult(&version);
    opts.AddCharOption('p', "Baseline pool")
            .Required()
            .StoreResult(&pool);
    opts.AddCharOption('o', "Output pool")
            .Required()
            .StoreResult(&output);
    opts.AddCharOption('j', "Number of threads")
            .Required()
            .DefaultValue(4)
            .StoreResult(&threads);
    opts.AddCharOption('f', "Factors to add to the end of the pool")
            .Required()
            .AppendTo(&factors);
    (void)NLastGetopt::TOptsParseResult(&opts, argc, argv);

    TConfig config(factsDir, modelConf, version);
    Cout << "Loading resources..." << Endl;
    TCalculatorData calculatorData{ config };
    TFileInput f(pool);
    TFileOutput resultPool(output);
    TString line;

    auto threadPool = CreateThreadPool(threads, 10000, IThreadPool::TParams().SetBlocking(true).SetCatching(false));
    threadPool->Start(threads, 10000);

    TVector<EFactorId> featuresId;
    Cout << "Append factors:" << Endl;
    for (auto& factor : factors) {
        featuresId.push_back(FromString<EFactorId>(factor));
        Cout << "\t" << factor << Endl;
    }

    TAtomic processed(0);
    TMutex mutex;

    while(f.ReadLine(line)) {
        threadPool->Run([line, &processed, &calculatorData, &featuresId, &mutex, &resultPool] {
            ProcessPool(line, processed, mutex, calculatorData, featuresId, resultPool);

        });
    }
    threadPool->Stop();
    Cout << "\n" << "Done." << Endl;
}


