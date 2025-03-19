#include <kernel/ugc/security/lib/token.h>
#include <kernel/ugc/security/proto/crypto_bundle.pb.h>

#include <kernel/ugc/runtime/modes.h>

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NUgc::NSecurity;

namespace {
    TSecretManager CreateSecretManager() {
        TSecretManager secretManager(DEV);
        secretManager.AddDatedKey("1test-secret-key-test-secret-key_2018-01-01");
        return secretManager;
    }
} // namespace

Y_UNIT_TEST_SUITE(TokenTests) {
    Y_UNIT_TEST(EncryptDecryptSuccessfully) {
        TTokenAad expected;
        expected.SetCreationTimeMs(123);
        expected.SetTtlMs(456);

        TSecretManager secretManager = CreateSecretManager();
        TString token = EncryptToken(secretManager, expected);

        TTokenAad data;
        UNIT_ASSERT(DecryptToken(secretManager, token, &data));
        UNIT_ASSERT_EQUAL(data.GetCreationTimeMs(), expected.GetCreationTimeMs());
        UNIT_ASSERT_EQUAL(data.GetTtlMs(), expected.GetTtlMs());
    }

    Y_UNIT_TEST(FailsToDecryptInvalidToken) {
        TTokenAad data;
        UNIT_ASSERT(!DecryptToken(CreateSecretManager(), "abc", &data));
    }

    Y_UNIT_TEST(FailsToDectyptTokenWithInvalidAad) {
        TCryptoBundle bundle;
        bundle.SetAad("abc");

        TTokenAad data;
        TString serialized = bundle.SerializeAsString();
        UNIT_ASSERT(!DecryptToken(CreateSecretManager(), Base64Encode(serialized), &data));
    }

    Y_UNIT_TEST(FailsToDectyptOutdatedToken) {
        TTokenAad aad;
        aad.SetCreationTimeMs(TInstant::ParseIso8601Deprecated("2018-01-02").MilliSeconds());
        aad.SetTtlMs(456);

        auto secretManager = CreateSecretManager();
        TTokenAad data;
        TString token = EncryptToken(secretManager, data, aad);
        UNIT_ASSERT(!DecryptToken(secretManager, token, &data));
    }

    Y_UNIT_TEST(FailsToDectyptTokenWithInvalidCT) {
        auto secretManager = CreateSecretManager();
        TTokenAad data;
        TString token = EncryptToken(secretManager, data);

        TCryptoBundle bundle;
        UNIT_ASSERT(bundle.ParseFromString(Base64Decode(token)));
        bundle.SetCiphertext("invalid ct");

        TString serialized = bundle.SerializeAsString();
        UNIT_ASSERT(!DecryptToken(secretManager, Base64Encode(serialized), &data));
    }
}
