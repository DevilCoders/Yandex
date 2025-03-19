#include <library/cpp/testing/unittest/registar.h>

#include "key_encoder.h"
#include "key_decoder.h"

#include "old_key_encoder.h"
#include "old_key_decoder.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TOldKeyEncoderTest) {

    Y_UNIT_TEST(TestLargeLemma) {
        TOldKeyEncoder encoder;
        TDecodedKey key;
        TString encoded;

        key.SetLemma(TString(255, 'a'));
        encoded = encoder.Encode(key);
        UNIT_ASSERT_VALUES_EQUAL(encoded, key.Lemma());

        key.Clear();
        key.SetLemma(TString(256, 'b'));
        encoded = encoder.Encode(key);
        UNIT_ASSERT_VALUES_EQUAL(encoded.size(), 0);
    }

    Y_UNIT_TEST(TestPrefix) {
        TString key = "#tel_code_area=\"011";

        TDecodedKey decodedKey;
        TString encodedKey;

        TOldKeyDecoder decoder(NoDecodingOptions);
        decoder.Decode(key, &decodedKey);

        TOldKeyEncoder encoder;
        bool success = encoder.Encode(decodedKey, &encodedKey);

        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(encodedKey, key);
    }

}

Y_UNIT_TEST_SUITE(TKeyEncoderTest) {

    Y_UNIT_TEST(TestSimple) {
        TKeyEncoder encoder;
        TKeyDecoder decoder(NoDecodingOptions);
        TDecodedKey key;
        TDecodedKey decoded;
        TString encoded;
        TString expected;

        key.SetLemma("Lemma");
        bool success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        expected += "Lemma";
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        key.AddForm(LANG_UNK, 0, "Lemma");

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        key.AddForm(LANG_UNK, 0, "Lemma1");
        success = encoder.Encode(key, &encoded);
        expected += '\x01';
        expected += '\x60';
        expected += '\x52';
        expected += '1';
        expected += '\0';
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Lemma1");
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        expected += '\x62';
        expected += '\x02';
        expected += '\x04';
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        key.AddForm(LANG_ENG, FORM_HAS_LANG, "Lemma1");
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        expected += '\x88';
        expected += '\x00';
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        key.AddForm(LANG_ENG, FORM_HAS_LANG | FORM_TRANSLIT, "SuperLemma");
        key.AddForm(LANG_ENG, FORM_HAS_LANG | FORM_TRANSLIT, "SuperLemmaAndSuffix");
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);

        expected += '\x0C';
        expected += "SuperLemma";
        expected += '\x02';
        expected += '\x0C';

        expected += '\x8A';
        expected += '\x0B';
        expected += "AndSuffix";
        expected += '\x02';
        expected += '\x0C';
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);
    }

    Y_UNIT_TEST(TestBigForms) {
        TKeyEncoder encoder;
        TKeyDecoder decoder(NoDecodingOptions);
        TDecodedKey key;
        TDecodedKey decoded;
        TString encoded;
        TString expected;

        key.SetLemma("Lemma");
        expected += "Lemma";
        expected += '\x01';

        TString bigLemma;
        for (size_t i = 0; i < 130; ++i) {
            bigLemma.push_back('x');
        }
        TString bigLemma2;
        for (size_t i = 0; i < 128; ++i) {
            bigLemma2.push_back('x');
        }
        for (size_t i = 0; i < 20; ++i) {
            bigLemma2.push_back('y');
        }
        key.AddForm(LANG_ENG, FORM_HAS_LANG, bigLemma);
        expected += '\x80';
        expected+= '\x84';
        expected += bigLemma;
        expected += '\x02';
        expected += '\x04';
        bool success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        key.AddForm(LANG_ENG, FORM_HAS_LANG, bigLemma2);
        expected += '\xFF';
        expected += '\x17';
        expected += bigLemma2.substr(127);
        expected += '\x02';
        expected += '\x04';
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);

        while (bigLemma2.size() < MAXKEY_LEN) {
            bigLemma2.push_back('y');
        }
        key.AddForm(LANG_ENG, FORM_HAS_LANG, bigLemma2);
        expected += '\xFF';
        expected += '\x82';
        expected += bigLemma2.substr(127);
        expected += '\x02';
        expected += '\x04';
        success = encoder.Encode(key, &encoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(expected, encoded);

        success = decoder.Decode(expected, &decoded);
        UNIT_ASSERT(success);
        UNIT_ASSERT_VALUES_EQUAL(key, decoded);
    }

}
