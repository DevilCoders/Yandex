#include <library/cpp/proto_config/config.h>

#include <library/cpp/proto_config/ut/test_config.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NProtoConfig;

Y_UNIT_TEST_SUITE(PortoConfig) {
    Y_UNIT_TEST(OverrideRootDefault) {
        TConfig config;
        UNIT_ASSERT_EQUAL(config.GetUInt64(), 42);
        OverrideConfig(config, "UInt64=13");
        UNIT_ASSERT_EQUAL(config.GetUInt64(), 13);
    }

    Y_UNIT_TEST(OverrideLeaf) {
        TConfig config;
        OverrideConfig(config, "Nested.Nested.Int32=1");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetInt32(), 1);
        OverrideConfig(config, "Nested.Nested.UInt32=2");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetUInt32(), 2);
        OverrideConfig(config, "Nested.Nested.Int64=3");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetInt64(), 3);
        OverrideConfig(config, "Nested.Nested.UInt64=4");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetUInt64(), 4);
        OverrideConfig(config, "Nested.Nested.Double=5.05");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetDouble(), 5.05);
        OverrideConfig(config, "Nested.Nested.Float=6.06");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetFloat(), 6.06F);
        OverrideConfig(config, "Nested.Nested.String=seven");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetString(), "seven");
        OverrideConfig(config, "Nested.Nested.String=seven=7");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetString(), "seven=7");
        OverrideConfig(config, "Nested.Nested.String=");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetString(), "");
        OverrideConfig(config, "Nested.Nested.Bool=true");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetBool(), true);
        OverrideConfig(config, "Nested.Nested.Enum=ONE");
        UNIT_ASSERT_EQUAL(config.GetNested().GetNested().GetEnum(), TNestedConfig2_ESomeEnum_ONE);
    }

    Y_UNIT_TEST(ThrowOnBadSyntax) {
        TConfig config;
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.Int321"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "=1"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "="), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, ""), yexception);
    }

    Y_UNIT_TEST(ThrowOnBadKey) {
        TConfig config;
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.BadName=1"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.BadName=1"), yexception);
    }

    Y_UNIT_TEST(ThrowOnBadValue) {
        TConfig config;
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested=0"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.Int32=one"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.Int32=1.01"), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.Int32="), yexception);
        UNIT_ASSERT_EXCEPTION(OverrideConfig(config, "Nested.Nested.Enum=One"), yexception);
    }
}
