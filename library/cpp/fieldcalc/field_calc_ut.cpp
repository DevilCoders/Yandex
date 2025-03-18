#include <cmath>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>

#include "field_calc.h"
#include "field_calc_int.h"

enum ut_enum1 {
    FLAG1 = 65536,
    FLAG2 = 131072,
    FLAG3 = 262144,
    FLAG4 = 524288,
};

enum ut_enum2 {
    ONE = 1,
    TWO = 2,
    THREE = 3,
};

struct ut_struct {
    ui32 f1;
    ui16 f2;
    ui32 f3;
    float f4;

    float log() const {
        return logf(f4);
    };
    int mask() const {
        return f1 | f2 | f3;
    };
};

template <>
std::pair<const named_dump_item*, size_t> get_named_dump_items<ut_struct>() {
    static named_dump_item items[] = {
        // members
        {"f1", &((ut_struct*)nullptr)->f1},
        {"f2", &((ut_struct*)nullptr)->f2},
        {"f3", &((ut_struct*)nullptr)->f3},
        {"f4", &((ut_struct*)nullptr)->f4},

        // methods
        {"log", (float_fn_t)&ut_struct::log},
        {"mask", (int_fn_t)&ut_struct::mask},

        // enums
        {"FLAG1", (long)FLAG1},
        {"FLAG2", (long)FLAG2},
        {"FLAG3", (long)FLAG3},
        {"FLAG4", (long)FLAG4},

        {"ONE", (long)ONE},
        {"TWO", (long)TWO},
        {"THREE", (long)THREE},
    };

    return std::make_pair(items, sizeof(items) / sizeof(*items));
}

class TFieldCalcTest: public TTestBase, protected TFieldCalculator<ut_struct> {
    UNIT_TEST_SUITE(TFieldCalcTest);
    UNIT_TEST(TestSimplePrintOut);
    UNIT_TEST(TestSum);
    UNIT_TEST(TestCondition);
    UNIT_TEST(TestFunction);
    UNIT_TEST(TestLogFunction);
    UNIT_TEST(TestLog10Function);
    UNIT_TEST(TestRoundFunction);
    UNIT_TEST(TestLocalVars);
    UNIT_TEST(TestMultipleAssignment);
    UNIT_TEST(TestVariablesInCondition);
    UNIT_TEST_EXCEPTION(TestReassignment, yexception);
    UNIT_TEST_EXCEPTION(TestAssignmentToNonVariable, yexception);
    UNIT_TEST_EXCEPTION(TestNonDefinedVariable, yexception);
    UNIT_TEST_EXCEPTION(TestPrintingNonDefinedField, yexception);
    UNIT_TEST(TestLocalVariablePrintout);
    UNIT_TEST(TestPrintoutWithAssignment);
    UNIT_TEST(TestConditionWithAssignment);
    UNIT_TEST(TestChainAssignment);
    UNIT_TEST_EXCEPTION(TestFailedChainAssignment, yexception);
    UNIT_TEST(TestTernaryOperator);
    UNIT_TEST(TestTernaryOperatorInCondition);
    UNIT_TEST_SUITE_END();

private:
    TVector<ut_struct> arr;
    //    TFieldCalculator<ut_struct> calc;
    TVector<TVector<eval_res_type>> evals;

public:
    void SetUp() override {
        const int COUNT = 8;
        arr.resize(COUNT);
        for (int i = 0; i < COUNT; ++i) {
            arr[i].f1 = i << 16;
            arr[i].f2 = (ui16)i << 8;
            arr[i].f3 = i;
            arr[i].f4 = 1.0f / (float)(i + 1);
        }
    }
    void TestLocalVars() {
        const char* field_names[] = {"t=1-f4", "-t - t*t/2 - t*t*t/3 - t*t*t*t/4"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {-0.000000, -0.682292, -1.037037, -1.250977, -1.393067, -1.494020, -1.569346, -1.627665};
        ExpectEvals(8, 1, expected);
    }
    void TestSimplePrintOut() {
        const char* field_names[] = {"f1"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0, 65536, 131072, 196608, 262144, 327680, 393216, 458752};
        ExpectEvals(8, 1, expected);
    }
    void TestSum() {
        const char* field_names[] = {"f1 + f2 + f3 + f4"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {1.000000, 65793.500000, 131586.333333, 197379.250000, 263172.200000, 328965.166667, 394758.142857, 460551.125000};
        ExpectEvals(8, 1, expected);
    }
    void TestMultipleAssignment() {
        const char* field_names[] = {"u=f1", "v=u", "w=v", "w"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0, 65536, 131072, 196608, 262144, 327680, 393216, 458752};
        ExpectEvals(8, 1, expected);
    }
    void TestCondition() {
        const char* field_names[] = {"f1", "f2", "f3", "f4", "? f3 == THREE"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {196608, 768, 3, 0.2500};
        ExpectEvals(1, 4, expected);
    }
    void TestVariablesInCondition() {
        const char* field_names[] = {"f1", "f2", "f3", "f4", "u = f3", "? u == THREE"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {196608, 768, 3, 0.2500};
        ExpectEvals(1, 4, expected);
    }
    void TestFunction() {
        const char* field_names[] = {"log"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0.0000, -0.6931, -1.0986, -1.3863, -1.6094, -1.7918, -1.9459, -2.0794};
        ExpectEvals(8, 1, expected);
    }
    void TestLogFunction() {
        const char* field_names[] = {"#LOG#(f4)"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0.0000, -0.6931, -1.0986, -1.3863, -1.6094, -1.7918, -1.9459, -2.0794};
        ExpectEvals(8, 1, expected);
    }
    void TestLog10Function() {
        const char* field_names[] = {"#LOG10#(f4)"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0.0, -0.30103, -0.477121, -0.60206, -0.69897, -0.778151, -0.845098, -0.90309};
        ExpectEvals(8, 1, expected);
    }
    void TestRoundFunction() {
        const char* field_names[] = {"#ROUND#(f4)"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {1, 1, 0, 0, 0, 0, 0, 0};
        ExpectEvals(8, 1, expected);
    }
    void TestReassignment() {
        const char* field_names[] = {"a=0", "? a", "a=2"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
    }
    void TestAssignmentToNonVariable() {
        const char* field_names[] = {"? f1 = 2", "f3"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
    }
    void TestNonDefinedVariable() {
        const char* field_names[] = {"a = b + 2", "f3"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
    }
    void TestPrintingNonDefinedField() {
        const char* field_names[] = {"f5", "f3"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
    }
    void TestLocalVariablePrintout() {
        const char* field_names[] = {"u = f1 + f2 + f3 + f4", "u"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {1.000000, 65793.500000, 131586.333333, 197379.250000, 263172.200000, 328965.166667, 394758.142857, 460551.125000};
        ExpectEvals(8, 1, expected);
    }
    void TestPrintoutWithAssignment() {
        const char* field_names[] = {"(u = f1 + f2 + f3 + f4)", "u-f1-f2-f4"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {1.000000, 0, 65793.500000, 1, 131586.333333, 2, 197379.250000, 3, 263172.200000, 4, 328965.166667, 5, 394758.142857, 6, 460551.125000, 7};
        ExpectEvals(8, 2, expected);
    }
    void TestConditionWithAssignment() {
        const char* field_names[] = {"?(u = f3)", "u"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {1, 2, 3, 4, 5, 6, 7};
        ExpectEvals(7, 1, expected);
    }
    void TestChainAssignment() {
        const char* field_names[] = {"a=(b=f3)", "a"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0, 1, 2, 3, 4, 5, 6, 7};
        ExpectEvals(8, 1, expected);
    }
    void TestFailedChainAssignment() {
        const char* field_names[] = {"a=b=f3", "a"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
    }
    void TestTernaryOperator() {
        const char* field_names[] = {"f3%3 ? f3*10 : 42"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {42, 10, 20, 42, 40, 50, 42, 70};
        ExpectEvals(8, 1, expected);
    }
    void TestTernaryOperatorInCondition() {
        const char* field_names[] = {"?f3%3 ? f3%2 : 1-f3%2", "f3"};
        int field_count = Y_ARRAY_SIZE(field_names);
        CompileAndRun(field_names, field_count);
        const double expected[] = {0, 1, 5, 6, 7};
        ExpectEvals(5, 1, expected);
    }

private:
    void CompileAndRun(const char** names, int field_count) {
        segmented_string_pool pool(16384);
        char** field_names = new char*[field_count];
        TAutoPtr<char*, TDeleteArray> field_names_auto_ptr(field_names);
        for (int i = 0; i < field_count; ++i) {
            field_names[i] = pool.append(names[i]);
        }
        Compile(field_names, field_count);
        evals.clear();
        for (size_t i = 0; i < arr.size(); i++)
            if (Cond(arr[i])) {
                const char* d = reinterpret_cast<const char*>(&arr[i]);
                TVector<eval_res_type> currentLine;
                for (int n = 0; n < out_el; n++) {
                    if (printouts[n].type != DIT_NAME) {
                        eval_res_type res = printouts[n].eval(&d);
                        currentLine.push_back(res);
                    }
                }
                evals.push_back(currentLine);
            }
    }

    void ExpectEvals(int lineCount, int fieldCount, const double* expectedEvals) {
        UNIT_ASSERT_EQUAL((int)evals.size(), lineCount);
        if (lineCount > 0)
            UNIT_ASSERT_EQUAL((int)evals[0].size(), fieldCount);
        for (int i = 0; i < lineCount; ++i)
            for (int j = 0; j < fieldCount; ++j) {
                double actual = (double)evals[i][j];
                double expected = expectedEvals[i * fieldCount + j];
                UNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 1e-4);
            }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TFieldCalcTest)
