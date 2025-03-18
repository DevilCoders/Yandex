#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/gradient_optimize/golden_search.h>

class TGoldenSearchTest: public TTestBase {
    UNIT_TEST_SUITE(TGoldenSearchTest);
    UNIT_TEST(Test);
    UNIT_TEST_SUITE_END();

public:
    void Test() {
        auto f = [](double x) {
            return x * x * x - x;
        };

        auto x1 = GoldenSearch(f, 0., 1., 1e-3);
        UNIT_ASSERT_DOUBLES_EQUAL(x1, 1. / sqrt(3.), 1.e-3);
        UNIT_ASSERT(fabs(x1 - 1. / sqrt(3.)) > 1.e-7);
        auto x2 = GoldenSearch(f, 0., 1., 0, 10);
        UNIT_ASSERT_DOUBLES_EQUAL(x2, 1. / sqrt(3.), 1. / pow(1.5, 10));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TGoldenSearchTest);
