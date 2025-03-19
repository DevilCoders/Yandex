#include "mask.h"
#include "relev_locale.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NRl;

Y_UNIT_TEST_SUITE(TRelevLocaleMaskTests) {
    Y_UNIT_TEST(TestExclude) {
        TRelevLocaleMask defaultLoc = TRelevLocaleMask(RL_UNIVERSE);
        defaultLoc.Exclude(RL_UZ);
        UNIT_ASSERT(!defaultLoc.Test(RL_UNIVERSE));
        UNIT_ASSERT(!defaultLoc.Test(RL_UZ));
        UNIT_ASSERT(!defaultLoc.Test(RL_XUSSR));
        UNIT_ASSERT(defaultLoc.Test(RL_AZ));

        TRelevLocaleMask defaultLoc2 = TRelevLocaleMask(RL_UNIVERSE);
        defaultLoc2.Exclude(RL_UNIVERSE);
        UNIT_ASSERT(!defaultLoc2.Test(RL_UNIVERSE));
        UNIT_ASSERT(!defaultLoc2.Test(RL_UZ));
        UNIT_ASSERT(!defaultLoc2.Test(RL_XUSSR));
        UNIT_ASSERT(!defaultLoc2.Test(RL_AZ));

        TRelevLocaleMask loc3 = TRelevLocaleMask(RL_XCOM);
        loc3.Exclude(RL_JP);
        UNIT_ASSERT(!loc3.Test(RL_UNIVERSE));
        UNIT_ASSERT(!loc3.Test(RL_UZ));
        UNIT_ASSERT(!loc3.Test(RL_XUSSR));
        UNIT_ASSERT(!loc3.Test(RL_AZ));
    }

    Y_UNIT_TEST(TestExtremeValues) {
        TRelevLocaleMask first = TRelevLocaleMask(RL_UNIVERSE);
        UNIT_ASSERT(first.Test(RL_UNIVERSE));
        first.Exclude(RL_UNIVERSE);
        UNIT_ASSERT(!first.Test(RL_UNIVERSE));

        TRelevLocaleMask last = TRelevLocaleMask(RL_XCOM);
        UNIT_ASSERT(last.Test(RL_XCOM));
        last.Exclude(RL_XCOM);
        UNIT_ASSERT(!last.Test(RL_XCOM));
    }

    Y_UNIT_TEST(TestVariadicTemplatesAddition) {
        TRelevLocaleMask trg;
        UNIT_ASSERT(!trg.Test(RL_UNIVERSE));
        UNIT_ASSERT(!trg.Test(RL_UZ));
        UNIT_ASSERT(!trg.Test(RL_XUSSR));
        UNIT_ASSERT(!trg.Test(RL_AZ));

        trg.Add(RL_AZ, RL_UZ, RL_XUSSR);

        UNIT_ASSERT(!trg.Test(RL_UNIVERSE));
        UNIT_ASSERT(trg.Test(RL_UZ));
        UNIT_ASSERT(trg.Test(RL_XUSSR));
        UNIT_ASSERT(trg.Test(RL_AZ));
        UNIT_ASSERT(trg.Test(RL_KZ));
    }
}
