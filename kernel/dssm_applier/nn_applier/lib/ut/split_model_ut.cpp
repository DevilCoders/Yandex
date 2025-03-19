#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/folder/path.h>
#include <util/folder/dirut.h>

#include <util/stream/file.h>
#include <util/stream/str.h>

#include <util/memory/blob.h>

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>

#include <util/system/types.h>

using namespace NNeuralNetApplier;

namespace {

    using TFloats = TVector<float>;

    const TString MODEL_FILENAME{"dssm_sample.bin"};

    TFloats EvaluateModel(TModel& model, const TVector<TString>& inputNames,
                          const TString& outputName,
                          TString value = "something") {
        auto values = TVector<TString>{inputNames.size(), value};
        auto sample = MakeAtomicShared<TSample>(inputNames, values);
        auto outputNames = TVector<TString>{outputName};
        auto result = TVector<float>{};
        model.Apply(sample, outputNames, result);
        return result;
    }

} // unnamed namespace

Y_UNIT_TEST_SUITE(SplitDssmTest) {
    Y_UNIT_TEST(ExtractQueryModel) {
        auto model = TModel{};
        auto blob = TBlob{};
        {
            TFileInput inputFile(MODEL_FILENAME);
            blob = TBlob::FromStream(inputFile);
            model.Load(blob);
        }
        model.Init();

        auto queryOutput = TString{"query_embedding"};
        auto queryModel = model.GetSubmodel(queryOutput, {"input"});

        //snippet,region,url,query,title
        auto result = EvaluateModel(
            *queryModel, TVector<TString>{"region", "query", "url", "title", "snippet"},
            "query_embedding");
        UNIT_ASSERT(result.size() != 0);
    } // ExtractQueryModel
} // SplitDssmTest
