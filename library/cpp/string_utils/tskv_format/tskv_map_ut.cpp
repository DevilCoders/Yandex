#include "tskv_map.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/random/random.h>

namespace {
    TString GetRandomString(const size_t length) {
        TString result;
        for (size_t i = 0; i < length; ++i) {
            result.push_back(RandomNumber<char>());
        }
        return result;
    }

    struct TStringBufMap : public THashMap<TStringBuf, TStringBuf> {
        TString DeserializeBuffer;
        THashMap<TString, TString> WithTString() const {
            return { begin(), end() };
        }
    };
}

namespace NTskvFormat {
    Y_UNIT_TEST_SUITE(TTskvMapTest) {
        Y_UNIT_TEST(SerializeMapTest) {
            THashMap<TString, TString> test;
            test["key\tas\tkey"] = "value=as=value";
            test["key=as=key"] = "value\tas\tvalue";
            TString res;
            SerializeMap(test, res);
            UNIT_ASSERT_STRINGS_EQUAL(res, "key\\tas\\tkey=value\\=as\\=value\tkey\\=as\\=key=value\\tas\\tvalue");
        }

        Y_UNIT_TEST(DeserializeMapTest) {
            THashMap<TString, TString> result;
            TStringBufMap resultWithBuf;
            THashMap<TString, TString> test;
            test["key\tas\tkey"] = "value=as=value";
            test["key=as=key"] = "value\tas\tvalue";
            test[""] = "";
            test["empty"] = "";
            TStringBuf input = "key\\tas\\tkey=value\\=as\\=value\tkey\\=as\\=key=value\\tas\\tvalue\t=\tempty=";
            DeserializeMap(input, result);
            UNIT_ASSERT_EQUAL(test, result);
            DeserializeMap(input, resultWithBuf);
            UNIT_ASSERT_EQUAL(test, resultWithBuf.WithTString());
        }

        Y_UNIT_TEST(DeserializeMapTestWithoutUnescape) {
            THashMap<TString, TString> result;
            TStringBufMap resultWithBuf;
            THashMap<TString, TString> test;
            test["key\\tas\\tkey"] = "value\\=as\\=value";
            test["key\\=as\\=key"] = "value\\tas\\tvalue";
            test[""] = "";
            test["empty"] = "";
            TStringBuf input = "key\\tas\\tkey=value\\=as\\=value\tkey\\=as\\=key=value\\tas\\tvalue\t=\tempty=";
            DeserializeMap(input, result, false);
            UNIT_ASSERT_EQUAL(test, result);
            DeserializeMap(input, resultWithBuf, false);
            UNIT_ASSERT_EQUAL(test, resultWithBuf.WithTString());
        }

        Y_UNIT_TEST(DeserializeReversibility) {
            TString buffer;
            THashMap<TString, TString> base, result;
            TStringBufMap resultWithBuf;
            for (size_t test = 0; test < 1000; ++test) {
                buffer.clear();
                base.clear();

                const auto size = RandomNumber<size_t>(64) + 1;
                for (size_t i = 0; i < size; ++i) {
                    TString key = GetRandomString(RandomNumber<size_t>(32));
                    TString value = GetRandomString(RandomNumber<size_t>(32));
                    base[key] = value;
                }
                DeserializeMap(SerializeMap(base, buffer), result);
                DeserializeMap(SerializeMap(base, buffer), resultWithBuf);
                UNIT_ASSERT_EQUAL(base, result);
                UNIT_ASSERT_EQUAL(base, resultWithBuf.WithTString());
            }
        }
    }
}
