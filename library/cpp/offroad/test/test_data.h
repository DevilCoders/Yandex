#pragma once

#include <array>

#include <util/generic/vector.h>

#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NOffroad {
    using TTestData = ui64;
    using TTestDataVectorizer = TUi64Vectorizer;
    using TTestDataSubtractor = TD1I1Subtractor;

    TVector<TTestData> MakeTestData(size_t count, size_t seed) {
        TVector<TTestData> result;

        TTestDataVectorizer vectorizer;
        Y_UNUSED(vectorizer); /* This makes MSVC happy. */

        for (size_t i = 0; i < count; i++) {
            seed = seed * 14190712451 + 1;

            std::array<size_t, 2> tmp = {{i, 1 + seed % 101}};

            ui64 hit;
            vectorizer.Gather(tmp, &hit);
            result.push_back(hit);
        }

        return result;
    }

}
