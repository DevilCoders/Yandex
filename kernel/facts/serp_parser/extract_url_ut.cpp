#include "extract_url.h"
#include <library/cpp/resource/resource.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NFactsSerpParser {

    static void CheckFactUrl(const TStringBuf& resourceKey, const TStringBuf& expectedUrl) {
        const TString json = NResource::Find(resourceKey);
        const NSc::TValue value = NSc::TValue::FromJsonThrow(json);

        const TString url = ExtractUrl(value);
        UNIT_ASSERT_EQUAL(url, expectedUrl);
    }

    Y_UNIT_TEST_SUITE(ExtractUrl) {
            Y_UNIT_TEST(CheckUrls) {
                CheckFactUrl("entity_fact", "http://ru.wikipedia.org/wiki/Украина");
                CheckFactUrl("suggest_fact", "https://ru.wikipedia.org/wiki/%D0%90%D0%BC%D0%B0%D0%B7%D0%BE%D0%BD%D0%BA%D0%B0");
            }
    }

} // namespace NFactsSerpParser
