#include "enum_cast_ut.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/event.h>

Y_UNIT_TEST_SUITE(EnumCast) {
    Y_UNIT_TEST(SimpleClass) {
        EClassTest code;
        UNIT_ASSERT(!NCS::TEnumWorker<EClassTest>::TryParseFromInt(3, code));
        UNIT_ASSERT(!NCS::TEnumWorker<EClassTest>::TryParseFromInt(-3, code));
        UNIT_ASSERT(NCS::TEnumWorker<EClassTest>::TryParseFromInt(4, code));
        UNIT_ASSERT(NCS::TEnumWorker<EClassTest>::TryParseFromInt(5, code));
        UNIT_ASSERT(!NCS::TEnumWorker<EClassTest>::TryParseFromInt(6, code));
    }
    Y_UNIT_TEST(Simple) {
        ETest code;
        UNIT_ASSERT(!NCS::TEnumWorker<ETest>::TryParseFromInt(3, code));
        UNIT_ASSERT(!NCS::TEnumWorker<ETest>::TryParseFromInt(-3, code));
        UNIT_ASSERT(NCS::TEnumWorker<ETest>::TryParseFromInt(4, code));
        UNIT_ASSERT(NCS::TEnumWorker<ETest>::TryParseFromInt(5, code));
        UNIT_ASSERT(!NCS::TEnumWorker<ETest>::TryParseFromInt(6, code));
    }
}
