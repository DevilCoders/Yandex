#include <library/cpp/testing/unittest/registar.h>

#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_filters.h>
#include <kernel/facts/vowpal_wabbit_filters/vowpal_wabbit_config.sc.h>

using namespace NVwFilters;


Y_UNIT_TEST_SUITE(TVowpalWabbitFilterConfigValidation) {
    Y_UNIT_TEST(TestValidateOk) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 1,
                "fml": "fs_medicine.vw",
                "threshold": 0.191257,
                "ngram": 2,
                "namespaces": [
                  "Q",
                  "A"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "");
    }

    Y_UNIT_TEST(TestValidateNoFml) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 1,
                "threshold": 0.191257,
                "ngram": 2,
                "namespaces": [
                  "Q",
                  "A"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine/fml: is a required field and is not found; ");
    }

    Y_UNIT_TEST(TestValidateNoThreshold) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 1,
                "fml": "fs_medicine.vw",
                "ngram": 2,
                "namespaces": [
                  "Q",
                  "A"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst <TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine/threshold: is a required field and is not found; ");
    }

    Y_UNIT_TEST(TestValidateBadThreshold) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 1,
                "fml": "fs_medicine.vw",
                "threshold": "0.191257",
                "ngram": 2,
                "namespaces": [
                  "Q",
                  "A"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine/threshold: is not double; ");
    }

    Y_UNIT_TEST(TestValidateNoNgram) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 1,
                "fml": "fs_medicine.vw",
                "threshold": 0.191257,
                "namespaces": [
                  "Q",
                  "A"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine/ngram: is a required field and is not found; ");
    }

    Y_UNIT_TEST(TestValidateNoNamespaces) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 0,
                "fml": "fs_medicine.vw",
                "threshold": 0.191257,
                "ngram": 2
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine/namespaces: is a required field and is not found; ");
    }

    Y_UNIT_TEST(TestValidateBadNamespace) {
        const NSc::TValue rawConfig = NSc::TValue::FromJsonThrow(R"""(
          {
            "filters": {
              "medicine": {
                "enabled": 0,
                "fml": "fs_medicine.vw",
                "threshold": 0.191257,
                "ngram": 2,
                "namespaces": [
                  "Q",
                  "A",
                  "DOGE_such_ns"
                ]
              }
            }
          }
        )""");
        TVwMultiFilterConfigConst<TSchemeTraits> config(&rawConfig);
        TConfigValidationHelper helper;
        UNIT_ASSERT(!config.Validate("", false, std::ref(helper), &helper));
        UNIT_ASSERT_STRINGS_EQUAL(helper.ToString(), "/filters/medicine: \"namespaces\" validation failed: unknown namespace DOGE_such_ns; ");
    }
}
