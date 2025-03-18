#include <library/cpp/ssh/sign_agent.h>
#include <library/cpp/ssh/ssh.h>

#include <util/stream/file.h>
#include <util/string/vector.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {
    TString ReadPub(const TString& keyName) {
        TFileInput file(ArcadiaSourceRoot() + "/library/cpp/ssh/ut/testdata/" + keyName + ".pub");
        auto parts = SplitString(file.ReadAll(), " ");
        return parts[1];
    }
}

Y_UNIT_TEST_SUITE(SshAgent) {
    Y_UNIT_TEST(Identities) {
        TSSHThinSession ses;
        TSSHAgent agent(&ses);

        auto keys = agent.Identities();
        UNIT_ASSERT_EQUAL(2, keys.size());
        UNIT_ASSERT_STRINGS_EQUAL(ReadPub("rsa_key"), Base64Encode(keys[0].PublicKey()));
        UNIT_ASSERT_STRINGS_EQUAL(ReadPub("ecdsa_key"), Base64Encode(keys[1].PublicKey()));
    }

    Y_UNIT_TEST(Sign) {
        TSSHThinSession ses;
        TSSHAgent agent(&ses);

        auto signs = agent.Sign("some_data", "some-user");
        UNIT_ASSERT_EQUAL(2, signs.size());
        UNIT_ASSERT_UNEQUAL(signs[0], signs[1]);
        UNIT_ASSERT(!signs[0].empty());
    }

    Y_UNIT_TEST(SignIdentity) {
        TSSHThinSession ses;
        TSSHAgent agent(&ses);

        auto keys = agent.Identities();
        UNIT_ASSERT_EQUAL(2, keys.size());
        auto ecdsaSign = agent.Sign(keys[1], "some_data", "some-user");
        auto rsaSign = agent.Sign(keys[0], "some_data", "some-user");
        UNIT_ASSERT(!rsaSign.empty());
        UNIT_ASSERT_UNEQUAL(rsaSign, ecdsaSign);
    }
}
