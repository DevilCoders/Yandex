#include <library/cpp/testing/unittest/registar.h>

#include "disabling_flags.h"

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TDisablingFlags) {
    Y_UNIT_TEST(FalseByDefault) {
        TDisablingFlags flags;

        UNIT_ASSERT(!flags.IsNeverBanForService(EHostType::HOST_WEB));
        UNIT_ASSERT(!flags.IsNeverBlockForService(EHostType::HOST_WEB));
    }

    Y_UNIT_TEST(NeverBlockAll) {
        TDisablingFlags flags;

        AtomicSet(flags.NeverBlock, 1);

        UNIT_ASSERT(!flags.IsNeverBanForService(EHostType::HOST_WEB));
        UNIT_ASSERT(flags.IsNeverBlockForService(EHostType::HOST_WEB));
    }

    Y_UNIT_TEST(NeverBlockByService) {
        TDisablingFlags flags;

        AtomicSet(flags.NeverBlockByService.GetByService(EHostType::HOST_WEB), 1);

        UNIT_ASSERT(!flags.IsNeverBanForService(EHostType::HOST_WEB));
        UNIT_ASSERT(!flags.IsNeverBlockForService(EHostType::HOST_IMAGES));

        UNIT_ASSERT(flags.IsNeverBlockForService(EHostType::HOST_WEB));
    }

    Y_UNIT_TEST(NeverBanAll) {
        TDisablingFlags flags;

        AtomicSet(flags.NeverBan, 1);

        UNIT_ASSERT(!flags.IsNeverBlockForService(EHostType::HOST_WEB));
        UNIT_ASSERT(flags.IsNeverBanForService(EHostType::HOST_WEB));
    }

    Y_UNIT_TEST(NeverBanByService) {
        TDisablingFlags flags;

        AtomicSet(flags.NeverBanByService.GetByService(EHostType::HOST_WEB), 1);

        UNIT_ASSERT(!flags.IsNeverBlockForService(EHostType::HOST_WEB));
        UNIT_ASSERT(!flags.IsNeverBanForService(EHostType::HOST_IMAGES));

        UNIT_ASSERT(flags.IsNeverBanForService(EHostType::HOST_WEB));
    }
}
