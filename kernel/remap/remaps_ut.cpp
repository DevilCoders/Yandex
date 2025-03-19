#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>

#include "remaps.h"

class TRemapsTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TRemapsTest);
        UNIT_TEST(TestLinear);
        UNIT_TEST(TestLogarithmic);
        UNIT_TEST(TestGroup);
        UNIT_TEST(TestExponentialGroup);
        UNIT_TEST(TestExponentialGroupOrder);
    UNIT_TEST_SUITE_END();

    void TestRemapAll(TRemapPtr remap, const float* values, const float* remappedValues, size_t valuesCount) {
        TVector<float> allValues(values, values + valuesCount);
        TVector<float*> valuesToRemap;
        for (size_t i = 0; i < valuesCount; ++i) {
            valuesToRemap.push_back(&allValues[i]);
        }

        remap->Remap(allValues, valuesToRemap);

        for (size_t i = 0; i < valuesCount; ++i) {
            UNIT_ASSERT_DOUBLES_EQUAL_DEPRECATED(remappedValues[i], allValues[i], 1e-6);
        }
    }

    void TestRemapEven(TRemapPtr remap, const float* values, const float* remappedValues, size_t valuesCount) {
        TVector<float> allValues(values, values + valuesCount);
        TVector<float*> valuesToRemap;

        for (size_t i = 0; i < valuesCount; i+=2) {
            valuesToRemap.push_back(&allValues[i]);
        }

        remap->Remap(allValues, valuesToRemap);

        for (size_t i = 0; i < valuesCount; ++i) {
            if (i % 2 == 0) {
                UNIT_ASSERT_DOUBLES_EQUAL_DEPRECATED(remappedValues[i], allValues[i], 1e-6);
            } else {
                UNIT_ASSERT_DOUBLES_EQUAL_DEPRECATED(values[i], allValues[i], 1e-6);
            }
        }
    }

public:

    void TestLinear() {
        float values[] = {0.4f, 0.1f, 1.5f, 0.7f, 4.0f};
        const float remappedValues[] = {0.1f, 0.025f, 0.375f, 0.175f, 1.0f};
        const size_t valuesCount = sizeof(values) / sizeof(values[0]);

        TRemapPtr linearRemap = GetRemap(TString("lin"));

        TestRemapEven(linearRemap, values, remappedValues, valuesCount);
        TestRemapAll(linearRemap, values, remappedValues, valuesCount);
    }

    void TestLogarithmic() {
        float values[] = {3.0f, 1.0f, 27.0f, 9.0f, 81.0f};
        float remappedValues[] = {0.25f, 0.0f, 0.75f, 0.5f, 1.0f};
        const size_t valuesCount = sizeof(values) / sizeof(values[0]);

        TRemapPtr logarithmicRemap = GetRemap(TString("log"));

        TestRemapEven(logarithmicRemap, values, remappedValues, valuesCount);
        TestRemapAll(logarithmicRemap, values, remappedValues, valuesCount);
    }

    void TestGroup() {
        float values[] = { 2.0f, 1.0f, 8.0f, 4.0f, 20.0f, 10.0f };
        float remappedValues2[] = { 1.0f / 14, 0.0f, 7.0f / 14, 3.0f / 14, 24.0f / 24, 14.f / 24 };
        float remappedValues3[] = { 1.0f / 9, 0.0f, 10.0f / 18, 3.0f / 9, 1.0f, 12.0f / 18 };
        const size_t valuesCount = sizeof(values) / sizeof(values[0]);

        TRemapPtr groupRemap = GetRemap(TString("grp@2"));

        TestRemapEven(groupRemap, values, remappedValues2, valuesCount);
        TestRemapAll(groupRemap, values, remappedValues2, valuesCount);

        groupRemap = GetRemap(TString("grp@3"));

        TestRemapAll(groupRemap, values, remappedValues3, valuesCount);
    }

    void TestExponentialGroup() {
        float values[] = { 2.0f, 1.0f, 4.0f, 3.0f, 6.0f, 5.0f, 8.0f, 7.0f };
        float remappedValues3[] = { 1.0f / 12, 0.0f, 3.0f / 12, 2.0f / 12, 3.0f / 6, 2.0f / 6, 1.0f, 2.0f / 3 };
        float remappedValues4[] = { 5.0f / 16, 0.0f, 7.0f / 16, 6.0f / 16, 5.0f / 8, 4.0f / 8, 1.0f, 3.0f / 4 };
        const size_t valuesCount = sizeof(values) / sizeof(values[0]);

        TRemapPtr expGroupRemap = GetRemap(TString("exp@3@2"));

        TestRemapEven(expGroupRemap, values, remappedValues3, valuesCount);
        TestRemapAll(expGroupRemap, values, remappedValues3, valuesCount);

        expGroupRemap = GetRemap(TString("exp@4@2"));

        TestRemapAll(expGroupRemap, values, remappedValues4, valuesCount);
    }

    void TestExponentialGroupOrder() {
        float values[] = { 2.0f, 1.0f, 4.0f, 3.0f, 5.0f, 8.0f, 5.0f, 7.0f };
        float remappedValues3[] = { 1.0f / 12, 0.0f, 3.0f / 12, 2.0f / 12, 6.0f / 12, 1.0f, 6.0f / 12, 8.0f / 12 };
        float remappedValues4[] = { 5.0f / 16, 4.0f / 16, 7.0f / 16, 6.0f / 16, 10.0f / 16, 1.0f, 10.0f / 16, 3.0f / 4 };
        const size_t valuesCount = sizeof(values) / sizeof(values[0]);

        TRemapPtr remap = GetRemap(TString("orderExpGroup@3@2"));

        TestRemapEven(remap, values, remappedValues3, valuesCount);
        TestRemapAll(remap, values, remappedValues3, valuesCount);

        remap = GetRemap(TString("orderExpGroup@4@2"));

        TestRemapAll(remap, values, remappedValues4, valuesCount);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TRemapsTest);
