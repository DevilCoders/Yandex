#include "named_lock.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(NamedLockSuite) {
    Y_UNIT_TEST(Simple) {
        TString cat = "cat";
        TString dog = "dog";
        {
            auto first = NNamedLock::TryAcquireLock(cat);
            UNIT_ASSERT(first);

            auto second = NNamedLock::TryAcquireLock(dog);
            UNIT_ASSERT(second);

            auto third = NNamedLock::TryAcquireLock(cat);
            UNIT_ASSERT(!third);
        }
        {
            auto first = NNamedLock::AcquireLock(cat);
            UNIT_ASSERT(first);

            auto second = NNamedLock::AcquireLock(dog);
            UNIT_ASSERT(second);

            auto third = NNamedLock::TryAcquireLock(dog);
            UNIT_ASSERT(!third);
        }
    }
}
