#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <kernel/querydata/dump/qd_dump.h>
#include <kernel/querydata/scheme/qd_scheme.h>

#include <library/cpp/testing/unittest/registar.h>

class TQDDumpTest : public TTestBase {
    UNIT_TEST_SUITE(TQDDumpTest)
        UNIT_TEST(TestQueryDataDump)
    UNIT_TEST_SUITE_END();

    void DoTestQueryDataDump(TStringBuf req, const NQueryData::TQueryData& qd, TStringBuf expected, bool values) {
        TStringStream sout;
        NQueryData::DumpQD(sout, req, qd, values);
        UNIT_ASSERT_STRINGS_EQUAL(sout.Str(), expected);
    }

    void DoTestSchemeDump(TStringBuf req, const NSc::TValue& sc, TStringBuf expected, bool values) {
        TStringStream sout;
        NQueryData::DumpSC(sout, req, sc, values);
        UNIT_ASSERT_STRINGS_EQUAL(sout.Str(), expected);
    }

    void TestQueryDataDump() {
        NQueryData::TQueryData qd;
        NQueryData::NTests::BuildQDJson(qd, "{SourceFactors:[{"
                                        "SourceName:src,"
                                        "SourceKey:test,"
                                        "SourceKeyType:doppel,"
                                        "Version:1351186794,"
                                        "Factors:[{Name:sf,StringValue:sfv},{Name:ff,FloatValue:1.5},{Name:if,IntValue:3}]"
                                        "},{"
                                        "SourceName:src,"
                                        "Common:true,"
                                        "Version:1351186794,"
                                        "Factors:[{Name:csf,StringValue:csfv},{Name:cff,FloatValue:2.5},{Name:cif,IntValue:5}]"
                                        "},{"
                                        "SourceName:src,"
                                        "SourceKey:test2,"
                                        "SourceKeyType:doppeltok,"
                                        "Version:1351186794,"
                                        "SourceSubkeys:[{Key:test3,Type:yid},{Key:'*',Type:tld},{Key:'213',Type:region}],"
                                        "Factors:[{Name:tsf,StringValue:tsfv},{Name:tff,FloatValue:3.5},{Name:tif,IntValue:7}]"
                                        "},{"
                                        "SourceName:src,"
                                        "SourceKey:test3,"
                                        "SourceKeyType:docid,"
                                        "Version:1351186794,"
                                        "Json:{a:b,c:d,e:[f,g]}"
                                        "},{"
                                        "SourceName:src,"
                                        "Common:true,"
                                        "Version:1351186794,"
                                        "Json:{z:y,w:x,v:[u,t]}"
                                        "}]}");
        DoTestQueryDataDump("test", qd, "test\tQD\tsrc\tCOMMON_FAC\tcff\tflt:1\n"
                        "test\tQD\tsrc\tCOMMON_FAC\tcif\tint:1\n"
                        "test\tQD\tsrc\tCOMMON_FAC\tcsf\tstr:1\n"
                        "test\tQD\tsrc\tCOMMON_JSON\t{v:[u,t],w:x,z:y}\n"
                        "test\tQD\tsrc\ttest(doppel)\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tff\tflt:1\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tif\tint:1\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tsf\tstr:1\n"
                        "test\tQD\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttff\tflt:1\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttif\tint:1\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttsf\tstr:1\n"
                        "test\tQD\tsrc\ttest3(docid)\n"
                        "test\tQD_JSON\tsrc\ttest3(docid)\t{a:b,c:d,e:[f,g]}\n", false);

        DoTestQueryDataDump("test", qd, "test\tQD\tsrc\tCOMMON_FAC\tcff\tflt:2.5\n"
                        "test\tQD\tsrc\tCOMMON_FAC\tcif\tint:5\n"
                        "test\tQD\tsrc\tCOMMON_FAC\tcsf\tstr:csfv\n"
                        "test\tQD\tsrc\tCOMMON_JSON\t{v:[u,t],w:x,z:y}\n"
                        "test\tQD\tsrc\ttest(doppel)\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tff\tflt:1.5\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tif\tint:3\n"
                        "test\tQD_FAC\tsrc\ttest(doppel)\tsf\tstr:sfv\n"
                        "test\tQD\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttff\tflt:3.5\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttif\tint:7\n"
                        "test\tQD_FAC\tsrc\ttest2(doppeltok)/test3(yid)/*(tld)/213(region)\ttsf\tstr:tsfv\n"
                        "test\tQD\tsrc\ttest3(docid)\n"
                        "test\tQD_JSON\tsrc\ttest3(docid)\t{a:b,c:d,e:[f,g]}\n", true);

        NSc::TValue v;
        NQueryData::QueryData2Scheme(v, qd);

        DoTestSchemeDump("test", v, "test\tSC\tsrc\tcommon\tcff\t1\n"
                        "test\tSC\tsrc\tcommon\tcif\t1\n"
                        "test\tSC\tsrc\tcommon\tcsf\t1\n"
                        "test\tSC\tsrc\tcommon\tv\t[0]\t1\n"
                        "test\tSC\tsrc\tcommon\tv\t[1]\t1\n"
                        "test\tSC\tsrc\tcommon\tw\t1\n"
                        "test\tSC\tsrc\tcommon\tz\t1\n"
                        "test\tSC\tsrc\ttimestamp\t1\n"
                        "test\tSC\tsrc\tvalues\tff\t1\n"
                        "test\tSC\tsrc\tvalues\tif\t1\n"
                        "test\tSC\tsrc\tvalues\tsf\t1\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttff\t1\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttif\t1\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttsf\t1\n"
                        "test\tSC\tsrc\tvalues\ttest3\ta\t1\n"
                        "test\tSC\tsrc\tvalues\ttest3\tc\t1\n"
                        "test\tSC\tsrc\tvalues\ttest3\te\t[0]\t1\n"
                        "test\tSC\tsrc\tvalues\ttest3\te\t[1]\t1\n", false);
        DoTestSchemeDump("test", v, "test\tSC\tsrc\tcommon\tcff\t2.5\n"
                        "test\tSC\tsrc\tcommon\tcif\t5\n"
                        "test\tSC\tsrc\tcommon\tcsf\tcsfv\n"
                        "test\tSC\tsrc\tcommon\tv\t[0]\tu\n"
                        "test\tSC\tsrc\tcommon\tv\t[1]\tt\n"
                        "test\tSC\tsrc\tcommon\tw\tx\n"
                        "test\tSC\tsrc\tcommon\tz\ty\n"
                        "test\tSC\tsrc\ttimestamp\t1351186794\n"
                        "test\tSC\tsrc\tvalues\tff\t1.5\n"
                        "test\tSC\tsrc\tvalues\tif\t3\n"
                        "test\tSC\tsrc\tvalues\tsf\tsfv\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttff\t3.5\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttif\t7\n"
                        "test\tSC\tsrc\tvalues\ttest2\t*\t213\ttsf\ttsfv\n"
                        "test\tSC\tsrc\tvalues\ttest3\ta\tb\n"
                        "test\tSC\tsrc\tvalues\ttest3\tc\td\n"
                        "test\tSC\tsrc\tvalues\ttest3\te\t[0]\tf\n"
                        "test\tSC\tsrc\tvalues\ttest3\te\t[1]\tg\n", true);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQDDumpTest)
