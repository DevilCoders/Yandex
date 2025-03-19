#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_filters.h>
#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_config.sc.h>

using namespace NVwFilters;


const TFsPath DATA_DIR = TFsPath(BuildRoot()) / "kernel/facts/vowpal_wabbit_filters/ut/data";
const TFsPath CONFIG_PATH = DATA_DIR / "filters_config.json";


Y_UNIT_TEST_SUITE(TVowpalWabbitFilters) {
    Y_UNIT_TEST(TestNotFiltered) {
        const TString query = "кто такой стример братишкин";
        const TString answer = "Стример Братишкин — талантливый блоггер, да и просто замечательный человек!";

        TVowpalWabbitMultiFilter filter(DATA_DIR, CONFIG_PATH);
        const TFilterResult result = filter.Filter(query, answer, {});

        UNIT_ASSERT(!result.Filtered);
        UNIT_ASSERT_STRINGS_EQUAL(result.FilteredBy, "");
        UNIT_ASSERT_STRINGS_EQUAL(result.Error, "");
    }

    Y_UNIT_TEST(TestFiltered) {
        const TString query = "как лечить рак в домашних условиях";
        const TString answer = "Чтобы вылечить рак в домашних условиях, возьмите соду, смешайте с медом, погрейте в...";

        TVowpalWabbitMultiFilter filter(DATA_DIR, CONFIG_PATH);
        const TFilterResult result = filter.Filter(query, answer, {});

        UNIT_ASSERT(result.Filtered);
        UNIT_ASSERT_STRINGS_EQUAL(result.FilteredBy, "medicine");
        UNIT_ASSERT_STRINGS_EQUAL(result.Error, "");
    }

    Y_UNIT_TEST(TestPatchOk) {
        const TString query = "кто такой стример братишкин";
        const TString answer = "Стример Братишкин — талантливый блоггер, да и просто замечательный человек!";
        const NSc::TValue configPatch = NSc::TValue::FromJson(R"""(
          {
            "filters": {
              "medicine": {
                "threshold": -999
              }
            }
          }
        )""");

        TVowpalWabbitMultiFilter filter(DATA_DIR, CONFIG_PATH);
        const TFilterResult result = filter.Filter(query, answer, configPatch);

        UNIT_ASSERT(result.Filtered);
        UNIT_ASSERT_STRINGS_EQUAL(result.FilteredBy, "medicine");
        UNIT_ASSERT_STRINGS_EQUAL(result.Error, "");
    }

    Y_UNIT_TEST(TestPatchErrorInvalidConfig) {
        const TString query = "кто такой стример братишкин";
        const TString answer = "Стример Братишкин — талантливый блоггер, да и просто замечательный человек!";
        const NSc::TValue configPatch = NSc::TValue::FromJson(R"""(
          {
            "filters": {
              "medicine": {
                "threshold": "doge_magic_threshold"
              }
            }
          }
        )""");

        TVowpalWabbitMultiFilter filter(DATA_DIR, CONFIG_PATH);
        const TFilterResult result = filter.Filter(query, answer, configPatch);

        UNIT_ASSERT(!result.Filtered);
        UNIT_ASSERT_STRINGS_EQUAL(result.FilteredBy, "");
        UNIT_ASSERT_STRINGS_EQUAL(result.Error, "Config validation error: /filters/medicine/threshold: is not double; ");
    }

    Y_UNIT_TEST(TestPatchErrorModelNotFound) {
        const TString query = "кто такой стример братишкин";
        const TString answer = "Стример Братишкин — талантливый блоггер, да и просто замечательный человек!";
        const NSc::TValue configPatch = NSc::TValue::FromJson(R"""(
          {
            "filters": {
              "medicine": {
                "fml": "fs_politics.vw"
              }
            }
          }
        )""");

        TVowpalWabbitMultiFilter filter(DATA_DIR, CONFIG_PATH);
        const TFilterResult result = filter.Filter(query, answer, configPatch);

        UNIT_ASSERT(!result.Filtered);
        UNIT_ASSERT_STRINGS_EQUAL(result.FilteredBy, "");
        UNIT_ASSERT_STRINGS_EQUAL(result.Error, "Model fs_politics.vw not found");
    }
}
