#include <library/cpp/testing/unittest/registar.h>

#include "decoded_key.h"
#include "decoded_key_prefix_getter.h"
#include "key_encoder.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TDecodedKeyPrefixGetter) {
    Y_UNIT_TEST(TestSimple) {
        TKeyEncoder encoder;
        TDecodedKey key;
        TString encoded;
        TDecodedKeyPrefixGetter prefixGetter;

        key.SetLemma("Lemma");
        bool success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL("Lemma", prefixGetter(encoded));

        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Form1");
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL("Lemma", prefixGetter(encoded));

        key.AddForm(LANG_RUS, FORM_HAS_LANG | FORM_TRANSLIT, "Form2");
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL("Lemma", prefixGetter(encoded));
    }
}
