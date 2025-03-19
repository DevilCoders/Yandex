#include <kernel/factor_storage/factor_storage.h>
#include <kernel/factors_selector/factors.h>

#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/testing/unittest/registar.h>

const TString WEB_PRODUCTION_CONFIG = R"(
Slice: "web_production"
Factors: [
    {Index: 10},
    {Index: 3, MinValue: 0.5},
    {Index: 15, ConstValue: 0.87}
]
)";

const TString RAPID_CLICKS_CONFIG = R"(
Slice: "rapid_clicks"
Factors: [
    {Index: 25},
    {Index: 37, MaxValue: 0.15},
    {Index: 18}
]
)";

const TString RAPID_PERS_CLICKS_CONFIG = R"(
Slice: "rapid_pers_clicks"
Factors: [
    {Index: 24, MaxValue: 0.8},
    {Index: 0}
]
)";

Y_UNIT_TEST_SUITE(TSelectFeaturesFromStorageTests) {
    Y_UNIT_TEST(SimpleTest) {
        TFactorDomain domain{EFactorSlice::WEB_PRODUCTION, EFactorSlice::RAPID_CLICKS, EFactorSlice::RAPID_PERS_CLICKS};
        TFactorStorage storage{domain};
        storage.Clear();

        auto webProductionSlice = storage.CreateViewFor(EFactorSlice::WEB_PRODUCTION);
        webProductionSlice[3] = 0.4;
        webProductionSlice[10] = 0.83;
        webProductionSlice[100] = -0.25;

        auto rapidClicksSlice = storage.CreateViewFor(EFactorSlice::RAPID_CLICKS);
        rapidClicksSlice[18] = 0.3;
        rapidClicksSlice[25] = -100;
        rapidClicksSlice[37] = 0.17344;

        auto rapidPersClicksSlice = storage.CreateViewFor(EFactorSlice::RAPID_PERS_CLICKS);
        rapidPersClicksSlice[0] = 0.182;
        rapidPersClicksSlice[24] = 1000.0;
        rapidPersClicksSlice[80] = -0.85;

        NNeuralNet::TFeaturesConfig config;
        for (const auto& configStr : {WEB_PRODUCTION_CONFIG, RAPID_CLICKS_CONFIG, RAPID_PERS_CLICKS_CONFIG}) {
            TMemoryInput stream{configStr.cbegin(), configStr.size()};
            ParseFromTextFormat(stream, *config.Add());
        }

        TVector<float> output(8);
        const auto& multiView = storage.CreateMultiConstViewFor(
            EFactorSlice::WEB_PRODUCTION,
            EFactorSlice::RAPID_CLICKS,
            EFactorSlice::RAPID_PERS_CLICKS
        );
        NNeuralNet::SelectFactorsFromStorage(multiView, config, output);

        static const std::vector<float> expected{0.83, 0.5, 0.87, 0.0, 0.15, 0.3, 0.8, 0.182};

        UNIT_ASSERT_EQUAL(expected.size(), output.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            UNIT_ASSERT_EQUAL(expected[i], output[i]);
        }
    }
}
