#include <library/cpp/testing/unittest/registar.h>

#include "decoded_key.h"

using namespace NDoom;

Y_UNIT_TEST_SUITE(TDecodedKeyTest) {
    struct TForm {
        TForm(ELanguage language, EFormFlags flags, const TString& text)
            : Language(language)
            , Flags(flags)
            , Text(text)
        {
        }

        ELanguage Language;
        EFormFlags Flags;
        TString Text;
    };

    bool operator==(const TForm& l, const TDecodedFormRef& r) {
        return l.Language == r.Language() && l.Flags == r.Flags() && l.Text == r.Text();
    }

    TDecodedKey GetKey() {
        TDecodedKey key;
        key.SetLemma("SuperLemma");
        key.AddForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1");
        key.AddForm(ELanguage::LANG_RUS, EFormFlag::FORM_HAS_LANG, "Form2");
        return key;
    }

    Y_UNIT_TEST(TestSimple) {
        TDecodedKey key;
        key.SetLemma("SuperLemma");
        UNIT_ASSERT_EQUAL(0, key.FormCount());
        UNIT_ASSERT_EQUAL("SuperLemma", key.Lemma());
        key.AddForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1");
        UNIT_ASSERT_EQUAL("SuperLemma", key.Lemma());
        UNIT_ASSERT_EQUAL(1, key.FormCount());
        UNIT_ASSERT_EQUAL(TForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1"), key.Form(0));
        key.AddForm(ELanguage::LANG_RUS, EFormFlag::FORM_HAS_LANG, "Form2");
        UNIT_ASSERT_EQUAL("SuperLemma", key.Lemma());
        UNIT_ASSERT_EQUAL(2, key.FormCount());
        UNIT_ASSERT_EQUAL(TForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1"), key.Form(0));
        UNIT_ASSERT_EQUAL(TForm(ELanguage::LANG_RUS, EFormFlag::FORM_HAS_LANG, "Form2"), key.Form(1));
    }

    Y_UNIT_TEST(TestKeyEquals) {
        TDecodedKey key = GetKey();
        TDecodedKey key2;
        UNIT_ASSERT(!(key == key2));
        key2.SetLemma("SuperLemma");
        UNIT_ASSERT(!(key == key2));
        key2.AddForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1");
        UNIT_ASSERT(!(key == key2));
        key2.AddForm(ELanguage::LANG_RUS, EFormFlag::FORM_HAS_LANG, "Form2");
        UNIT_ASSERT(key == key2);
    }

    Y_UNIT_TEST(TestAddFormRef) {
        TDecodedKey key = GetKey();
        TDecodedKey key2 = GetKey();
        key.AddForm(key2.Form(0));
        UNIT_ASSERT_EQUAL(3, key.FormCount());
        UNIT_ASSERT_EQUAL(TForm(ELanguage::LANG_ENG, EFormFlag::FORM_HAS_LANG | EFormFlag::FORM_TRANSLIT, "Form1"), key.Form(2));
    }

    Y_UNIT_TEST(TestFormIndex) {
        TDecodedKey key = GetKey();
        TDecodedKey key2 = GetKey();
        UNIT_ASSERT_EQUAL(0, key.FormIndex(key2.Form(0)));
        UNIT_ASSERT_EQUAL(1, key.FormIndex(key2.Form(1)));
        key2.AddForm(ELanguage::LANG_RUM, EFormFlag::FORM_HAS_LANG, "Form3");
        UNIT_ASSERT_EQUAL(-1, key.FormIndex(key2.Form(2)));
    }
}
