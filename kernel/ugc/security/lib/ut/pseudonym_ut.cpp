#include "pseudonym.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NUgc::NSecurity;

class TPseudonymTest: public TTestBase {
public:
    UNIT_TEST_SUITE(TPseudonymTest);
    UNIT_TEST(Creates)
    UNIT_TEST(Rejects)
    UNIT_TEST_SUITE_END();

    void SetUp() override {
        SecretManager.Reset(new TSecretManager(DEV));
        SecretManager->AddSingleKey("UGC_PSEUDONYM_KEY");
        Pseudonym.Reset(new TPseudonym(*SecretManager));
    }

    void TearDown() override {
        SecretManager.Reset();
        Pseudonym.Reset();
    }

    void Creates() {
        const ui64 passport = 123;
        const TString pseudo = Pseudonym->IdToPseudo(passport);
        UNIT_ASSERT_STRINGS_EQUAL("AbfUfxk60HdB", pseudo);
        ui64 receivedPassport = 0;
        UNIT_ASSERT(Pseudonym->PseudoToId(pseudo, receivedPassport));
        UNIT_ASSERT_EQUAL(passport, receivedPassport);
    }

    void Rejects() {
        ui64 receivedPassport = 0;
        UNIT_ASSERT(!Pseudonym->PseudoToId("12345678901", receivedPassport)); // short
        UNIT_ASSERT(!Pseudonym->PseudoToId("1234567890123", receivedPassport)); // long
        UNIT_ASSERT(!Pseudonym->PseudoToId("BbfUfxk60HdB", receivedPassport)); // version
        UNIT_ASSERT_EQUAL(0, receivedPassport);
    }

    THolder<TSecretManager> SecretManager;
    THolder<TPseudonym> Pseudonym;
};

UNIT_TEST_SUITE_REGISTRATION(TPseudonymTest);
