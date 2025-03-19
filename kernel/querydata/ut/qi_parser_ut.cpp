#include <library/cpp/testing/unittest/registar.h>

#include <kernel/querydata/indexer/qi_parser.h>
#include <kernel/querydata/indexer2/qd_record_type.h>
#include <kernel/querydata/indexer2/qd_parser_utils.h>

class TQueryDataParserTest: public TTestBase {
UNIT_TEST_SUITE(TQueryDataParserTest);
    UNIT_TEST(TestFactorIdMapping)
    UNIT_TEST(TestFactorParser)
    UNIT_TEST(TestValueBuilder)
    UNIT_TEST(TestCommonFactorsBuilder)
UNIT_TEST_SUITE_END();
private:

    void TestFactorIdMapping() {
        using namespace NQueryData;
        TFactorIdMapping m;
        UNIT_ASSERT_VALUES_EQUAL(0u, m.MapName("foo"));
        UNIT_ASSERT_VALUES_EQUAL(1u, m.MapName("bar"));
        UNIT_ASSERT_VALUES_EQUAL(2u, m.MapName("baz"));
        UNIT_ASSERT_VALUES_EQUAL(1u, m.MapName("bar"));

        UNIT_ASSERT_VALUES_EQUAL("foo", m.MapId(0));
        UNIT_ASSERT_VALUES_EQUAL("bar", m.MapId(1));
        UNIT_ASSERT_VALUES_EQUAL("baz", m.MapId(2));
    }

    void AssertEmpty(NQueryData::TFactorParser& p) {
        UNIT_ASSERT(!p.Next());
    }

    void AssertFactor(NQueryData::TFactorParser& p, TStringBuf kk, TStringBuf vv, NQueryData::EFactorType tt) {
        UNIT_ASSERT(p.Next());
        UNIT_ASSERT_VALUES_EQUAL(p.GetName(), kk);
        UNIT_ASSERT_VALUES_EQUAL(p.GetValue(), vv);
        UNIT_ASSERT_EQUAL(p.GetType(), tt);
    }

    void TestFactorParser() {
        using namespace NQueryData;
        {
            TFactorParser p;
            p.Init("  \t \t \t\n\n");
            AssertEmpty(p);
        }
        {
            TFactorParser p;
            p.Init("a=b \tfeature:i=333\tfeat:f=2.5\ttest:s=\\u0007\tfff:s= and the history books forgot about us \t\\\\x3A\\x3D\\cool:b=\\\\x3A");
            AssertFactor(p, "a", "b ", FT_STRING);
            AssertFactor(p, "feature", "333", FT_INT);
            AssertFactor(p, "feat", "2.5", FT_FLOAT);
            AssertFactor(p, "test", "\\u0007", FT_STRING);
            AssertFactor(p, "fff", " and the history books forgot about us ", FT_STRING);
            AssertFactor(p, "\\x3A=cool", "\\x3A", FT_STRING);
        }
        {
            TFactorParser p;
            p.Init(" \na=b \t\t feature:i= 333\t feat:f=2.5 \t fff:s= and the history books forgot about us \n\n");
            AssertFactor(p, "a", "b ", FT_STRING);
            AssertFactor(p, "feature", "333", FT_INT);
            AssertFactor(p, "feat", "2.5", FT_FLOAT);
            AssertFactor(p, "fff", " and the history books forgot about us ", FT_STRING);
        }
        {
            TFactorParser p;
            p.Init(" a=z\t b:s=y\t c:i=4 \t d:f=3.5 ");
            AssertFactor(p, "a", "z", FT_STRING);
            AssertFactor(p, "b", "y", FT_STRING);
            AssertFactor(p, "c", "4", FT_INT);
            AssertFactor(p, "d", "3.5", FT_FLOAT);
        }
    }

    void TestValueBuilder() {
        using namespace NQueryData;
        TValueBuilder bld;

        {
            TRawQueryData qd;
            std::pair<TStringBuf, size_t> res = bld.Build(" a=z\t b:s=y\t c:i=4 \t d:f=3.5\td\\0x9d:s=\\0x7\\0x8\\0x9\\0xA ", 1234);
            Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(res.first.data(), res.first.size());
            UNIT_ASSERT_VALUES_EQUAL(64u, res.second);
            UNIT_ASSERT_VALUES_EQUAL(5u, qd.FactorsSize());
            UNIT_ASSERT_VALUES_EQUAL(1234u, qd.GetVersion());
            {
                const TRawFactor& a = qd.GetFactors(0);
                UNIT_ASSERT_VALUES_EQUAL(0u, a.GetId());
                UNIT_ASSERT_VALUES_EQUAL("z", a.GetStringValue());
                UNIT_ASSERT_VALUES_EQUAL("a", bld.Mapping.MapId(0));
            }
            {
                const TRawFactor& b = qd.GetFactors(1);
                UNIT_ASSERT_VALUES_EQUAL(1u, b.GetId());
                UNIT_ASSERT_VALUES_EQUAL("y", b.GetStringValue());
                UNIT_ASSERT_VALUES_EQUAL("b", bld.Mapping.MapId(1));
            }
            {
                const TRawFactor& c = qd.GetFactors(2);
                UNIT_ASSERT_VALUES_EQUAL(2u, c.GetId());
                UNIT_ASSERT_VALUES_EQUAL(4, c.GetIntValue());
                UNIT_ASSERT_VALUES_EQUAL("c", bld.Mapping.MapId(2));
            }
            {
                const TRawFactor& d = qd.GetFactors(3);
                UNIT_ASSERT_VALUES_EQUAL(3u, d.GetId());
                UNIT_ASSERT_VALUES_EQUAL(3.5, d.GetFloatValue());
                UNIT_ASSERT_VALUES_EQUAL("d", bld.Mapping.MapId(3));
            }

            {
                const TRawFactor& d = qd.GetFactors(4);
                UNIT_ASSERT_VALUES_EQUAL(4u, d.GetId());
                UNIT_ASSERT_STRINGS_EQUAL("\\0x7\\0x8\\0x9\\0xA ", d.GetStringValue().data());
                UNIT_ASSERT_STRINGS_EQUAL("d\0x9d", TString{bld.Mapping.MapId(4)}.data());
            }
        }

        {
            TRawQueryData qd;
            std::pair<TStringBuf, size_t> res = bld.Build(" b=zz", 0);
            Y_PROTOBUF_SUPPRESS_NODISCARD qd.ParseFromArray(res.first.data(), res.first.size());
            UNIT_ASSERT_VALUES_EQUAL(9u, res.second);
            UNIT_ASSERT_VALUES_EQUAL(1u, qd.FactorsSize());
            UNIT_ASSERT(!qd.HasVersion());
            {
                const TRawFactor& b = qd.GetFactors(0);
                UNIT_ASSERT_VALUES_EQUAL(1u, b.GetId());
                UNIT_ASSERT_VALUES_EQUAL("zz", b.GetStringValue());
            }
        }
    }

    void TestCommonFactorsBuilder() {
        using namespace NQueryData;
        TCommonFactorsBuilder bld;
        bld.BuildFactors(" a=z\t b:s=y\t c:i=4 \t d:f=3.5 ");
        UNIT_ASSERT_VALUES_EQUAL(25u, bld.Size());
        UNIT_ASSERT_VALUES_EQUAL(4u, bld.Factors.size());
        {
            const TFactor& a = bld.Factors[0];
            UNIT_ASSERT_VALUES_EQUAL("a", a.GetName());
            UNIT_ASSERT_VALUES_EQUAL("z", a.GetStringValue());
        }
        {
            const TFactor& b = bld.Factors[1];
            UNIT_ASSERT_VALUES_EQUAL("b", b.GetName());
            UNIT_ASSERT_VALUES_EQUAL("y", b.GetStringValue());
        }
        {
            const TFactor& c = bld.Factors[2];
            UNIT_ASSERT_VALUES_EQUAL("c", c.GetName());
            UNIT_ASSERT_VALUES_EQUAL(4, c.GetIntValue());
        }
        {
            const TFactor& d = bld.Factors[3];
            UNIT_ASSERT_VALUES_EQUAL("d", d.GetName());
            UNIT_ASSERT_VALUES_EQUAL(3.5, d.GetFloatValue());
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataParserTest)
