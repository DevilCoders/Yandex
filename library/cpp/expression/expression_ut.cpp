#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/regex/pire/regexp.h>

#include <util/stream/output.h>
#include <utility>

#include <util/generic/buffer.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/ylimits.h>

#include <util/random/random.h>

#include "expression.h"

Y_UNIT_TEST_SUITE(TCalcExpressionTest) {
    Y_UNIT_TEST(TestCalcExpression) {
        THashMap<TString, double> m;
        m["mv"] = 32768;
        m["big"] = 68719476736;
        m["small"] = 1;

        UNIT_ASSERT_EQUAL(CalcExpression("1 == 1", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("1 == 0", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("1 > 0", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("(2 - 1) > 0", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("(2 - 1) > 1", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("2 - 2 - 2", m), -2);

        UNIT_ASSERT_EQUAL(CalcExpression("-2 + 2", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("mv&32768==32768", m), 1);

        UNIT_ASSERT_EQUAL(CalcExpression("big&34359738368==big&68719476736", m), 0); //big & (1<<35) != big & 1 << 36
        UNIT_ASSERT_EQUAL(CalcExpression("small&34359738369==1", m), 1);

        UNIT_ASSERT_EQUAL(CalcExpression("(mv&32768==32768)||(mv&32768==32768)||(2 - 2 - 2)||(2 - 2 - 2)&&(mv&32768==32768)", m), 1);

        m["a"] = 0.1;
        m["size"] = 18;
        UNIT_ASSERT_EQUAL(CalcExpression("(a*11) > 1", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("(9*a) > 1 && size > 20", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("2+2*2", m), 6);

        // Sum with boolean
        UNIT_ASSERT_EQUAL(CalcExpression("2+(2==2)", m), 3);
        UNIT_ASSERT_EQUAL(CalcExpression("2+(2==0)", m), 2);
        UNIT_ASSERT_EQUAL(CalcExpression("~big+~small+~azaza", m), 2);
    }

    Y_UNIT_TEST(TestStringExpression) {
        THashMap<TString, TString> m;
        m["a"] = "b";
        m["vm"] = "32768";
        m["qc"] = "1";
        m["l"] = "2.";
        m["version"] = "15.9.3145";
        const TString key = "UPPER.ApplyBlender_MarketExp_exp.#insrule_Vertical/market";
        const TString strVal = "UPPER.ApplyBlender_MarketExp_exp.#insrule_Vertical";
        m[key] = "1";
        UNIT_ASSERT_EQUAL(CalcExpression("'" + key + "'==1", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("\"" + strVal + "\"==" + strVal, m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("vm&32768==32768", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("(qc&1)==1", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("a == b", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("a == a", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("a == c", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("#EXP#l", m), exp(2.));
        UNIT_ASSERT_EQUAL(CalcExpression("#EXP# l + 1", m), exp(2.) + 1);
        UNIT_ASSERT_EQUAL(CalcExpression("#EXP#(l + 1)", m), exp(3.));
        UNIT_ASSERT_EQUAL(CalcExpression("-#EXP#(l + 1)", m), -exp(3.));
        UNIT_ASSERT_EQUAL(CalcExpression("#LOG#l", m), log(2.));
        UNIT_ASSERT_EQUAL(CalcExpression("#LOG# l + 1", m), log(2.) + 1);
        UNIT_ASSERT_EQUAL(CalcExpression("#LOG#(l + 1)", m), log(3.));
        UNIT_ASSERT_EQUAL(CalcExpression("-#LOG#(l + 1)", m), -log(3.));
        UNIT_ASSERT_EQUAL(CalcExpression("#EXP#(-#LOG#(l + 1) + #EXP#(l + 3))", m), exp(-log(3.) + exp(5.)));
        UNIT_ASSERT_EQUAL(CalcExpression("~a", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("~b", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("~a && a == \"b\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("~b && b == \"b\"", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpressionStr("a", m), TString("b"));
        UNIT_ASSERT_EQUAL(CalcExpressionStr("\"c\"", m), TString("c"));

        UNIT_ASSERT_EQUAL(CalcExpressionStr("version", m), TString("15.9.3145"));
        UNIT_ASSERT_EQUAL(CalcExpression("version >@ \"1\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >@ \"15\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >@ \"20\"", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("version <@ \"16.0.00\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >=@ \"15.9\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >=@ \"1.0.0.1\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >=@ \"16.0.00\"", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("version <=@ \"16.2.4.5\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version <=@ \"15.9.3145\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version <=@ \"20\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version <=@ \"10\"", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("version <=@ \"10.0\"", m), 0);

        UNIT_ASSERT_EQUAL(CalcExpression("version >? \"15.9\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >? \"1\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("version >? \"16\"", m), 0);


        UNIT_ASSERT_EQUAL(CalcExpression("#SQR#l", m), pow(2., 2.));
        UNIT_ASSERT_EQUAL(CalcExpression("#SQR# l + 1", m), pow(2., 2) + 1);
        UNIT_ASSERT_EQUAL(CalcExpression("#SQR#(l + 1)", m), pow(3., 2));
        UNIT_ASSERT_EQUAL(CalcExpression("-#SQRT#(l + 1)", m), -pow(3., 0.5));

        UNIT_ASSERT_EXCEPTION(CalcExpressionStr("\"", m), yexception);
        UNIT_ASSERT_EXCEPTION(CalcExpressionStr("qqqqq\"", m), yexception);
        UNIT_ASSERT_EXCEPTION(CalcExpressionStr("\"qqqqq", m), yexception);
        UNIT_ASSERT_EXCEPTION(CalcExpressionStr("\'qqqqq", m), yexception);
        UNIT_ASSERT_EQUAL(CalcExpression("''", m), 0);
        m[""] = "1";
        UNIT_ASSERT_EQUAL(CalcExpression("''", m), 1);
    }

    Y_UNIT_TEST(TestDoubleExpression) {
        THashMap<TString, double> m;
        m["rand"] = 0.0001;
        m["IntentProbability"] = 0.23;
        m["filter"] = 1;
        UNIT_ASSERT_EQUAL(CalcExpression("rand < 0.001", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("filter > 0.5 && rand < 0.001", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("filter > 0.5 && rand < 0.001 && IntentProbability > 0.2", m), 1);
    }

    Y_UNIT_TEST(TestPushBackToVectorUB) {
        THashMap<TString, double> m;
        TString expr = "0";
        const size_t n = 100;
        for (size_t i = 1; i <= n; ++i)
            expr += " + " + ToString(i);
        expr = "(" + expr + ") * (" + expr + ")";
        const size_t sum = n * (n + 1) / 2;
        UNIT_ASSERT_EQUAL(CalcExpression(expr, m), sum * sum);
    }

    Y_UNIT_TEST(TestGetTokens) {
        TExpression expression{"(ab + ac - bc > 0) || c"};
        TVector<TString> tokens;
        expression.GetTokensWithPrefix("a", tokens);
        UNIT_ASSERT_VALUES_EQUAL(tokens.size(), 2);
        expression.GetTokensWithSuffix("c", tokens);
        UNIT_ASSERT_VALUES_EQUAL(tokens.size(), 3);
    }

    Y_UNIT_TEST(TestUnknownExpression) {
        THashMap<TString, double> m;
        m["mv"] = 32768;
        m["big"] = 68719476736;
        m["small"] = 1;

        // test unknown parameter, that is not present in dictionary "m"
        UNIT_ASSERT_EQUAL(CalcExpression("unknown == 1", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown == 0", m), 0);
        // but in sum it has value 0
        UNIT_ASSERT_EQUAL(CalcExpression("unknown + small + mv", m), 32769);
        // in comparisons < or > too
        UNIT_ASSERT_EQUAL(CalcExpression("unknown > 1", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown > 0", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown > (-1)", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown < 1", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown < 0", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("unknown < (-1)", m), 0);
    }

    Y_UNIT_TEST(TestDivZero) {
        THashMap<TString, double> m;
        m["mv"] = 32768.2;
        m["zero"] = 0;

        UNIT_ASSERT_EQUAL(CalcExpressionStr("zero / zero", m), "nan");
        UNIT_ASSERT_EQUAL(CalcExpressionStr("mv / zero", m), "inf");
    }

    Y_UNIT_TEST(TestCond) {
        THashMap<TString, double> m;
        UNIT_ASSERT_EQUAL(CalcExpression("1 ? 2 : 3", m), 2);
        UNIT_ASSERT_EQUAL(CalcExpression("1 ? 1+1 : 3 + 100", m), 2);
        UNIT_ASSERT_EQUAL(CalcExpression("0 ? 0 : 3", m), 3);
        UNIT_ASSERT_EQUAL(CalcExpression("1 ? \"abc.def\" >? \"abc\" : 0", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("1 ? 1 : 0 ? 2 : 3", m), CalcExpression("1 ? 1 : (0 ? 2 : 3)", m));
        UNIT_ASSERT_EQUAL(CalcExpression("1 ? 77 : 2 && 1", m), 77);
    }

    Y_UNIT_TEST(TestText) {
        THashMap<TString, TString> m;
        m["A"] = "a_str";
        m["B"] = "b_str";
        m["C"] = "a_str";
        m["D"] = "";
        UNIT_ASSERT_EQUAL(CalcExpression("A == \"a_str\"", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("A != \"a_str\"", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("A == a_str", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("A != a_str", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("A == A", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("A != A", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("A == B", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("A != B", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("A == C", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("A != C", m), 0);
        UNIT_ASSERT_EQUAL(CalcExpression("~D", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("D == ''", m), 1);
        UNIT_ASSERT_EQUAL(CalcExpression("D != ''", m), 0);
    }

    Y_UNIT_TEST(TestInvalidCond) {
        THashMap<TString, double> m;
        UNIT_CHECK_GENERATED_EXCEPTION(CalcExpression("(2 > 3 ? 41+1) : 666 / 2", m), yexception);
    }

    Y_UNIT_TEST(TestRegex) {
        THashMap<TString, TString> m;
        m["A"] = "a_str";
        m["cyr"] = "кириллица";
        m["RX"] = "._s[a-z]+";
        auto calcExpression = [&m](TStringBuf expr) {
            return TExpression{expr}.SetRegexMatcher([](TStringBuf str, TStringBuf rx) {
                const auto opts = NRegExp::TFsm::TOptions{}.SetCharset(CODES_UTF8);
                return NRegExp::TMatcher{NRegExp::TFsm{rx, opts}}.Match(str).Final();
            }).CalcExpression(m);
        };
        UNIT_ASSERT_EQUAL(calcExpression("A =~ RX"), 1);
        UNIT_ASSERT_EQUAL(calcExpression("A =~ \"b\""), 0);
        UNIT_ASSERT_EQUAL(calcExpression("A =~ \"..s.*\""), 1);
        UNIT_ASSERT_EQUAL(calcExpression("A =~ \".*r\""), 1);
        UNIT_ASSERT_EQUAL(calcExpression("A =~ A"), 1);
        UNIT_ASSERT_EQUAL(calcExpression("cyr =~ \"к.р[и]л*ица\""), 1);
    }

} // TCalcExpressionTest
