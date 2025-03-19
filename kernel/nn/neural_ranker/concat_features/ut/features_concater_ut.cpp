#include <kernel/nn/neural_ranker/concat_features/features_concater.h>
#include <library/cpp/testing/unittest/registar.h>


Y_UNIT_TEST_SUITE(ConcatFeaturesTestSuite) {
    NNeuralRankerProtocol::TInput GenerateInput() {
        NNeuralRankerProtocol::TInput input;
        auto inputParts = input.MutableInputParts();
        for (size_t i = 0; i < 3; ++i) {
            NNeuralRankerProtocol::TInputPart* inputPart = inputParts->Add();
            auto newSlice = inputPart->MutableSlice();
            newSlice->SetName("Slice_" + IntToString<10>(i));
            auto indices = newSlice->MutableIndices();
            for (auto j: {0, 2, 4}) {
                *(indices->Add()) = j;
            }
        }
        for (size_t i = 0; i < 4; ++i) {
            NNeuralRankerProtocol::TInputPart* inputPart = inputParts->Add();
            auto newEmbedding = inputPart->MutableEmbedding();
            newEmbedding->SetName("DSSM_" + IntToString<10>(i));
            newEmbedding->SetLen(100);
        }
        return input;
    }

    using TInputMap = NNeuralRanker::TFeaturesConcater::TInputMap;
    using TInputBatchMap = NNeuralRanker::TFeaturesConcater::TInputBatchMap;

    Y_UNIT_TEST(CorrectInit) {
        NNeuralRanker::TFeaturesConcater concater(GenerateInput());
    }

    Y_UNIT_TEST(IncorrectInit) {
        NNeuralRankerProtocol::TInput input = GenerateInput();
        auto inputParts = input.MutableInputParts();
        NNeuralRankerProtocol::TInputPart* inputPart = inputParts->Add();
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NNeuralRanker::TFeaturesConcater(input),
            yexception,
            "neither slice nor embedding is given"
        );
        auto newSlice = inputPart->MutableSlice();
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NNeuralRanker::TFeaturesConcater(input),
            yexception,
            "invalid slice given: "
        );
        newSlice->SetName("Slice_5");
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NNeuralRanker::TFeaturesConcater(input),
            yexception,
            "invalid slice given: "
        );
        *(newSlice->MutableIndices()->Add()) = 0;

        inputPart = inputParts->Add();
        auto newEmbedding = inputPart->MutableEmbedding();
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            NNeuralRanker::TFeaturesConcater(input),
            yexception,
            "invalid embedding given: "
        );
        newEmbedding->SetName("DSSM_10");
        newEmbedding->SetLen(200);
        NNeuralRanker::TFeaturesConcater concater(input);
    }

    Y_UNIT_TEST(CorrectConcatSample) {
        NNeuralRanker::TFeaturesConcater concater(GenerateInput());
        TVector<float> sliceVector(10, 1.);
        TVector<float> embVector(100, 1.);
        TInputMap map;
        for (size_t i = 0; i < 3; ++i) {
            map["Slice_" + IntToString<10>(i)] = sliceVector;
        }
        for (size_t i = 0; i < 4; ++i) {
            map["DSSM_" + IntToString<10>(i)] = embVector;
        }
        TVector<float> concated = concater.Concat(map);
        UNIT_ASSERT(concated.size() == 409);
        for (auto i: concated) {
            UNIT_ASSERT_DOUBLES_EQUAL(i, 1., 0.000001);
        }
    }

    Y_UNIT_TEST(CorrectConcatBatch) {
        TVector<float> sliceVector(10, 1.);
        TVector<float> embVector(100, 1.);
        TInputMap map;
        for (size_t i = 0; i < 3; ++i) {
            map["Slice_" + IntToString<10>(i)] = sliceVector;
        }
        for (size_t i = 0; i < 4; ++i) {
            map["DSSM_" + IntToString<10>(i)] = embVector;
        }

        TInputBatchMap batchMap;
        for (const auto& it: map) {
            batchMap[it.first] = TVector<TConstArrayRef<float>>{
                    it.second, it.second, it.second
            };
        }

        NNeuralRanker::TFeaturesConcater concater(GenerateInput());
        TVector<TVector<float>> concated = concater.Concat(batchMap);
        UNIT_ASSERT(concated.size() == 3);
        for (const auto& i: concated) {
            UNIT_ASSERT(i.size() == 409);
            for (float j: i) {
                UNIT_ASSERT_DOUBLES_EQUAL(j, 1., 0.000001);
            }
        }
    }

    Y_UNIT_TEST(IncorrectConcatBatch) {
        NNeuralRankerProtocol::TInput input = GenerateInput();
        NNeuralRanker::TFeaturesConcater concater(input);

        TInputMap map;
        TVector<float> sliceSample(10, 1.);
        TVector<float> embedddingSample(100, 1.);
        for (size_t i = 0; i < 3; ++i) {
            map["Slice_" + IntToString<10>(i)] = MakeArrayRef(sliceSample);
        }
        for (size_t i = 0; i < 4; ++i) {
            map["DSSM_" + IntToString<10>(i)] = MakeArrayRef(embedddingSample);
        }

        TInputBatchMap batchMap;
        for (const auto& it: map) {
            batchMap[it.first] = TVector<TConstArrayRef<float>>{
                it.second, it.second, it.second
            };
        }
        concater.Concat(batchMap);

        batchMap.erase("Slice_0");
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "slice \"Slice_0\" is not given"
        );
        TVector<float> sample(2, 1);
        batchMap["Slice_0"] = TVector<TConstArrayRef<float>>{sample};
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "batchsizes should be equal"
        );
        batchMap["Slice_0"] = TVector<TConstArrayRef<float>>{
            sample, sample, sample
        };
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "slice \"Slice_0\" should be longer than 4, but has size = 2"
        );
        sample.resize(10, 1);
        batchMap["Slice_0"] = TVector<TConstArrayRef<float>>{
            sample, sample, sample
        };

        concater.Concat(batchMap);

        batchMap.erase("DSSM_0");
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "embedding \"DSSM_0\" is not given"
        );

        TVector<float> sampleEmb(2, 1.);
        batchMap["DSSM_0"] = TVector<TConstArrayRef<float>>{sampleEmb};
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "batchsizes should be equal"
        );
        batchMap["DSSM_0"] = TVector<TConstArrayRef<float>>{
            sampleEmb, sampleEmb, sampleEmb
        };
        UNIT_ASSERT_EXCEPTION_CONTAINS(
            concater.Concat(batchMap),
            yexception,
            "embedding \"DSSM_0\" should be length 100, but has size = 2"
        );
        sampleEmb.resize(100, 1);
        batchMap["DSSM_0"] = TVector<TConstArrayRef<float>>{
            sampleEmb, sampleEmb, sampleEmb
        };
        concater.Concat(batchMap);
    }
};
