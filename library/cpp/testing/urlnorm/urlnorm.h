#pragma once

#include <functional>
#include <util/generic/string.h>

namespace NUrlNormTest {
    // Try to normalize, returns true on success, false for unacceptable urls
    using TPartialNormalizer = std::function<bool(const TString& url, TString& result)>;
    // Normalizes any url successfully to something
    using TFullNormalizer = std::function<TString(const TString& url)>;

    // This function checks that for a normalization F and any url F(F(url)) == F(url), i.e. idempotency
    void TestProjectorProperty(TPartialNormalizer normalizer);

    inline void TestProjectorProperty(TFullNormalizer normalizer) {
        TestProjectorProperty([&normalizer](const TString& url, TString& result) {
            result = normalizer(url);
            return true;
        });
    }
}
