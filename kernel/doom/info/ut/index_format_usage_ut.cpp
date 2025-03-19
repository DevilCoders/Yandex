#include <kernel/doom/info/index_format_usage.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/serialized_enum.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(IndexFormatUsageTest) {
    Y_UNIT_TEST(ExistUsageText) {
        const auto& names = GetEnumNames<EIndexFormat>();
        for (const auto& p : names) {
            if (p.first == UnknownIndexFormat) {
                continue;
            }
            UNIT_ASSERT_NO_EXCEPTION(GetIndexFormatUsage(p.first));
        }
    }
}
