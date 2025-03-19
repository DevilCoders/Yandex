#include <library/cpp/testing/unittest/registar.h>

#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/querydata/indexer2/qd_factors_parser.h>
#include <kernel/querydata/indexer2/qd_indexer.h>
#include <kernel/querydata/indexer2/qd_parser_utils.h>
#include <kernel/querydata/indexer2/qd_record_type.h>

#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>

#include <util/generic/strbuf.h>
#include <util/string/builder.h>

class TQueryDataIndexerTest: public TTestBase {
    UNIT_TEST_SUITE(TQueryDataIndexerTest);
        UNIT_TEST(TestUnescapeKey)
        UNIT_TEST(TestParseRecordType)
        UNIT_TEST(TestParseRecord)
        UNIT_TEST(TestFillSourceFactors)
        UNIT_TEST(TestSkipN)
    UNIT_TEST_SUITE_END();
private:

    void DoTestUnescapeKey(TStringBuf key, TStringBuf expkey, int kt, const NQueryData::TKeyTypes& skt, TString& buf) {
        using namespace NQueryData;
        UNIT_ASSERT_STRINGS_EQUAL_C(UnescapeKey(key, kt, skt, buf), expkey, key);
    }

    void TestUnescapeKey() {
        using namespace NQueryData;
        TString buf;
        DoTestUnescapeKey("dGVzdA==", "test", KT_BINARYKEY, TKeyTypes(), buf);
        DoTestUnescapeKey("test", "test", KT_QUERY_EXACT, TKeyTypes(), buf);
        DoTestUnescapeKey("dGVzdDA=\tdGVzdDE=\tdGVzdDI=", "test0\ttest1\ttest2", KT_BINARYKEY, {KT_BINARYKEY, KT_BINARYKEY}, buf);
        DoTestUnescapeKey("test0\tdGVzdDE=\tdGVzdDI=", "test0\ttest1\ttest2", KT_QUERY_EXACT, {KT_BINARYKEY, KT_BINARYKEY}, buf);
        DoTestUnescapeKey("dGVzdDA=\ttest1\tdGVzdDI=", "test0\ttest1\ttest2", KT_BINARYKEY, {KT_QUERY_EXACT, KT_BINARYKEY}, buf);
        DoTestUnescapeKey("dGVzdDA=\tdGVzdDE=\ttest2", "test0\ttest1\ttest2", KT_BINARYKEY, {KT_BINARYKEY, KT_QUERY_EXACT}, buf);
        DoTestUnescapeKey("test0\ttest1\ttest2", "test0\ttest1\ttest2", KT_QUERY_EXACT, {KT_QUERY_EXACT, KT_QUERY_EXACT}, buf);
    }

    void DoTestParseType(TStringBuf dir, NQueryData::TRecordType expected, bool except = false) {
        using namespace NQueryData;
        if (except) {
            UNIT_ASSERT_EXCEPTION(ParseRecordType(dir), yexception);
        } else {
            TRecordType res = ParseRecordType(dir);
            UNIT_ASSERT_EQUAL_C(expected.Mode, res.Mode, dir);
            UNIT_ASSERT_VALUES_EQUAL_C(expected.Timestamp, res.Timestamp, dir);
        }
    }

    void TestParseRecordType() {
        using namespace NQueryData;
        DoTestParseType("#foo;bar", TRecordType());
        DoTestParseType("#query", TRecordType::M_QUERY);
        DoTestParseType("#query;", TRecordType::M_QUERY);
        DoTestParseType("#keyref", TRecordType::M_KEYREF);
        DoTestParseType("#common", TRecordType::M_COMMON);
        DoTestParseType("#query;tstamp=", TRecordType::M_QUERY);
        DoTestParseType("#tstamp;query", TRecordType::M_QUERY);
        DoTestParseType("#query;tstamp=100000", TRecordType(TRecordType::M_QUERY, 100000));
        DoTestParseType("#tstamp=100000;query", TRecordType(TRecordType::M_QUERY, 100000));
        DoTestParseType("#query;tstamp=abc", TRecordType(), true);
    }

    TString PrintRawRecord(const NQueryData::TRawParsedRecord& rec) {
        return TStringBuilder() << rec.Type.ToString() << "|" << rec.Key << "|" << rec.Value;
    }

    void DoTestLocalRecordParse(TStringBuf line, TStringBuf expected, const NQueryData::TIndexerSettings& opts) {
        using namespace NQueryData;
        TRawParsedRecord rec;
        ParseLocalRecord(rec, line, opts);
        UNIT_ASSERT_STRINGS_EQUAL(PrintRawRecord(rec), expected);
    }

    void DoTestLocalRecordReject(TStringBuf line, const NQueryData::TIndexerSettings& opts) {
        using namespace NQueryData;
        TRawParsedRecord rec;
        UNIT_ASSERT_EXCEPTION(ParseLocalRecord(rec, line, opts), yexception);
    }

    void DoTestMRRecordParse(TStringBuf line, TStringBuf expected, const NQueryData::TIndexerSettings& opts) {
        using namespace NQueryData;
        TRawParsedRecord rec;
        TStringBuf key = line.NextTok('|');
        TStringBuf subkey = line.NextTok('|');
        ParseMRRecord(rec, key, subkey, line, opts);
        UNIT_ASSERT_STRINGS_EQUAL(PrintRawRecord(rec), expected);
    }

    void DoTestMRRecordReject(TStringBuf line, const NQueryData::TIndexerSettings& opts) {
        using namespace NQueryData;
        TRawParsedRecord rec;
        TStringBuf key = line.NextTok('|');
        TStringBuf subkey = line.NextTok('|');
        UNIT_ASSERT_EXCEPTION(ParseMRRecord(rec, key, subkey, line, opts), yexception);
    }

    NQueryData::TIndexerSettings BuildSettings(ui32 keysNum, bool useDirectives) {
        NQueryData::TIndexerSettings settings;
        settings.Indexing.Subkeys.resize(keysNum);
        settings.HasDirectives = useDirectives;
        return settings;
    }

    void TestParseRecord() {
        using namespace NQueryData;

        DoTestLocalRecordReject("#query\t", BuildSettings(1, true));
        DoTestLocalRecordReject("#query\t", BuildSettings(2, true));
        DoTestLocalRecordReject("#query\ta", BuildSettings(2, true));
        DoTestLocalRecordReject("#query\ta\t", BuildSettings(2, true));
        DoTestLocalRecordReject("#query\t\tb", BuildSettings(2, true));
        DoTestLocalRecordReject("#query\t\tb\t", BuildSettings(3, true));
        DoTestLocalRecordReject("#query\ta\t\tc", BuildSettings(3, true));
        DoTestLocalRecordParse("#query\ta\tb\tc", "#query|a\tb\tc|", BuildSettings(3, true));
        DoTestLocalRecordParse("#query\ta\tb\tc", "#query|a\tb|c", BuildSettings(2, true));
        DoTestLocalRecordParse("#query\ta\tb\tc", "#query|#query\ta\tb\tc|", BuildSettings(4, false));
        DoTestLocalRecordParse("#query\ta\tb\tc", "#query|#query\ta\tb|c", BuildSettings(3, false));

        DoTestMRRecordReject("||", BuildSettings(1, true));
        DoTestMRRecordReject("\t||", BuildSettings(2, true));
        DoTestMRRecordReject("a||", BuildSettings(2, true));
        DoTestMRRecordReject("a\t||", BuildSettings(2, true));
        DoTestMRRecordReject("\tb||", BuildSettings(2, true));
        DoTestMRRecordReject("\tb\t||", BuildSettings(3, true));
        DoTestMRRecordReject("a\t\tc||", BuildSettings(3, true));
        DoTestMRRecordReject("a\tb\tc||", BuildSettings(2, true));
        DoTestMRRecordParse("a\tb\tc||", "#query|a\tb\tc|", BuildSettings(3, true));

        auto settings = BuildSettings(3, true);
        settings.Indexing.CommonAllowed = true;
        settings.LegacyCommonAllowed = true;

        DoTestLocalRecordParse("#query\t\t\t",    "#common||", settings);
    }

    void DoTestFillSourceFactors(TStringBuf line, TStringBuf expected) {
        using namespace NQueryData;
        TSourceFactors sf;
        FillSourceFactors(sf, line);
        UNIT_ASSERT_JSON_EQ_JSON(NSc::TValue::From(sf), expected);
    }

    void TestFillSourceFactors() {
        DoTestFillSourceFactors("", "");
        DoTestFillSourceFactors("a", "{Factors:[{Name:a,StringValue:''}]}");
        DoTestFillSourceFactors("\t\ta\t\t", "{Factors:[{Name:a,StringValue:''}]}");
        DoTestFillSourceFactors("a\tb", "{Factors:[{Name:a,StringValue:''},{Name:b,StringValue:''}]}");
        DoTestFillSourceFactors("a=xxx\tb=yyy", "{Factors:[{Name:a,StringValue:xxx},{Name:b,StringValue:yyy}]}");
        DoTestFillSourceFactors("a:i=123\tb:f=456", "{Factors:[{Name:a,IntValue:123},{Name:b,FloatValue:456}]}");
        DoTestFillSourceFactors("a:s=xxx\ta:b=\\u0430\\u0431\\u0432",
                                "{Factors:[{Name:a,StringValue:xxx},{Name:a,StringValue:абв}]}");
        DoTestFillSourceFactors("a:s=xxx\t\ta:B=0LDQsdCy",
                                "{Factors:[{Name:a,StringValue:xxx},{Name:a,BinaryValue:абв}]}");
    }

    void TestSkipN() {
        using namespace NQueryData;
        {
            TStringBuf ss = "1.2.3";
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 0, '.'), "");
                UNIT_ASSERT_VALUES_EQUAL(s, "1.2.3");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 1, '.'), "1");
                UNIT_ASSERT_VALUES_EQUAL(s, "2.3");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 2, '.'), "1.2");
                UNIT_ASSERT_VALUES_EQUAL(s, "3");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 3, '.'), "1.2.3");
                UNIT_ASSERT_VALUES_EQUAL(s, "");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 4, '.'), "1.2.3");
                UNIT_ASSERT_VALUES_EQUAL(s, "");
            }
        }
        {
            TStringBuf ss = "..";
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 0, '.'), "");
                UNIT_ASSERT_VALUES_EQUAL(s, "..");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 1, '.'), "");
                UNIT_ASSERT_VALUES_EQUAL(s, ".");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 2, '.'), ".");
                UNIT_ASSERT_VALUES_EQUAL(s, "");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 3, '.'), "..");
                UNIT_ASSERT_VALUES_EQUAL(s, "");
            }
            {
                TStringBuf s = ss;
                UNIT_ASSERT_VALUES_EQUAL(SkipN(s, 4, '.'), "..");
                UNIT_ASSERT_VALUES_EQUAL(s, "");
            }
        }
    }

};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataIndexerTest)
