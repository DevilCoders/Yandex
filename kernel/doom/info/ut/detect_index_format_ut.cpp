#include <kernel/doom/info/detect_index_format.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

Y_UNIT_TEST_SUITE(TDetectIndexFormatTests) {
    using namespace NDoom;

    const size_t TEST_INDEX_CNT = 5;
    const TStringBuf pathSuffixes[TEST_INDEX_CNT] = {
        TStringBuf("detect_index_format_testing/1_yindex/index"),
        TStringBuf("detect_index_format_testing/2_yindex_panther/indexpanther."),
        TStringBuf("detect_index_format_testing/3_array4d/indexfactorann.data"),
        TStringBuf("detect_index_format_testing/4_off_panther/indexpanther")
    };
    const EIndexFormat indexFormats[TEST_INDEX_CNT] = {
        YandexIndexFormat,
        YandexPantherIndexFormat,
        Array4dIndexFormat,
        OffroadPantherIndexFormat
    };

    void RunTest(TStringBuf pathSuffix, EIndexFormat answer) {
        EIndexFormat format = DetectIndexFormat(TString(pathSuffix));
        UNIT_ASSERT_VALUES_EQUAL(format, answer);
    }

    Y_UNIT_TEST(TestIndexFormats) {
        for (size_t i = 0; i < TEST_INDEX_CNT; ++i)
            RunTest(pathSuffixes[i], indexFormats[i]);
    }

}
