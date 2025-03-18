#include "sorter.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/digest/md5/md5.h>

struct stru48 {
    char data[6];
    stru48() {
    }
    stru48(ui64 src) {
        memcpy(data, &src, 6);
    }
    stru48(const stru48&) = default;
    stru48& operator=(const stru48&) = default;
    bool operator<(const stru48& w) const {
        return memcmp(data, w.data, 6) < 0;
    }
};

struct stru96 {
    ui32 data[3];
    stru96() {
    }
    stru96(ui64 src) {
        memcpy(data, &src, 8);
        data[2] = data[0] + data[1];
    }
    bool operator<(const stru96& w) const {
        return memcmp(data, w.data, 12) < 0;
    }
};

using namespace NSorter;

template <bool Compressed>
class TSorterTestBase: public TTestBase {
public:
    template <class T, class TSortMode>
    TString SortRandomData() {
        ui64 mydata_seed = 0;
        typedef NSorter::TSorter<T, TSortMode> TSorter;
        TSorter srt(100000, nullptr, Compressed);
        srt.SetFileNameCallback(new NSorter::TPortionFileNameCallback2("unittest"), 0);
        for (int n = 0; n < 345678; n++) {
            srt.PushBack((T)mydata_seed);
            mydata_seed = (mydata_seed + 5678) * 6151;
        }
        for (int n = 0; n < 123456; n++)
            srt.PushBack((T)mydata_seed);
        typename TSortMode::TIterator i(22 * 1024);
        srt.Close(i);
        MD5 md5;
        for (; !i.Finished(); ++i)
            md5.Update((ui8*)&*i, sizeof(T));
        char out_buf[26];
        md5.End_b64(out_buf);
        return {out_buf, 24};
    }
    void testSort32() {
        TString res = SortRandomData<ui32, TSimpleSort<ui32>>();
//UNIT_ASSERT_EQUAL(res == "", true);
#ifdef _little_endian_
        UNIT_ASSERT_VALUES_EQUAL(TString("hQdVu3H5d78LikP6tosBbA=="), res);
#else
        UNIT_ASSERT_VALUES_EQUAL(TString(""), res); // to do!
#endif
    }
    void testSortUniq32() {
        TString res = SortRandomData<ui32, TSmartUniq<ui32>>();
#ifdef _little_endian_
        UNIT_ASSERT_VALUES_EQUAL(TString("fKgxZLzIPaaMUkCq5GQu9g=="), res);
#else
        UNIT_ASSERT_VALUES_EQUAL(TString(""), res); // to do!
#endif
    }
    void testSort48() {
        TString res = SortRandomData<stru48, TSimpleSort<stru48>>();
#ifdef _little_endian_
        UNIT_ASSERT_VALUES_EQUAL(TString("6FN+n+n7JKdHQ+wKHem27w=="), res);
#else
        UNIT_ASSERT_VALUES_EQUAL(TString(""), res); // to do!
#endif
    }
    void testSort64() {
#ifndef _win32_ // этот тест не работает на _win32_ по неизвестным причинам
        TString res = SortRandomData<ui64, TSimpleSort<ui64>>();
#ifdef _little_endian_
        UNIT_ASSERT_VALUES_EQUAL(TString("i6bGEpE0qOjs9N9OoKp/hQ=="), res);
#else
        UNIT_ASSERT_VALUES_EQUAL(TString(""), res); // to do!
#endif
#endif
    }
    void testSort96() {
        TString res = SortRandomData<stru96, TSimpleSort<stru96>>();
#ifdef _little_endian_
        UNIT_ASSERT_VALUES_EQUAL(TString("4v/t41l4lCK00OlCMGX4UA=="), res);
#else
        UNIT_ASSERT_VALUES_EQUAL(TString(""), res); // to do!
#endif
    }
};

class TSorterTest: public TSorterTestBase<false> {
    UNIT_TEST_SUITE(TSorterTest);
    UNIT_TEST(testSort32);
    UNIT_TEST(testSortUniq32);
    UNIT_TEST(testSort48);
    UNIT_TEST(testSort64);
    UNIT_TEST(testSort96);
    UNIT_TEST_SUITE_END();
};
UNIT_TEST_SUITE_REGISTRATION(TSorterTest);

class TCompressedSorterTest: public TSorterTestBase<true> {
    UNIT_TEST_SUITE(TCompressedSorterTest);
    UNIT_TEST(testSort32);
    UNIT_TEST(testSort48);
    UNIT_TEST(testSort64);
    UNIT_TEST(testSort96);
    UNIT_TEST_SUITE_END();
};
UNIT_TEST_SUITE_REGISTRATION(TCompressedSorterTest);
