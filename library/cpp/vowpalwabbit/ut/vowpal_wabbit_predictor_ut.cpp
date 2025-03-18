#include <library/cpp/vowpalwabbit/vowpal_wabbit_predictor.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TVowpalWabbitPredictorTest) {
    Y_UNIT_TEST(TestNumberHash) {
        TVector<TString> words = {"1", "123", "65432"};
        TVector<ui32> hashes;
        NVowpalWabbit::THashCalcer::CalcHashes(TStringBuf(), words, 1, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(hashes[0], 1);
        UNIT_ASSERT_VALUES_EQUAL(hashes[1], 123);
        UNIT_ASSERT_VALUES_EQUAL(hashes[2], 65432);
    }

    Y_UNIT_TEST(TestWordUnigramHash) {
        TVector<TString> words = {"Привет", "мир"};
        TVector<ui32> hashes;
        NVowpalWabbit::THashCalcer::CalcHashes(TStringBuf(), words, 1, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(hashes[0], 698543816);
        UNIT_ASSERT_VALUES_EQUAL(hashes[1], 3059760018);

        NVowpalWabbit::THashCalcer::CalcHashesWithAppend(TStringBuf(), words, 1, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes, TVector<ui32>({698543816, 3059760018, 698543816, 3059760018}));

        NVowpalWabbit::THashCalcer::CalcHashesWithAppend(TStringBuf(), words, 1, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes, TVector<ui32>({698543816, 3059760018, 698543816, 3059760018, 698543816, 3059760018}));
    }

    Y_UNIT_TEST(TestWordBigramHash) {
        TVector<TString> words = {"to", "be", "or", "not", "to", "be"};
        TVector<ui32> hashes;
        NVowpalWabbit::THashCalcer::CalcHashes(TStringBuf(), words, 2, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes.size(), 11);
        TVector<ui32> expectedHashes = {
            152217691, 2814203010, 117848935, 572399988, 152217691, 2814203010,
            2065459825, 2010478561, 705026623, 1072130047, 2065459825};
        UNIT_ASSERT_VALUES_EQUAL(hashes, expectedHashes);

        NVowpalWabbit::THashCalcer::CalcHashesWithAppend(TStringBuf(), words, 2, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes, TVector<ui32>({
                152217691, 2814203010, 117848935, 572399988, 152217691, 2814203010,
                2065459825, 2010478561, 705026623, 1072130047, 2065459825,
                152217691, 2814203010, 117848935, 572399988, 152217691, 2814203010,
                2065459825, 2010478561, 705026623, 1072130047, 2065459825
        }));
    }

    Y_UNIT_TEST(TestWordBigramHashWithNamespace) {
        TVector<TString> words = {"to", "be", "or", "not", "to", "be"};
        TVector<ui32> hashes;
        NVowpalWabbit::THashCalcer::CalcHashes(TStringBuf("ns"), words, 2, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes.size(), 11);
        TVector<ui32> expectedHashes = {
            1160597812, 933989770, 2852992276, 15632490, 1160597812, 933989770,
            4281040366, 3709447798, 576138542, 3931388406, 4281040366};
        UNIT_ASSERT_VALUES_EQUAL(hashes, expectedHashes);

        NVowpalWabbit::THashCalcer::CalcHashesWithAppend(TStringBuf("ns"), words, 2, hashes);
        UNIT_ASSERT_VALUES_EQUAL(hashes, TVector<ui32>({
            1160597812, 933989770, 2852992276, 15632490, 1160597812, 933989770,
            4281040366, 3709447798, 576138542, 3931388406, 4281040366,
            1160597812, 933989770, 2852992276, 15632490, 1160597812, 933989770,
            4281040366, 3709447798, 576138542, 3931388406, 4281040366
       }));
    }
}
