#include "ngram.h"
#include "shingler.h"
#include "readable.h"

#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/charset/wide.h>
#include <util/datetime/cputimer.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/stream/format.h>

struct TIsUTF8 {
    static TString Do(const TString& w) {
        return w;
    }
};

struct TWideToUTF8 {
    static TString Do(const TUtf16String& w) {
        return WideToUTF8(w);
    }
};

struct TYandToUTF8 {
    static TString Do(const TString& s) {
        return WideToUTF8(CharToWide(s, csYandex));
    }
};

const TString TestDataLatin1 = "test utf8 shingling in latin1";
const TString TestDataUtf8 = "\xE4\xBD\xA0\xE5\xA5\xBD\x21\x20\x4E\x69\x20\x48\x61\x6F\x3F\x20\xE4\xBD\xA0\xE5\xA5\xBD\xD0\x9D\xD0\xB8\xE4\xBD\xA0\xE5\xA5\xBD\x20\xD0\xA5\xD0\xB0\xD0\xBE\xE4\xBD\xA0\xE5\xA5\xBD\x20\xE4\xBD\xA0\xE5\xA5\xBD\xD0\xBC\xD0\xB0";
const TString TestDataUtf8Lc = "\xE4\xBD\xA0\xE5\xA5\xBD\x21\x20\x6E\x69\x20\x68\x61\x6F\x3F\x20\xE4\xBD\xA0\xE5\xA5\xBD\xD0\xBD\xD0\xB8\xE4\xBD\xA0\xE5\xA5\xBD\x20\xD1\x85\xD0\xB0\xD0\xBE\xE4\xBD\xA0\xE5\xA5\xBD\x20\xE4\xBD\xA0\xE5\xA5\xBD\xD0\xBC\xD0\xB0";
const TString TestDataUtf8Cyr = "\xD0\xB0\x20\xD0\xBF\xD1\x80\xD0\xBE\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB8\xD0\xBC\x2D\xD0\xBA\xD0\xB0\x20\xD0\xBC\xD1\x8B\x20\xD0\xA8\xD0\xAB\xD0\x9D\xD0\x93\xD0\x9B\xD0\xAB\x2E\xD0\x92\xD0\xB4\xD1\x80\xD1\x83\xD0\xB3\x20\xD1\x81\xD0\xBB\xD0\xBE\xD0\xBC\xD0\xB0\xD0\xBB\xD0\xB8\xD1\x81\xD1\x8F\x20\xD1\x83\x20\xD0\xBD\xD0\xB0\xD1\x81";
const TString TestDataUtf8Old = "\xD0\x9F\xD0\xBE\x20\xD0\x92\xD0\xBE\xD0\xBB\xD0\xBE\xD0\xB4\xD0\xB8\xD0\xBC\xD0\xB5\xD1\x80\xD1\xA3\x20\xD0\xB6\xD0\xB5\x20\xD0\xBD\xD0\xB0\xD1\x87\xD1\xA7\x20\xD0\xBA\xD0\xBD\xD1\xA7\xD0\xB6\xD0\xB8\xD1\x82\xD0\xB8\x20\xD0\xA1\xD1\x82\xD2\x83\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xBB\xD0\xBA\xD1\x8A\x2E\x20\xD0\x90\x20\xD0\xBF\xD0\xBE\x20\xD0\xA1\xD1\x82\xD2\x83\xD0\xBE\xD0\xBF\xD0\xBE\xD0\xBB\xD1\x86\xD1\xA3\x20\xD3\x94\xD1\x80\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB0\xD0\xB2";

template <typename TShingler, typename TToUTF8, typename TData>
TString DoTestShingler(const TData& data, size_t minWordLength, bool caseSensitive, bool v) {
    if (v)
        Cerr << "UTF8: " << TToUTF8::Do(data) << Endl;

    typedef typename TShingler::TShingle TShingle;
    TShingler shingler(minWordLength, caseSensitive);

    TVector<TShingle> shingles;
    shingler.ShingleText(shingles, data.data(), data.size());

    MD5 md5;
    for (size_t i = 0; i < shingles.size(); ++i) {
        md5.Update((ui8*)&shingles[i].Value, sizeof(shingles[i].Value));
        if (v)
            Cerr << Sprintf("%016" PRIx64, (ui64)shingles[i].Value) << "\t[" << shingles[i].ToString() << "]" << Endl;
    }
    char out_buf[26];
    md5.End_b64(out_buf);
    return TString(out_buf);
}

template <typename ValueType>
struct TSpoilNothing {
    ValueType operator()(const ValueType& v, size_t) const {
        return v;
    }
};

template <typename ValueType>
struct TSpoilPow2 {
    size_t C;

    TSpoilPow2()
        : C(1)
    {
    }

    ValueType operator()(const ValueType& v, size_t k) {
        if (k > C) {
            return ValueType();
            C = C << 1;
        }
        return v;
    }
};

void MakeNGramTestSequence(size_t n, TVector<i32>& d) {
    TVector<i32> tmp;
    tmp.reserve(n);
    for (size_t i = 0; i < n; i++) {
        i32 v = i;
        if (i % 10 == i / 10)
            v = -1;
        tmp.push_back(v);
    }
    d.swap(tmp);
}

template <typename ProcessorType, typename ValueType, typename SpoilType>
TString ProcessValues(ProcessorType& s, const TVector<ValueType>& d, SpoilType& spoil) {
    MD5 md5;
    typename TVector<ValueType>::const_iterator i = d.begin(), e = d.end();
    for (size_t k = 0; i != e; ++i, ++k) {
        ValueType v = *i;
        v = spoil(v, k);
        s.Update(v);
        ValueType w = s.Get();
        md5.Update((ui8*)&w, sizeof(ValueType));
    }
    char out_buf[26];
    md5.End_b64(out_buf);
    return out_buf;
}

Y_UNIT_TEST_SUITE(TestShingles) {
    Y_UNIT_TEST(WideHashTest) {
        typedef TShingler<TWideReadableCarver<1, THashFirstWideWord<TFNVHash<ui64>>>> TShingler;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, false, true) << Endl;

        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, false, false)), "PPHGI8kiQspMQj/3J8TpNQ==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, false, false)), "KAgoEIccVSw8aRAP3FvTgg==");
    }

    Y_UNIT_TEST(UTF8HashTest) {
        typedef TShingler<TUTF8ReadableCarver<1, THashFirstUTF8Word<TFNVHash<ui64>>>> TShingler;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, false, true) << Endl;

        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, false)), "PPHGI8kiQspMQj/3J8TpNQ==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, false, false)), "KAgoEIccVSw8aRAP3FvTgg==");
    }

    Y_UNIT_TEST(YandHashTest) {
        typedef TShingler<TYandReadableCarver<1, THashFirstYandWord<TFNVHash<ui64>>>> TShingler;
        //~ Cerr << DoTestShingler<TShingler, TYandToUTF8>(WideToChar(UTF8ToWide(TestDataUtf8Cyr), CODES_YANDEX), 2, false, true) << Endl;

        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TYandToUTF8>(WideToChar(UTF8ToWide(TestDataUtf8Cyr), CODES_YANDEX), 2, false, false)), "KAgoEIccVSw8aRAP3FvTgg==");
    }

    Y_UNIT_TEST(UTF8CIHashTest) {
        typedef TShingler<TUTF8ReadableCarver<1, THashFirstUTF8Word<TFNVHash<ui64>>>> TShingler;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Lc, 2, true, true) << Endl;

        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, false)), "PPHGI8kiQspMQj/3J8TpNQ==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Lc, 2, true, false)), "PPHGI8kiQspMQj/3J8TpNQ==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, false)), TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Lc, 2, true, false)));
    }

    Y_UNIT_TEST(NGramExtremumTest) {
        TVector<i32> d;
        MakeNGramTestSequence(45, d);
        TNGramExtremum<10, i32, std::less<i32>> m;
        TSpoilNothing<i32> s;
        UNIT_ASSERT_EQUAL(ProcessValues(m, d, s), "wmKDEaPbSmvgEDlMsUqI6g==");
    }

    Y_UNIT_TEST(WideShinglesTest) {
        typedef TReadableUTF8Shingler TShingler;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, true, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, true, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, false, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, true, true) << Endl;
        //~ Cerr << DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, true, true) << Endl;

        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, false, false)), "omUsiBicF/mPrqNotAYGOw==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, false, false)), "omUsiBicF/mPrqNotAYGOw==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8), 2, true, false)), "zXIo3gG1ETCTVBrwfGzCMg==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8, 2, true, false)), "zXIo3gG1ETCTVBrwfGzCMg==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, false, false)), "MWoplY88KubiYorzWyUfCA==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, false, false)), "MWoplY88KubiYorzWyUfCA==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TWideToUTF8>(UTF8ToWide(TestDataUtf8Cyr), 2, true, false)), "/bPgVfBVCt0j0nFyd2a96A==");
        UNIT_ASSERT_EQUAL(TString(DoTestShingler<TShingler, TIsUTF8>(TestDataUtf8Cyr, 2, true, false)), "/bPgVfBVCt0j0nFyd2a96A==");
    }

        //~ Y_UNIT_TEST(FilteredWideShinglesTest) {
        //~ typedef TShingler<5, TWideShingleTraits, TMinModInWindow<THashedWord<ui64>, 10, 8> > TShingler;
        //~ TString data;
        //~ for (size_t i = 0; i < 10; ++i)
        //~ data += TestDataUtf8 + ToString(i);

        //~ Cerr << DoTestShingler<TShingler>(2,  UTF8ToWide(data), false, true) << Endl;
        //~ UNIT_ASSERT_EQUAL(DoTestShingler<TShingler>(2,  UTF8ToWide(data), false, false), "ivPgXYItRRI5+WwlNcjF+A==");
        //~ }

#if 0
    Y_UNIT_TEST(UTF8Bench) {
        TString data;
        for (size_t i = 0; i < 1000000; ++i)
            data += TestDataUtf8Cyr;

        typedef THashedWordPos<ui64> THashPos;
        {
            TFuncTimer timer("inline");

            size_t pos = 0;
            ui64 check = 0;
            while (pos < +data) {
                THashPos part = THashFirstUTF8Word<TFNVHash<ui64> >::Do(~data + pos, +data - pos, true);
                part.Pos += pos;
                pos = part.Pos + part.Len;
                check ^= part.Hash;
            }
            Cerr << Hex(check) << Endl;
        }

        {
            TFuncTimer timer("convert");

            TUtf16String wide(UTF8ToWide(data));
            size_t pos = 0;
            ui64 check = 0;
            while (pos < +wide) {
                THashPos part = THashFirstUTF8Word<TFNVHash<ui64> >::Do(~wide + pos, +wide - pos, true);
                part.Pos += pos;
                pos = part.Pos + part.Len;
                check ^= part.Hash;
            }
            Cerr << Hex(check) << Endl;
        }
    }
#endif
}
