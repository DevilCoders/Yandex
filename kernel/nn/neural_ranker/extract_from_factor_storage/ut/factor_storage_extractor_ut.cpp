#include <kernel/nn/neural_ranker/extract_from_factor_storage/factor_storage_extractor.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/random/random.h>


Y_UNIT_TEST_SUITE(FactorStorageExtractorTestSuite) {
    Y_UNIT_TEST(VectorExtract) {
        int size = 123;
        TFactorStorage factorStorage(size);
        TVector<double> values;
        values.resize(size);
        for (int i = 0; i < size; ++i) {
            values[i] = RandomNumber<double>();
            factorStorage[i] = values[i];
        }

        TStringStream ss;
        NFSSaveLoad::Serialize(factorStorage, &ss);

        NFactorSlices::TSlicesMetaInfo hostInfo;
        THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, hostInfo);
        const auto& extracted = NNeuralRanker::ConvertToVector(loaded);

        UNIT_ASSERT(extracted.contains("Slice_web_production"));
        UNIT_ASSERT(extracted.size() == 1);
        const auto& data = extracted.at("Slice_web_production");

        for (size_t i = 0; i < values.size(); ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(values[i], data[i], 0.000001);
        }
    }

    Y_UNIT_TEST(RefExtract) {
        int size = 123;
        TFactorStorage factorStorage(size);
        TVector<double> values;
        values.resize(size);
        for (int i = 0; i < size; ++i) {
            values[i] = RandomNumber<double>();
            factorStorage[i] = values[i];
        }

        TStringStream ss;
        NFSSaveLoad::Serialize(factorStorage, &ss);

        NFactorSlices::TSlicesMetaInfo hostInfo;
        THolder<TFactorStorage> loaded = NFSSaveLoad::Deserialize(&ss, hostInfo);

        const auto& extracted = NNeuralRanker::ConvertToArrayRef(loaded);

        UNIT_ASSERT(extracted.contains("Slice_web_production"));
        UNIT_ASSERT(extracted.size() == 1);

        const auto& data = extracted.at("Slice_web_production");

        for (int i = 0; i < size; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL(values[i], data[i], 0.000001);
        }
    }
}
