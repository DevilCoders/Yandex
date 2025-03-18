#include "table.h"
#include "minhash_builder.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/random/random.h>
#include <util/generic/string.h>
#include <util/generic/map.h>
#include <util/stream/buffer.h>

using namespace NMinHash;

#define RANDOM_MAP_DATA(name, len, seed)                                                                 \
    TMap<TString, ui32> name;                                                                            \
    TVector<TString> name##_v;                                                                           \
    {                                                                                                    \
        for (int i = 0; i < len; ++i) {                                                                  \
            TString val(NUnitTest::RandomString(23, seed + i));                                          \
            name##_v.push_back(val);                                                                     \
            name.insert(std::make_pair(NUnitTest::RandomString(23, seed + i), RandomNumber<ui32>(len))); \
        }                                                                                                \
    }

Y_UNIT_TEST_SUITE(TTableTest) {
    Y_UNIT_TEST(Test1) {
        RANDOM_MAP_DATA(data, 10000, 17);
        TBufferStream sb;
        TChdHashBuilder builder(data.size(), 0.8, 5, 17, 16);
        builder.Build(data_v, &sb);
        TChdMinHashFunc hash(&sb);
        TVector<ui32> vals(data.size());
        for (auto& it : data) {
            size_t pos = hash.Get(it.first.data(), it.first.size());
            UNIT_ASSERT(pos < vals.size());
            vals[pos] = it.second;
        }

        TTable<ui32, TChdMinHashFunc> table;
        table.Add(vals.begin(), vals.end());
        for (TMap<TString, ui32>::iterator it = data.begin(); it != data.end(); ++it) {
            size_t pos = hash.Get(it->first.data(), it->first.size());
            UNIT_ASSERT_VALUES_EQUAL(table[0][pos], it->second);
        }
    }

    Y_UNIT_TEST(Test2) {
        RANDOM_MAP_DATA(data, 10000, 17);
        TBufferStream sb;
        TChdHashBuilder builder(data.size(), 0.8, 5, 17, 16);
        builder.Build(data_v, &sb);
        TAutoPtr<TChdMinHashFunc> hash(new TChdMinHashFunc(&sb));
        TTable<ui32, TChdMinHashFunc> table;
        table.Add(hash.Get(), data.begin(), data.end());
        for (TMap<TString, ui32>::iterator it = data.begin(); it != data.end(); ++it) {
            size_t pos = hash->Get(it->first.data(), it->first.size());
            UNIT_ASSERT_EQUAL(table[0][pos], it->second);
        }
    }
}

#undef RANDOM_MAP_DATA
