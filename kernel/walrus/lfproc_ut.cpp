#include <kernel/search_types/search_types.h>
#include <library/cpp/testing/unittest/registar.h>
#include "lfproc.h"

namespace {

    void CountForms(NIndexerCore::TLemmaAndFormsProcessor& proc, const TNumFormArray& indices, const size_t* count, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            int formIndex = indices[i];
            for (size_t j = 0; j < count[i]; ++j)
                proc.IncreaseFormCount(formIndex);
        }
    }

}

class TLemmaAndFormsProcessorTest : public TTestBase {
    UNIT_TEST_SUITE(TLemmaAndFormsProcessorTest);
        UNIT_TEST(TestNoRearrange);
        UNIT_TEST(TestIntegerAttribute);
        UNIT_TEST(TestRearrange);
    UNIT_TEST_SUITE_END();
public:
    void TestNoRearrange();
    void TestIntegerAttribute();
    void TestRearrange();
};

UNIT_TEST_SUITE_REGISTRATION(TLemmaAndFormsProcessorTest);

using namespace NIndexerCore;

void TLemmaAndFormsProcessorTest::TestNoRearrange() {
    TKeyLemmaInfo lemmaInfo1;
    strcpy(lemmaInfo1.szLemma, "00000000007");
    lemmaInfo1.Lang = LANG_UNK;

    char key1[MAXKEY_BUF];
    const char* forms1[] = { "000007", "007", "07", "7" };
    ConstructKeyWithForms(key1, MAXKEY_BUF, lemmaInfo1, 4, forms1);

    TKeyLemmaInfo lemmaInfo2;
    strcpy(lemmaInfo2.szLemma, "read");
    lemmaInfo2.Lang = LANG_ENG;

    char key2[MAXKEY_BUF];
    const char* forms2[] = { "read\001", "reading", "reads" }; // must be sorted
    ConstructKeyWithForms(key2, MAXKEY_BUF, lemmaInfo2, 3, forms2);

    TKeyLemmaInfo lemmaInfo3;
    strcpy(lemmaInfo3.szLemma, "write");
    lemmaInfo3.Lang = LANG_ENG;

    char key3[MAXKEY_BUF];
    const char* forms3[] = { "write\001", "written", "wrote" }; // must be sorted
    ConstructKeyWithForms(key3, MAXKEY_BUF, lemmaInfo3, 3, forms3);

    char key4[MAXKEY_BUF];
    const char* forms4[] = { "write\001", "writes\001", "writing", "written" }; // must be sorted
    ConstructKeyWithForms(key4, MAXKEY_BUF, lemmaInfo3, 4, forms4);

    const size_t INPUT_KEY_COUNT = 4;
    const char* ikeys[INPUT_KEY_COUNT] = { key1, key2, key3, key4 };
    int testIndices[INPUT_KEY_COUNT][N_MAX_FORMS_PER_KISHKA] = {
        { 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 3, 4, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    const size_t TEST_KEY_COUNT = 3;
    const char* testKeys[TEST_KEY_COUNT] = { "00000000007\001\0257\0227\0217\0207", "read\x1$\x2\x14Ting\x2\x14\x34s\x2\x14", "write\x1%\x2\x14\x35s\x2\x14Ting\x2\x14Tten\x2\x14Rote\x2\x14" };

    TLemmaAndFormsProcessor proc(false, true);
    typedef TVector<TOutputKey> TOutputKeys;
    TOutputKeys okeys;
    TNumFormArray indices;

    size_t i = 0;
    size_t j = 0;
    for (; i < INPUT_KEY_COUNT; ++i) {
        bool res = proc.ProcessNextKey(ikeys[i]);
        if (res) {
            if (proc.GetTotalFormCount()) {
                proc.ConstructOutKeys(okeys, false); // construct key for 'read'
                UNIT_ASSERT(okeys.size() == 1);
                UNIT_ASSERT_STRINGS_EQUAL(okeys[0].Key, testKeys[j]);
                okeys.clear();
                ++j;
            }
            proc.PrepareForNextLemma();
        }

        proc.FindFormIndexes(indices);
        UNIT_ASSERT(memcmp(indices, testIndices[i], sizeof(int) * N_MAX_FORMS_PER_KISHKA) == 0);

        // some additional processing...
    }

    if (proc.GetTotalFormCount()) {
        proc.ConstructOutKeys(okeys, false); // construct key for 'read'
        UNIT_ASSERT(okeys.size() == 1);
        UNIT_ASSERT_STRINGS_EQUAL(okeys[0].Key, testKeys[j]);
        ++j;
    }

    UNIT_ASSERT(i == INPUT_KEY_COUNT && j == TEST_KEY_COUNT);
}

void TLemmaAndFormsProcessorTest::TestIntegerAttribute() {
    const char key[] = "#cat=00001000025";

    TLemmaAndFormsProcessor proc;
    proc.ProcessNextKey(key);

    UNIT_ASSERT(proc.GetTotalFormCount() == 0);
    UNIT_ASSERT(proc.GetCurFormCount() == 0);

    TNumFormArray forms;
    proc.FindFormIndexes(forms);

    UNIT_ASSERT(proc.GetTotalFormCount() == 0);
    UNIT_ASSERT(proc.GetCurFormCount() == 0);

    UNIT_ASSERT(forms[0] == 0);
}

void TLemmaAndFormsProcessorTest::TestRearrange() {
    TKeyLemmaInfo lemmaInfo;
    strcpy(lemmaInfo.szLemma, "a");
    lemmaInfo.Lang = LANG_UNK;

    char key1[MAXKEY_BUF];
    const char* forms1[] = { "aa", "ab", "ac", "ad", "ae", "af", "ag", "ah", "ai", "aj", "ak", "al", "am", "an", "ao" };
    const size_t count1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    ConstructKeyWithForms(key1, MAXKEY_BUF, lemmaInfo, 15, forms1);

    char key2[MAXKEY_BUF];
    const char* forms2[] = { "ab", "ad", "af", "ah", "ap", "aq", "ar", "as", "at", "au", "av", "aw", "ax", "ay", "az" };
    const size_t count2[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    ConstructKeyWithForms(key2, MAXKEY_BUF, lemmaInfo, 15, forms2);

    char key3[MAXKEY_BUF];
    const char* forms3[] = { "aa", "ab", "ac", "ad", "ae", "ak", "al", "am", "an", "ao", "av", "aw", "ax", "ay", "az" };
    const size_t count3[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    ConstructKeyWithForms(key3, MAXKEY_BUF, lemmaInfo, 15, forms3);

    int testIndices[][N_MAX_FORMS_PER_KISHKA] = {
        { 0, 1, 2, 3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 0 },
        { 1, 3, 5, 7, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0 },
        { 0, 1, 2, 3,  4, 10, 11, 12, 13, 14, 21, 22, 23, 24, 25, 0 }
    };

    TLemmaAndFormsProcessor proc;
    TNumFormArray indices;

    bool res = proc.ProcessNextKey(key1);
    UNIT_ASSERT(res);
    proc.PrepareForNextLemma();
    proc.FindFormIndexes(indices);
    UNIT_ASSERT(memcmp(indices, testIndices[0], sizeof(int) * N_MAX_FORMS_PER_KISHKA) == 0);
    CountForms(proc, indices, count1, 15);

    res = proc.ProcessNextKey(key2);
    UNIT_ASSERT(!res);
    proc.FindFormIndexes(indices);
    UNIT_ASSERT(memcmp(indices, testIndices[1], sizeof(int) * N_MAX_FORMS_PER_KISHKA) == 0);
    CountForms(proc, indices, count2, 15);

    res = proc.ProcessNextKey(key3);
    UNIT_ASSERT(!res);
    proc.FindFormIndexes(indices);
    UNIT_ASSERT(memcmp(indices, testIndices[2], sizeof(int) * N_MAX_FORMS_PER_KISHKA) == 0);
    CountForms(proc, indices, count3, 15);

    res = proc.ProcessNextKey("z"); // just a next key
    UNIT_ASSERT(res);

    typedef TVector<TOutputKey> TOutputKeys;
    TOutputKeys okeys;
    proc.ConstructOutKeys(okeys, true);

    UNIT_ASSERT_VALUES_EQUAL(okeys.size(), 2u);

    const TOutputKey& okey0 = okeys[0];
    UNIT_ASSERT_VALUES_EQUAL(okey0.KeyFormCount, 10);
    UNIT_ASSERT_STRINGS_EQUAL(okey0.Key, "a\001\021a\021b\021c\021g\021i\021p\021q\021r\021s\021t");
    TNumFormArray testIndices0 = { 0, 1, 2, 6, 8, 15, 16, 17, 18, 19 };
    UNIT_ASSERT(memcmp(okey0.FormIndexes, testIndices0, sizeof(int) * okey0.KeyFormCount) == 0);

    const TOutputKey& okey1 = okeys[1];
    UNIT_ASSERT_VALUES_EQUAL(okey1.KeyFormCount, 16);
    UNIT_ASSERT_STRINGS_EQUAL(okey1.Key, "a\001\021d\021e\021f\021h\021j\021k\021l\021m\021n\021o\021u\021v\021w\021x\021y\021z");
    TNumFormArray testIndices1 = { 3, 4, 5, 7, 9, 10, 11, 12, 13, 14, 20, 21, 22, 23, 24, 25 };
    UNIT_ASSERT(memcmp(okey1.FormIndexes, testIndices1, sizeof(int) * okey1.KeyFormCount) == 0);
}
