#pragma once

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

#include "test_data.h"

namespace NOffroad {
    TMap<TString, TVector<TTestData>> MakeTestIndex(size_t keyCount, size_t seed) {
        TMap<TString, TVector<TTestData>> result;

        result["a"] = MakeTestData(100, 101);
        result["b"] = MakeTestData(255, 17);
        result["abcd"] = MakeTestData(10000, 997);
        for (size_t i = 0; i < 20; i++)
            result[TString(250, static_cast<char>('a' + i))] = MakeTestData(777, seed + i);
        for (size_t i = 0; i < keyCount; i++)
            result[ToString(i)] = MakeTestData(1, seed + i);

        return result;
    }

}
