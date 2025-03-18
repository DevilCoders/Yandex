#include <antirobot/lib/p0f_parser/parser.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TestP0fParser) {
        Y_UNIT_TEST(TestStringToP0fValid) {
            {
                TString text = "4:55+9:0:0:14600,0::df,id+,bad:0";
                TStringStream result;
                result << P0fParser::StringToP0f(text);
                UNIT_ASSERT_STRINGS_EQUAL(text, result.Str());
            }
            {
                TString text = "6:244+11:0:0:43200,0:ts,eol+9,?123:flow,ts2+,opt+,bad:0";
                TStringStream result;
                result << P0fParser::StringToP0f(text);
                UNIT_ASSERT_STRINGS_EQUAL(text, result.Str());
            }
            {
                TString text = "6:53+11:0:8850:mss*4,8:mss,sok,ts,nop,ws:flow:0";
                P0fParser::TP0f p0f = P0fParser::StringToP0f(text);
                UNIT_ASSERT_EQUAL(p0f.Version, 6);

                UNIT_ASSERT_EQUAL(p0f.ObservedTTL, 53);
                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.ITTLDistance), true);
                UNIT_ASSERT_EQUAL(*p0f.ITTLDistance, 11);

                UNIT_ASSERT_EQUAL(p0f.Olen, 0);

                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.MSS), true);
                UNIT_ASSERT_EQUAL(*p0f.MSS, 8850);

                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.WSize), false);
                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.Scale), true);
                UNIT_ASSERT_EQUAL(*p0f.Scale, 8);

                UNIT_ASSERT_EQUAL(p0f.LayoutMSS, true);
                UNIT_ASSERT_EQUAL(p0f.LayoutSOK, true);
                UNIT_ASSERT_EQUAL(p0f.LayoutSACK, false);
                UNIT_ASSERT_EQUAL(p0f.LayoutTS, true);
                UNIT_ASSERT_EQUAL(p0f.LayoutNOP, true);
                UNIT_ASSERT_EQUAL(p0f.LayoutWS, true);
                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.EOL), false);
                UNIT_ASSERT_EQUAL(static_cast<bool>(p0f.UnknownOptionID), false);

                UNIT_ASSERT_EQUAL(p0f.QuirksDF, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksIDp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksIDn, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksECN, false);
                UNIT_ASSERT_EQUAL(p0f.Quirks0p, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksFlow, true);
                UNIT_ASSERT_EQUAL(p0f.QuirksSEQn, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksACKp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksACKn, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksUPTRp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksURGFp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksPUSHFp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksTS1n, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksTS2p, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksOPTp, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksEXWS, false);
                UNIT_ASSERT_EQUAL(p0f.QuirksBad, false);
                UNIT_ASSERT_EQUAL(p0f.PClass, false);
            }
        }
        Y_UNIT_TEST(TestStringToP0fInvalid) {
            {
                TString text = "7:55+9:0:0:14600,0::df,id+,bad:0";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
            {
                TString text = "4:55+9:0:0:14600,0::df,id+,bad:-";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
            {
                TString text = "4:55+9:0:0:14600,0::shpion,df,id+,bad:0";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
            {
                TString text = "4:55+9:0:0:14600,0::,,df,id+,bad:0";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
            {
                TString text = "6:244+11:0:0:43200,0:ts,peol+9:flow,ts2+,opt+,bad:0";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
            {
                TString text = "VersaceOnTheFloor";
                UNIT_CHECK_GENERATED_EXCEPTION(P0fParser::StringToP0f(text), std::exception);
            }
        }
    }
} // namespace NAntiRobot
