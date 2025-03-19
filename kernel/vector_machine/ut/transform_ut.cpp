#include <kernel/vector_machine/transforms.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NVectorMachine;

class TTransformTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TTransformTest);
        UNIT_TEST(TestSigmoid);
        UNIT_TEST(TestLinear);
        UNIT_TEST(TestSigmoidLinear);
        UNIT_TEST(TestClip);
    UNIT_TEST_SUITE_END();

public:
    void TestSigmoid() {
        TSigmoidTransform transform;

        TVector<float> inputs = {-100.f, 0.f, 100.f};
        TVector<float> expected = {0.f, 0.5, 1.f};

        for (size_t i = 0; i < inputs.size(); ++i) {
            float feature = transform.Transform(inputs[i]);
            UNIT_ASSERT_DOUBLES_EQUAL(feature, expected[i], 1e-5);
        }
    }

    void TestLinear() {
        TLinearTransform transform(2, 1);

        TVector<float> inputs = {0.f, 0.5, 1.f};
        TVector<float> expected = {1.f, 2.f, 3.f};

        for (size_t i = 0; i < inputs.size(); ++i) {
            float feature = transform.Transform(inputs[i]);
            UNIT_ASSERT_DOUBLES_EQUAL(feature, expected[i], 1e-5);
        }
    }

    void TestSigmoidLinear() {
        TLinearTransform transform1(2, 1);
        TSigmoidTransform transform2;

        TVector<float> inputs = {-100.f, 0.f, 100.f};
        TVector<float> expected = {1.f, 2.f, 3.f};

        for (size_t i = 0; i < inputs.size(); ++i) {
            float feature = transform1.Transform(transform2.Transform(inputs[i]));
            UNIT_ASSERT_DOUBLES_EQUAL(feature, expected[i], 1e-5);
        }
    }

    void TestClip() {
        TClampTransform transform(0.f, 1.f);

        TVector<float> inputs = {-1.5f, 0.5f, 1.5f};
        TVector<float> expected = {0.f, 0.5f, 1.f};

        for (size_t i = 0; i < inputs.size(); ++i) {
            float feature = transform.Transform(inputs[i]);
            UNIT_ASSERT_DOUBLES_EQUAL(feature, expected[i], 1e-5);
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTransformTest);
