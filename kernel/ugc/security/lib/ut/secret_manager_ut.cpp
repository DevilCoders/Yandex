#include "secret_manager.h"
#include "exception.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/datetime/base.h>
#include <util/system/env.h>

using namespace NUgc::NSecurity;

TString GetDay(const TInstant& instant) {
    struct tm dateTime;
    auto localTime = instant.LocalTime(&dateTime);
    return Strftime("%Y-%m-%d", localTime);
}

Y_UNIT_TEST_SUITE(TSecretManagerTest) {

    Y_UNIT_TEST(TestInitInvalidMode) {
        UNIT_ASSERT_EXCEPTION(TSecretManager("abacaba"), TApplicationException);
    }

    Y_UNIT_TEST(TestInitTestMode) {
        UNIT_ASSERT_NO_EXCEPTION(TSecretManager(DEV));
    }

    Y_UNIT_TEST(TestInitProdMode) {
        UNIT_ASSERT_NO_EXCEPTION(TSecretManager(PROD));
    }

    Y_UNIT_TEST(TestModeDevAddSingleKey) {
        TSecretManager secretManager(DEV);
        TStringBuf key = "aaa123";
        std::vector<TString> keys = {static_cast<TString>(key)};
        secretManager.AddSingleKey(key);
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKey(), key);
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), keys);
    }

    Y_UNIT_TEST(TestModeDevAddSingleKeyInCtor) {
        TStringBuf key = "aaa123";
        TSecretManager secretManager(DEV, key);
        std::vector<TString> keys = {static_cast<TString>(key)};
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKey(), key);
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), keys);
    }

    Y_UNIT_TEST(TestModeDevNoKeys) {
        TSecretManager secretManager(DEV);
        UNIT_ASSERT_EXCEPTION(secretManager.GetKey(), TApplicationException);
        UNIT_ASSERT_EXCEPTION(secretManager.GetKeys(), TApplicationException);
    }

    Y_UNIT_TEST(TestModeDevNoActiveKeys) {
        TSecretManager secretManager(DEV);
        TDuration oneDay = TDuration::Days(1);
        TInstant now = Now();
        TInstant tomorrow = now + oneDay;
        TString key = "secret_" + GetDay(tomorrow);
        secretManager.AddDatedKey(key);
        UNIT_ASSERT_EXCEPTION(secretManager.GetKey(), TApplicationException);
    }

    Y_UNIT_TEST(TestModeDevAddFewSingleKeys) {
        TSecretManager secretManager(DEV);
        TStringBuf key1 = "aaa111";
        TStringBuf key2 = "bbb222";
        std::vector<TString> keys = {static_cast<TString>(key1)};
        secretManager.AddSingleKey(key1);
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKey(), key1);
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), keys);
        UNIT_ASSERT_EXCEPTION(secretManager.AddSingleKey(key2), TApplicationException);
    }

    Y_UNIT_TEST(TestModeDevAddFewSingleAndDatedKeys) {
        TSecretManager secretManager(DEV);
        TStringBuf key1 = "aaa111";
        TStringBuf key2 = "my_secret_key_2017-11-12";
        secretManager.AddSingleKey(key1);
        UNIT_ASSERT_EXCEPTION(secretManager.AddDatedKey(key2), TApplicationException);
    }

    Y_UNIT_TEST(TestModeDevAddDatedKey) {
        TSecretManager secretManager(DEV);
        TStringBuf key = "my_secret_key_2017-11-12";
        secretManager.AddDatedKey(key);
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKeyByDate(TInstant::ParseIso8601Deprecated("2017-11-13")), "my_secret_key");
    }

    Y_UNIT_TEST(TestModeDevReturnKeyByDate) {
        TSecretManager secretManager(DEV);
        TStringBuf key = "my_secret_key_2017-11-12";
        secretManager.AddDatedKey(key);
        const TString& resultKey = secretManager.GetKeyByDate(TInstant::ParseIso8601Deprecated("2017-11-13"));
        UNIT_ASSERT_STRINGS_EQUAL(resultKey, "my_secret_key");
    }

    Y_UNIT_TEST(TestModeDevAddDatedKeys) {
        TSecretManager secretManager(DEV);
        TDuration oneDay = TDuration::Days(1);
        TInstant now = Now();
        TInstant yesterday = now - oneDay;
        TInstant dayBeforeYesterday = yesterday - oneDay;
        TInstant tomorrow = now + oneDay;

        TString key1 = "secret_1_" + GetDay(dayBeforeYesterday);
        TString key2 = "secret_2_" + GetDay(yesterday);
        TString key3 = "secret_3_" + GetDay(tomorrow);
        std::vector<TString> keys = {key1, key2, key3};

        for (auto key: keys) {
            secretManager.AddDatedKey(static_cast<TStringBuf>(key));
        }
        std::vector<TString> keysSorted =
             {"secret_3", "secret_2", "secret_1"};
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), keysSorted);
        UNIT_ASSERT_EQUAL(secretManager.GetKeyByDate(TInstant::Now()), "secret_2");
    }

    Y_UNIT_TEST(TestModeDevAddSingleKeyAfterDatedKey) {
        TSecretManager secretManager(DEV);
        secretManager.AddDatedKey("secret_1_2017-11-12");
        UNIT_ASSERT_EXCEPTION(secretManager.AddSingleKey("my_secret_key"), TApplicationException);
    }

    Y_UNIT_TEST(TestModeTestAddSingleKey) {
        TSecretManager secretManager(TEST);
        TString key = "UGC_TEST_KEY_1";
        TString secret = "my_little_secret";
        SetEnv(key, secret);
        std::vector<TString> secrets = {secret};
        secretManager.AddSingleKey(static_cast<TStringBuf>(key));
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKey(), secret);
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), secrets);
    }

    Y_UNIT_TEST(TestModeTestAddDatedKeys) {
        TSecretManager secretManager(TEST);
        TDuration oneDay = TDuration::Days(1);
        TInstant now = Now();
        TInstant yesterday = now - oneDay;
        TInstant dayBeforeYesterday = yesterday - oneDay;
        TInstant tomorrow = now + oneDay;

        TString key1 = "UGC_TEST_KEY_1_" + GetDay(dayBeforeYesterday);
        TString key2 = "UGC_TEST_KEY_2_" + GetDay(yesterday);
        TString key3 = "UGC_TEST_KEY_3_" + GetDay(tomorrow);
        std::vector<TString> keys = {key1, key2, key3};
        std::vector<TString> secrets = {
            "secret_1",
            "secret_2",
            "secret_3"
        };
        std::vector<TString> secretsSorted = {
            "secret_3",
            "secret_2",
            "secret_1"
        };


        for (size_t i = 0; i < keys.size(); ++i) {
            SetEnv(keys[i], secrets[i]);
            secretManager.AddDatedKey(static_cast<TStringBuf>(keys[i]));
        }

        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), secretsSorted);
        UNIT_ASSERT_EQUAL(secretManager.GetKeyByDate(TInstant::Now()), "secret_2");
    }

    Y_UNIT_TEST(TestModeTestReturnKeyByDate) {
        TSecretManager secretManager(TEST);
        TString key = "my_secret_key_2017-11-12";
        SetEnv(key, "my_secret_key");
        secretManager.AddDatedKey(key);
        const TString& resultKey = secretManager.GetKeyByDate(TInstant::ParseIso8601Deprecated("2017-11-13"));
        UNIT_ASSERT_STRINGS_EQUAL(resultKey, "my_secret_key");
    }

    Y_UNIT_TEST(TestModeTestAddInvalidKey) {
        TSecretManager secretManager(TEST);
        TString key = "UGC_TEST_KEY_1";
        SetEnv(key, nullptr);
        UNIT_ASSERT_EXCEPTION(secretManager.AddSingleKey(static_cast<TStringBuf>(key)),
            TApplicationException);
    }

    Y_UNIT_TEST(TestModeTestDecodeKey) {
        TSecretManager secretManager(TEST);
        TString base64Key = "BASE64_ENCODE";
        TString base64Value = "1";
        SetEnv(base64Key, base64Value);

        TString key = "UGC_TEST_KEY_1";
        TString secret = "my_little_secret";
        TString encodeSecret = "bXlfbGl0dGxlX3NlY3JldA==";
        SetEnv(key, encodeSecret);
        std::vector<TString> secrets = {secret};
        secretManager.AddSingleKey(static_cast<TStringBuf>(key));
        UNIT_ASSERT_STRINGS_EQUAL(secretManager.GetKey(), secret);
        UNIT_ASSERT_EQUAL(secretManager.GetKeys(), secrets);
        SetEnv(base64Key, "0");
    }

    Y_UNIT_TEST(TestModeTestDecodeInvalidKey) {
        TSecretManager secretManager(TEST);
        TString base64Key = "BASE64_ENCODE";
        TString base64Value = "1";
        SetEnv(base64Key, base64Value);

        TString key = "UGC_TEST_KEY_1";
        TString invalidEncodeSecret = "bXlfbGl0dGxlX3NlY3JldA";
        SetEnv(key, invalidEncodeSecret);
        UNIT_ASSERT_EXCEPTION(secretManager.AddSingleKey(static_cast<TStringBuf>(key)), TApplicationException);
        SetEnv(base64Key, "0");
    }
}
