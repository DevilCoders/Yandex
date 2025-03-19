#include "index_format_processor.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/serialized_enum.h>

using namespace NDoom;

Y_UNIT_TEST_SUITE(TestIndexFormatProcessor) {
    Y_UNIT_TEST(TestSimple) {
        const auto& names = GetEnumNames<EIndexFormat>();

        THashSet<TString> touchedFormats;

        TIndexFormatProcessor<> proc;
        for (const auto& p : names) {
            if (p.first == UnknownIndexFormat) {
                continue;
            }
            proc.AddProcessor(p.first, [&, p]{
                touchedFormats.insert(p.second);
            });
        }

        for (const auto& p : names) {
            if (p.first == UnknownIndexFormat) {
                continue;
            }
            UNIT_ASSERT_NO_EXCEPTION(proc.Process(p.first));
        }

        UNIT_ASSERT_VALUES_EQUAL(names.size() - 1, touchedFormats.size());

        for (const auto& p : names) {
            if (p.first == UnknownIndexFormat) {
                continue;
            }
            UNIT_ASSERT(touchedFormats.contains(p.second));
        }
    }
}
