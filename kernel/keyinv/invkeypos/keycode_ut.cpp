#include "keycode.h"

#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/escape.h>
#include <util/system/maxlen.h>
#include <kernel/search_types/search_types.h>

namespace {

    void InitLemmaInfo(TKeyLemmaInfo& lemmaInfo, const char* prefix, const char* lemma, unsigned char lang = LANG_UNK) {
        strcpy(lemmaInfo.szPrefix, prefix);
        strcpy(lemmaInfo.szLemma, lemma);
        lemmaInfo.Lang = lang;
    }

    const char* InitLemmaForm(char* buffer, const char* form, ui8 flags = 0, ui8 joins = 0, ui8 lang = 0) {
        strcpy(buffer, form);
        if (flags) {
            int length = strlen(buffer);
            AppendFormFlags(buffer, &length, MAXKEY_BUF, flags, joins, lang);
        }
        return buffer;
    }

    void CompareResults(int numberOfFormsBefore, int numberOfFormsAfter,
        const TKeyLemmaInfo& lemmaInfoBefore, const TKeyLemmaInfo& lemmaInfoAfter,
        char (*formsBefore)[MAXKEY_BUF], char (*formsAfter)[MAXKEY_BUF], bool decodeForms = false)
    {
        UNIT_ASSERT_VALUES_EQUAL(numberOfFormsBefore, numberOfFormsAfter);
        UNIT_ASSERT_STRINGS_EQUAL(lemmaInfoBefore.szPrefix, lemmaInfoAfter.szPrefix);
        UNIT_ASSERT_STRINGS_EQUAL(lemmaInfoBefore.szLemma, lemmaInfoAfter.szLemma);
        UNIT_ASSERT_VALUES_EQUAL(lemmaInfoBefore.Lang, lemmaInfoAfter.Lang);

        for (int i = 0; i < numberOfFormsBefore; ++i) {
            if (!decodeForms) {
                UNIT_ASSERT_STRINGS_EQUAL(formsBefore[i], formsAfter[i]);
            } else {
                char formBefore[MAXKEY_BUF];
                char formAfter[MAXKEY_BUF];

                strcpy(formBefore, formsBefore[i]);
                strcpy(formAfter, formsAfter[i]);

                ui8 flagsBefore = 0, joinsBefore = 0, langBefore = LANG_UNK;
                ui8 flagsAfter = 0, joinsAfter = 0, langAfter = LANG_UNK;
                int lengthBefore = strlen(formBefore);
                int lengthAfter = strlen(formAfter);

                RemoveFormFlags(formBefore, &lengthBefore, &flagsBefore, &joinsBefore, &langBefore);
                RemoveFormFlags(formAfter, &lengthAfter, &flagsAfter, &joinsAfter, &langAfter);

                UNIT_ASSERT_STRINGS_EQUAL(formBefore, formAfter);
                UNIT_ASSERT_VALUES_EQUAL(flagsBefore, flagsAfter);
                UNIT_ASSERT_VALUES_EQUAL(joinsBefore, joinsAfter);
                UNIT_ASSERT_VALUES_EQUAL(langBefore, langAfter);
            }
        }
    }
}

class TKeyCodeTest: public TTestBase {
    UNIT_TEST_SUITE(TKeyCodeTest);
        UNIT_TEST(TestAttributes);
        UNIT_TEST(TestZones);
        UNIT_TEST(TestLemmaWithJoins);
        UNIT_TEST(TestLemmaWithLang);
        UNIT_TEST(TestLemmaNoLang);
        UNIT_TEST(TestInvalidKeys);
        UNIT_TEST(TestNonWords);
        UNIT_TEST(TestKeyPrefixes);
        UNIT_TEST(TestOldKeys);
        UNIT_TEST(TestTokenPrefixes);
        UNIT_TEST(TestFormVersions);
        UNIT_TEST(TestLongForms);
    UNIT_TEST_SUITE_END();
public:
    void TestAttributes();
    void TestZones();
    void TestLemmaWithJoins();
    void TestLemmaWithLang();
    void TestLemmaNoLang();
    void TestInvalidKeys();
    void TestNonWords();
    void TestKeyPrefixes();
    void TestOldKeys();
    void TestTokenPrefixes();
    void TestFormVersions();
    void TestLongForms();
};

UNIT_TEST_SUITE_REGISTRATION(TKeyCodeTest);

//! @note copied from DecodeKey():
//! 1) lemma x02 L x01 forms x00
//! 2) lemma x02 L x00
//! 3) lemma x01 forms x00
//! 4) lemma x00

void TKeyCodeTest::TestAttributes() {
    const char* pkey = nullptr;
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    int numberOfFormsBefore, numberOfFormsAfter;

    // literal attribute
    InitLemmaInfo(lemmaInfoBefore, "#url=\"", "google.softline.ru/news/?p=522");
    numberOfFormsBefore = 0;
    pkey = "#url=\"google.softline.ru/news/?p=522";
    numberOfFormsAfter = DecodeKey(pkey, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // integer attribute
    InitLemmaInfo(lemmaInfoBefore, "#cat=", "00071102237");
    numberOfFormsBefore = 0;
    pkey = "#cat=00071102237";
    numberOfFormsAfter = DecodeKey(pkey, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // integer attribute
    InitLemmaInfo(lemmaInfoBefore, "#00000000000001AB?cat=", "00071102237");
    numberOfFormsBefore = 0;
    pkey = "#00000000000001AB?cat=00071102237";
    numberOfFormsAfter = DecodeKey(pkey, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestZones() {
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];

    const char* key = "(title";
    InitLemmaInfo(lemmaInfoBefore, "", key);
    CompareResults(0, DecodeKey(key, &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    key = ")anchor";
    InitLemmaInfo(lemmaInfoBefore, "", key);
    CompareResults(0, DecodeKey(key, &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestLemmaWithJoins() {
    char key[MAXKEY_BUF];
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    int numberOfFormsBefore, numberOfFormsAfter;

    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_ENG);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert-", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], "alert", FORM_TITLECASE | FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN);
    formPtrsBefore[2] = InitLemmaForm(formsBefore[2], "alerted_", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    formPtrsBefore[3] = InitLemmaForm(formsBefore[3], "alerts", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN);
    formPtrsBefore[4] = InitLemmaForm(formsBefore[4], "alerts/", FORM_TITLECASE | FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    numberOfFormsBefore = 5;
    // language after lemma (with forms)
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_ENG);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert", FORM_HAS_JOINS, FORM_LEFT_JOIN);
    numberOfFormsBefore = 1;
    // language after lemma (no forms)
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // no language (with forms)
    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_UNK);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert-", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], "alert", FORM_TITLECASE | FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN);
    formPtrsBefore[2] = InitLemmaForm(formsBefore[2], "alerted_", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_LEFT_DELIM | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    formPtrsBefore[3] = InitLemmaForm(formsBefore[3], "alerts", FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN);
    formPtrsBefore[4] = InitLemmaForm(formsBefore[4], "alerts/", FORM_TITLECASE | FORM_HAS_JOINS, FORM_LEFT_JOIN | FORM_RIGHT_JOIN | FORM_RIGHT_DELIM);
    numberOfFormsBefore = 5;
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // no language (no forms)
    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_UNK);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert", FORM_HAS_JOINS, FORM_LEFT_JOIN);
    numberOfFormsBefore = 1;
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestLemmaWithLang() {
    char key[MAXKEY_BUF];
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    int numberOfFormsBefore, numberOfFormsAfter;

    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_ENG);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert");
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], "alert", FORM_TITLECASE);
    formPtrsBefore[2] = InitLemmaForm(formsBefore[2], "alerted");
    formPtrsBefore[3] = InitLemmaForm(formsBefore[3], "alerts");
    formPtrsBefore[4] = InitLemmaForm(formsBefore[4], "alerts", FORM_TITLECASE);
    numberOfFormsBefore = 5;
    // language after lemma (with forms)
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    InitLemmaInfo(lemmaInfoBefore, "", "alert", LANG_ENG);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert");
    numberOfFormsBefore = 1;
    // language after lemma (no forms - actually single form that is equal to lemma): "alert\002\002"
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestLemmaNoLang() {
    char key[MAXKEY_BUF];
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    int numberOfFormsBefore, numberOfFormsAfter;

    // with forms
    InitLemmaInfo(lemmaInfoBefore, "", "alert");
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert");
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], "alert", FORM_TITLECASE);
    formPtrsBefore[2] = InitLemmaForm(formsBefore[2], "alerted");
    formPtrsBefore[3] = InitLemmaForm(formsBefore[3], "alerts");
    formPtrsBefore[4] = InitLemmaForm(formsBefore[4], "alerts", FORM_TITLECASE);
    numberOfFormsBefore = 5;
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // no forms - actually single form that is equal to lemma: "alert"
    InitLemmaInfo(lemmaInfoBefore, "", "alert");
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "alert");
    numberOfFormsBefore = 1;
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestInvalidKeys() {
    TKeyLemmaInfo lemmaInfo;
    char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    // invalid keys
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("?", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("?\x02", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("#", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("#=", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("#=\"", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("#a=", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(-1, DecodeKey("#a=\"", &lemmaInfo, forms));
    // "valid" keys
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey("", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey("##", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey("#=n", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey("#=\"s", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey("(", &lemmaInfo, forms));
    UNIT_ASSERT_VALUES_EQUAL(0, DecodeKey(")", &lemmaInfo, forms));
}

void TKeyCodeTest::TestNonWords() {
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    InitLemmaInfo(lemmaInfoBefore, "##_DOC_IDF_SUM", "");
    CompareResults(0, DecodeKey("##_DOC_IDF_SUM", &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestKeyPrefixes() {
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    InitLemmaInfo(lemmaInfoBefore, "00000000000001AB", "00000000001", LANG_UNK);
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    (void)formPtrsBefore;
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "1", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], "0001", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
    formPtrsBefore[2] = InitLemmaForm(formsBefore[2], "00001", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
    formPtrsBefore[3] = InitLemmaForm(formsBefore[3], "000001", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
//    char key[MAXKEY_BUF];
//    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, 4, formPtrsBefore);
//    Cout << EscapeC(key) << Endl;
    CompareResults(4, DecodeKey("00000000000001AB?00000000001\00101\4\x12`0001\4\022C01\4\022D01\4\x12", &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    char buf[MAXKEY_BUF];
    buf[0] = ATTR_PREFIX;
    ui64 kps = 12345;
    UNIT_ASSERT_VALUES_EQUAL(EncodePrefix(kps, buf + 1), 16);
    UNIT_ASSERT_VALUES_EQUAL(DecodePrefix(buf), kps);
}

void TKeyCodeTest::TestOldKeys() {
    TKeyLemmaInfo lemmaInfo;
    char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    // language before lemma and the previous version of joins in the key
    const int ret = DecodeKey("?\002alert\015%-\001\025\001Eed_\001\025s&/\005", &lemmaInfo, forms);
    UNIT_ASSERT_VALUES_EQUAL(ret, 5);
    UNIT_ASSERT_VALUES_EQUAL((int)lemmaInfo.Lang, (int)LANG_ENG);
    UNIT_ASSERT_STRINGS_EQUAL(lemmaInfo.szLemma, "alert");
    UNIT_ASSERT_STRINGS_EQUAL(forms[0], "alert-\x11");
    UNIT_ASSERT_STRINGS_EQUAL(forms[1], "alert\x11");
    UNIT_ASSERT_STRINGS_EQUAL(forms[2], "alerted_\x11");
    UNIT_ASSERT_STRINGS_EQUAL(forms[3], "alerts");
    UNIT_ASSERT_STRINGS_EQUAL(forms[4], "alerts/\x11");
}

void TKeyCodeTest::TestTokenPrefixes() {
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    (void)formPtrsBefore;
//    char key[MAXKEY_BUF];

    // "$00 ($00 R)"
    InitLemmaInfo(lemmaInfoBefore, "", "$23", LANG_UNK);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "$23", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
//    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, 1, formPtrsBefore);
//    Cout << EscapeC(key) << Endl;
    CompareResults(1, DecodeKey("$23\1#\4\x12", &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    // "00000000000001AB?$00 ($00 R)" // with key prefix
    InitLemmaInfo(lemmaInfoBefore, "00000000000001AB", "$40", LANG_UNK);
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], "$40", FORM_HAS_JOINS, FORM_RIGHT_JOIN);
//    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, 1, formPtrsBefore);
//    Cout << EscapeC(key) << Endl;
    CompareResults(1, DecodeKey("00000000000001AB?$40\1#\4\x12", &lemmaInfoAfter, formsAfter), lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);
}

void TKeyCodeTest::TestFormVersions() {
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];

    CompareResults(
        DecodeKey("00000000000001AB?$40\1#\2\4", &lemmaInfoBefore, formsBefore),
        DecodeKey("00000000000001AB?$40\1#\4\x12", &lemmaInfoAfter, formsAfter),
        lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter, true);

    CompareResults(
        DecodeKey("$23\1#\2\4", &lemmaInfoBefore, formsBefore),
        DecodeKey("$23\1#\4\x12", &lemmaInfoAfter, formsAfter),
        lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter, true);

    CompareResults(
        DecodeKey("00000000000001AB?00000000001\00101\2\4`0001\2\4C01\2\4D01\2\4", &lemmaInfoBefore, formsBefore),
        DecodeKey("00000000000001AB?00000000001\00101\4\x12`0001\4\022C01\4\022D01\4\x12", &lemmaInfoAfter, formsAfter),
        lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter, true);
}

void TKeyCodeTest::TestLongForms() {
    char key[MAXKEY_BUF];
    TKeyLemmaInfo lemmaInfoBefore, lemmaInfoAfter;
    char formsBefore[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF], formsAfter[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrsBefore[N_MAX_FORMS_PER_KISHKA];
    int numberOfFormsBefore, numberOfFormsAfter;

    const char* longForm = "adidasconversenikesalomonreebokpumabalasevicsilluziowol";
    UNIT_ASSERT_VALUES_EQUAL(strlen(longForm), MAXWORD_LEN);

    InitLemmaInfo(lemmaInfoBefore, "", "a");
    formPtrsBefore[0] = InitLemmaForm(formsBefore[0], longForm, FORM_TITLECASE | FORM_HAS_LANG, 0, LANG_ENG);
    formPtrsBefore[1] = InitLemmaForm(formsBefore[1], longForm, FORM_HAS_LANG, 0, LANG_ENG);
    numberOfFormsBefore = 2;
    ConstructKeyWithForms(key, MAXKEY_BUF, lemmaInfoBefore, numberOfFormsBefore, formPtrsBefore);
    numberOfFormsAfter = DecodeKey(key, &lemmaInfoAfter, formsAfter);
    CompareResults(numberOfFormsBefore, numberOfFormsAfter, lemmaInfoBefore, lemmaInfoAfter, formsBefore, formsAfter);

    int length = strlen(formsBefore[0]);
    ui8 flags, joins, lang;
    RemoveFormFlags(formsBefore[0], &length, &flags, &joins, &lang);
    UNIT_ASSERT_VALUES_EQUAL(flags, FORM_TITLECASE | FORM_HAS_LANG);
    UNIT_ASSERT_VALUES_EQUAL(joins, 0);
    UNIT_ASSERT_VALUES_EQUAL(lang, static_cast<ui8>(LANG_ENG));
}


