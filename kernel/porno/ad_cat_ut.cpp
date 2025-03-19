#include "ad_cat.h"

#include <library/cpp/protobuf/util/is_equal.h>
#include <library/cpp/testing/unittest/registar.h>

#include <kernel/porno/proto/ad_cat.pb.h>

Y_UNIT_TEST_SUITE(TAdCatTests) {
    Y_UNIT_TEST(TestAdCatToString) {
        {
            TAdCatProto p;
            p.SetExplicit(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "P");
        }
        {
            TAdCatProto p;
            p.SetPerversion(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "I");
        }
        {
            TAdCatProto p;
            p.SetSensitive(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "S");
        }
        {
            TAdCatProto p;
            p.SetGrey(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "G");
        }
        {
            TAdCatProto p;
            p.SetGruesome(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "R");
        }
        {
            TAdCatProto p;
            p.SetChild(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "C");
        }
        {
            TAdCatProto p;
            p.SetOrdinary(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "O");
        }
        {
            TAdCatProto p;
            p.SetFixlist(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "F");
        }
        {
            TAdCatProto p;
            p.SetChild(true);
            p.SetExplicit(true);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "CP");
            // always sorted
            UNIT_ASSERT_VALUES_UNEQUAL(AdCatToString(p), "PC");
        }
        {
            TAdCatProto p;
            p.SetExplicit(false);
            UNIT_ASSERT_VALUES_EQUAL(AdCatToString(p), "");
        }
    }
    Y_UNIT_TEST(TestAdCatFromString) {
        {
            TAdCatProto p;
            p.SetExplicit(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("P")));
        }
        {
            TAdCatProto p;
            p.SetPerversion(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("I")));
        }
        {
            TAdCatProto p;
            p.SetSensitive(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("S")));
        }
        {
            TAdCatProto p;
            p.SetGrey(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("G")));
        }
        {
            TAdCatProto p;
            p.SetGruesome(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("R")));
        }
        {
            TAdCatProto p;
            p.SetChild(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("C")));
        }
        {
            TAdCatProto p;
            p.SetOrdinary(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("O")));
        }
        {
            TAdCatProto p;
            p.SetFixlist(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("F")));
        }
        {
            TAdCatProto p;
            p.SetChild(true);
            p.SetExplicit(true);
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("CP")));
            UNIT_ASSERT(NProtoBuf::IsEqual(p, AdCatFromString("PC")));
        }
        {
            TAdCatProto p;
            UNIT_ASSERT(!TryAdCatFromString("XYZ", p));
        }
    }
}
