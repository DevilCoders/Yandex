#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>
#include "css_sanitize.h"
#include "class_sanitize.h"

using namespace std;

TString Sanitize(TString config, TString css);
TString SanitizeFile(TString config_file, TString css_file);
TString SanitizeInline(TString config, TString inline_css, int& err_count);
TString SanitizeClass(TString config, TString src);

const char* conf_pass_all = "default pass";
const char* conf_deny_all = "default deny";
const char* conf_pass_all_but_expr = "default pass expression deny";
const char* conf_selector_append = "default pass selector append '#blah .blah'";

class TCSSSanitizeTest: public TTestBase {
    UNIT_TEST_SUITE(TCSSSanitizeTest);
    UNIT_TEST(TestComments);
    UNIT_TEST(TestSingleQuote);
    UNIT_TEST(TestDoubleQuote);
    UNIT_TEST(TestDotSelector);
    UNIT_TEST(TestColonSelector);
    UNIT_TEST(TestFunc1);
    UNIT_TEST(TestFunc2);
    UNIT_TEST(TestImport1);
    UNIT_TEST(TestImport2);
    UNIT_TEST(TestMalformed);
    UNIT_TEST(TestExpressionCommentsInside);
    UNIT_TEST(TestCommentsInsideKeyword);
    UNIT_TEST(TestMalformedExpression);
    UNIT_TEST(TestImportant);
    UNIT_TEST(TestValidAtKeyword);
    UNIT_TEST(TestSelectorAppend);
    UNIT_TEST(TestUrl);
    UNIT_TEST(Test1);
    UNIT_TEST(Test2);
    UNIT_TEST(Test3);
    UNIT_TEST(Test4);
    UNIT_TEST(TestDoubleSanitize);
    UNIT_TEST(TestRgbColor);
    UNIT_TEST(TstInline1);
    UNIT_TEST(TstInline2);
    UNIT_TEST(TstInlineExpr1);
    UNIT_TEST(TstInlineExpr2);
    UNIT_TEST(TstInlineExpr3);
    UNIT_TEST(TstInlineBadBigText);
    UNIT_TEST(TstInlineEscape);
    UNIT_TEST(TestDoubleInlineSanitize);
    UNIT_TEST(TestClassDenyAll);
    UNIT_TEST(TestClassDenyOnly);
    UNIT_TEST(TestClassPassAll);
    UNIT_TEST(TestClassPassOnly);
    UNIT_TEST(TestClassEmpty);
    UNIT_TEST(TestClassDoubleSanitize);
    UNIT_TEST(TestVulnerability1);
    UNIT_TEST_SUITE_END();

public:
    void Test1();
    void Test2();
    void Test3();
    void Test4();
    void TestComments();
    void TestSingleQuote();
    void TestDoubleQuote();
    void TestDotSelector();
    void TestColonSelector();
    void TestFunc1();
    void TestFunc2();
    void TestImport1();
    void TestImport2();
    void TestMalformed();
    void TestExpressionCommentsInside();
    void TestCommentsInsideKeyword();
    void TestMalformedExpression();
    void TestImportant();
    void TestValidAtKeyword();
    void TestSelectorAppend();
    void TestDoubleSanitize();
    void TestRgbColor();
    void TestUrl();
    void TstInline1();
    void TstInline2();
    void TstInlineExpr1();
    void TstInlineExpr2();
    void TstInlineExpr3();
    void TstInlineBadBigText();
    void TstInlineEscape();
    void TestDoubleInlineSanitize();
    void TestClassDenyAll();
    void TestClassDenyOnly();
    void TestClassPassAll();
    void TestClassPassOnly();
    void TestClassEmpty();
    void TestClassDoubleSanitize();
    void TestVulnerability1();
};

void TCSSSanitizeTest::TestColonSelector() {
    TString test =
        "a:hover {cursor:pointer;}\n\
body {cursor:pointer;}\n\
body:after {cursor:pointer;}\n\
body:after .link{cursor:pointer;}\
";

    TString canon =
        "a:hover{cursor:pointer;}\n\
body{cursor:pointer;}\n\
body:after{cursor:pointer;}\n\
body:after .link{cursor:pointer;}\n\
";
    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestComments() {
    TString test =
        "a:hover {cursor:pointer;} // comment must be removed\n"
        "body {cursor:pointer;} /* comment must be removed */";

    TString canon =
        "a:hover{cursor:pointer;}\n\
body{cursor:pointer;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestFunc1() {
    TString test =
        "td { background-color:rgb(1);}\n"
        "#b { background-color:rgb(127,127,127) }";

    TString canon =
        "td{background-color:rgb(1);}\n"
        "#b{background-color:rgb(127,127,127);}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestFunc2() {
    TString test =
        "td { background-color:rgb(1);}\n"
        "#a { background-color:rgb();}\n";

    TString canon =
        "td{background-color:rgb(1);}\n"
        "#a{background-color:rgb();}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestImport1() {
    TString test =
        "@import \"/style/print.css\" print;\n"
        "@import url(style/test.css);\n"
        "@import url(\"style/test.css\");\n"
        "\n"
        "td { width: 40; }";

    TString canon =
        "@import url(\"/style/print.css\") print;\n"
        "@import url(\"style/test.css\");\n"
        "@import url(\"style/test.css\");\n"
        "td{width:40;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestImport2() {
    TString test =
        "@import \"/style/print.css\" print;\n"
        "@import url(style/test.css);\n"
        "@import url(\"style/test.css\");\n"
        "\n"
        "td { width: 40; }";

    TString canon = "";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_deny_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestMalformed() {
    TString test =
        "p { color:green }\n"
        "p { color:green; color }  /* malformed declaration missing ':', value */\n"
        "p { color:red;   color; color:green }  /* same with expected recovery */\n"
        "p { color:green; color: } /* malformed declaration missing value */\n"
        "p { color:red;   color:; color:green } /* same with expected recovery */\n"
        "p { color:green; color{;color:maroon} } /* unexpected tokens { } */\n"
        "p { color:red;   color{;color:maroon}; color:green } /* same with recovery */\n";

    TString canon =
        "p{color:green;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestImportant() {
    TString test = "vvv b{color:#f00 !important}";
    TString canon = "vvv b{color:#f00 !important;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestValidAtKeyword() {
    TString test =
        "@media print, screen {\n\
    h1 { color: red }\n\
    body { font-size: 10pt }\n\
}\n\
@page :left {\n\
  margin-left: 4cm;\n\
  margin-right: 3cm;\n\
}";

    TString canon =
        "@media print,screen\n\
{\n\
h1{color:red;}\n\
body{font-size:10pt;}\n\
}\n\
@page :left {\
margin-left:4cm;\
margin-right:3cm;\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
};

void TCSSSanitizeTest::TestSelectorAppend() {
    TString test =
        "td .class1 id1#a id2, div.class2 { width: 40 }";

    TString canon =
        "#blah .blah td .class1 id1#a id2,#blah .blah div.class2{width:40;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_selector_append, test).c_str(),
        canon.c_str());
}

const char* common_css =
    "selector1 {\n \
    common_prop1 : 10; \n\
    common_prop2 : 11; \n\
    sel1_prop1 : 12px; \n\
    sel1_prop2 : 13px; \n\
}\n\
selector2 {\n \
    common_prop1 : 14; \n\
    common_prop2 : 15; \n\
    sel2_prop1 : 16px; \n\
    sel2_prop2 : 17px; \n\
}";

void TCSSSanitizeTest::Test1() {
    const char* conf = "default pass\nproperty deny ( common_prop1 )";

    TString canon =
        "selector1{common_prop2:11;\
sel1_prop1:12px;\
sel1_prop2:13px;\
}\n\
selector2{\
common_prop2:15;\
sel2_prop1:16px;\
sel2_prop2:17px;\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf, common_css).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::Test2() {
    const char* conf = "default pass property deny ( /common_prop.*/ )";

    TString canon =
        "selector1{\
sel1_prop1:12px;\
sel1_prop2:13px;\
}\n\
selector2{\
sel2_prop1:16px;\
sel2_prop2:17px;\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf, common_css).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::Test3() {
    const char* conf = "default deny property pass ( /common_prop.*/ )";

    TString canon =
        "selector1{\
common_prop1:10;\
common_prop2:11;\
}\n\
selector2{\
common_prop1:14;\
common_prop2:15;\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf, common_css).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::Test4() {
    const char* conf = "default deny property pass ( /common_prop.*/ { value <15 } )";

    TString canon =
        "selector1{\
common_prop1:10;\
common_prop2:11;\
}\n\
selector2{\
common_prop1:14;\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf, common_css).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestDotSelector() {
    TString test =
        " span.link { cursor : pointer }\n\
 span .link {  cursor : pointer ; } ";

    TString canon =
        "span.link{cursor:pointer;}\n\
span .link{cursor:pointer;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestSingleQuote() {
    TString test =
        "td {\n\
    background-color: url ( 'http://blah-blah' );\n\
}";

    TString canon =
        "td{\
background-color:url(http://blah-blah);\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestUrl() {
    TString test;
    TString canon;

    test =
        "td {\n\
    background-color: url(path/to);\n\
}";

    canon =
        "td{\
background-color:url('path/to');\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());

    test =
        "td {\n\
    background-color: url('path/to');\n\
}";

    canon =
        "td{\
background-color:url('path/to');\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());

    test =
        "td {\n\
    background-color: url(\"path/to\");\n\
}";

    canon =
        "td{\
background-color:url('path/to');\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());

    test =
        "td {\n\
    background-color: url('\x7f\x19\x20');\n\
}";

    canon =
        "td{\
background-color:url(' ');\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestDoubleQuote() {
    TString test =
        "td {\n\
    background-color: url ( \"http://blah-blah\" );\n\
}";

    TString canon =
        "td{\
background-color:url(http://blah-blah);\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}
void TCSSSanitizeTest::TestExpressionCommentsInside() {
    TString test =
        "td {\n\
    background-color: expression ( /* comments inside must be cut*/ alert (\"ewere\"); );\n\
}";

    TString canon =
        "td{\
background-color:expression( alert (\"ewere\"); );\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestCommentsInsideKeyword() {
    TString test =
        "td {\n\
    background-color: u/*XSS*/rl ( 'http://blah-blah' );\n\
}";

#if 0
    TString canon =
"td{\
background-color:url('http://blah-blah');\
}\n";
#else
    TString canon = "";
#endif
    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}
void TCSSSanitizeTest::TestMalformedExpression() {
    TString test =
        "td {\n\
    height: 10px;\n\
    background-color: expression ( alert (\"ewere\"); );\n\
    color: \\expr\\00045\\ssio\\n( (new Date()).getHours(%2 ? \"#B8D4FF\" : \"#F08A00\" );\n\
    width:10px; // это свойство будет удалено, т.к. предшествующий color: expression содержит синтакс. ошибку \n\
}\n\
\n\
div.class1 {\n\
    height: 10px;\n\
    background-color: expression ( alert (\"ewere\"); );\n\
    color: \\expr\\00045\\ssio\\n( (new Date()).getHours(%2 ? \"#B8D4FF\" : \"#F08A00\" ))\n\
}";

    TString canon =
        "td{\
height:10px;\
background-color:expression(alert (\"ewere\"); );\
color:expression();\
}\n\
div.class1\
{\
height:10px;\
background-color:expression(alert (\"ewere\"); );\
color:expression((new Date()).getHours(%2 ? \"#B8D4FF\" : \"#F08A00\" ));\
}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        Sanitize(conf_pass_all, test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestDoubleSanitize() {
    using namespace Yandex::NCssSanitize;

    TString config = conf_pass_all;
    TString test;
    TString canon;

    TCssSanitizer ss;
    ss.OpenConfigString(config);

    test = "td{ height:10px}";
    canon = "td{height:10px;}\n";

    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());

    test = "div { width: left } ";
    canon = "div{width:left;}\n";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());

    test = "h{size: 12};";
    canon = "h{size:12;}\n";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestRgbColor() {
    using namespace Yandex::NCssSanitize;

    TString config;

    TString test;
    TString canon;

    {
        config = "default deny property pass(color)";
        test = "td{ color: rgb(12,34,234)}";
        canon = "td{color:rgb(12,34,234);}\n";

        TCssSanitizer ss;
        ss.OpenConfigString(config);

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Sanitize(test).c_str(),
            canon.c_str());
    }

    {
        config = "default pass property deny (color)";
        test = "td{ color: rgb(12,34,234); width: 40}";
        canon = "td{width:40;}\n";

        TCssSanitizer ss;
        ss.OpenConfigString(config);

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Sanitize(test).c_str(),
            canon.c_str());
    }

    {
        config = "default deny property pass (color)";
        test = "td{ color: rgb(12%, '34&#127;%', 33%); width: 40}";
        canon = "td{color:rgb(12%,\"34%\",33%);}\n";

        TCssSanitizer ss;
        ss.OpenConfigString(config);

        UNIT_ASSERT_STRINGS_EQUAL(
            ss.Sanitize(test).c_str(),
            canon.c_str());
    }
}

void TCSSSanitizeTest::TstInline1() {
    TString test = " background-width: 35px; background-color:black";
    TString result = "";

    int err_count = 0;
    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_deny_all, test, err_count).c_str(),
        result.c_str());
}
void TCSSSanitizeTest::TstInline2() {
    TString test = "background-width:31px;  background-color:black;height:23px";
    TString canon = "background-width:31px;background-color:black;height:23px;";

    int err_count = 0;
    TString result = SanitizeInline(conf_pass_all, test, err_count);

    UNIT_ASSERT_EQUAL_C(err_count, 0, "There was parse errors");
    UNIT_ASSERT_STRINGS_EQUAL(result.c_str(), canon.c_str());
}

void TCSSSanitizeTest::TstInlineExpr1() {
    TString test = " background-width: 35px; background-color:black;-ie-width: expression(\"window.height+23\")";
    TString result = "background-width:35px;background-color:black;";

    int err_count = 0;

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all_but_expr, test, err_count).c_str(),
        result.c_str());
}

void TCSSSanitizeTest::TstInlineExpr2() {
    TString test = " background-width: 35px; background-color:black;-ie-width: expression(window.height+23)";
    TString result = "background-width:35px;background-color:black;";

    int err_count = 0;
    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all_but_expr, test, err_count).c_str(),
        result.c_str());
}

void TCSSSanitizeTest::TstInlineExpr3() {
    TString test = " background-width: 35px; background-color:black;-ie-width: expression('window.height+23')";
    TString result = "background-width:35px;background-color:black;-ie-width:expression('window.height+23');";

    int err_count = 0;
    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all, test, err_count).c_str(),
        result.c_str());
}

void TCSSSanitizeTest::TstInlineBadBigText() {
    TString test = "background: url(";

    /* Generate big string, so lexer will fail with message
       'input buffer overflow, can't enlarge buffer because scanner uses REJECT'
    */
    for (int i = 0; i < 10000; i++)
        test += "&quot;";

    test += ") no-repeat scroll 100% 50% transparent;";

    TString result = "";

    int err_count = 0;
    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all, test, err_count).c_str(),
        result.c_str());
}

void TCSSSanitizeTest::TestDoubleInlineSanitize() {
    using namespace Yandex::NCssSanitize;

    TString config = conf_pass_all;
    TString test;
    TString canon;

    TCssSanitizer ss;
    ss.OpenConfigString("default pass");

    test = "width: 34";
    canon = "width:34;";

    UNIT_ASSERT_STRINGS_EQUAL(
        ss.SanitizeInline(test).c_str(),
        canon.c_str());

    test = " height: 23 ";
    canon = "height:23;";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.SanitizeInline(test).c_str(),
        canon.c_str());

    test = "height:56";
    canon = "height:56;";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.SanitizeInline(test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TstInlineEscape() {
    TString test;
    TString result;

    test = "background-image: url(home/path)";
    result = "background-image:url('home/path');";

    int err_count = 0;

    test = "prop: url('abcde&#32;er')";
    result = "prop:url('abcde&#32%59er');";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all, test, err_count).c_str(),
        result.c_str());
    test = " background: url('/some/&#127;;left:0;z-index:998;background:white;opacity:0.9;width:100%;height:100%;position:fixed;top:0;/*picture.jpg');";
    result = "background:url('/some/%59%59left:0%59z-index:998%59background:white%59opacity:0.9%59width:100%%59height:100%%59position:fixed%59top:0%59/*picture.jpg');";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(conf_pass_all, test, err_count).c_str(),
        result.c_str());
}

const char* test_class_string =
    "class1 class2 -class\n\
        lass class-1 class-deny -class-23 ya-class1 class8 -ya-class23";

void TCSSSanitizeTest::TestClassDenyAll() {
    TString config = "default deny";
    TString canon = "";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeClass(config, test_class_string).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestClassDenyOnly() {
    TString config =
        "default pass \
 \
class deny \
( \
    class1 \
    class2 \
    class-deny \
    /^ya-.+/ \
)";

    TString canon = "-class lass class-1 -class-23 class8 -ya-class23";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeClass(config, test_class_string).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestClassPassAll() {
    TString config = "default pass";
    TString canon =
        "class1 class2 -class lass class-1 class-deny -class-23 ya-class1 class8 -ya-class23";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeClass(config, test_class_string).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestClassPassOnly() {
    TString config =
        "default deny\
 \
class pass \
( \
    class1 \
    class2 \
    /ya-.+/ \
)";
    TString canon = "class1 class2 ya-class1 -ya-class23";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeClass(config, test_class_string).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestClassEmpty() {
    TString config = " ";
    TString canon = "";

    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeClass(config, test_class_string).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestClassDoubleSanitize() {
    using namespace Yandex::NCssSanitize;

    TString config = conf_pass_all;
    TString test;
    TString canon;

    TClassSanitizer ss;
    ss.OpenConfigString("default pass");

    test = "class1";
    canon = "class1";

    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());

    test = " class2 ";
    canon = "class2";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());

    test = "class3";
    canon = "class3";
    UNIT_ASSERT_STRINGS_EQUAL(
        ss.Sanitize(test).c_str(),
        canon.c_str());
}

void TCSSSanitizeTest::TestVulnerability1() {
    TString config = "default pass";
    TString input = "image:url(\"\
      ');position:fixed;top:-1%;left:-1%;z-index:999999999999999;after:\"\
      lol \";on=op(\"// //op);";

    TString canon = "image:url('      %39%41%59position:fixed%59top:-1%%59left:-1%%59z-index:999999999999999%59after:%34      lol %34%59on=op%40%34// //op');";

    int error_count = 0;
    UNIT_ASSERT_STRINGS_EQUAL(
        SanitizeInline(config, input, error_count).c_str(),
        canon.c_str());
}

TString Sanitize(TString config, TString css) {
    using namespace Yandex::NCssSanitize;

    TCssSanitizer ss;

    ss.OpenConfigString(config);

    return ss.Sanitize(css);
}

TString SanitizeFile(TString config_file, TString css_file) {
    using namespace Yandex::NCssSanitize;

    TCssSanitizer ss;

    ss.OpenConfig(config_file);

    return ss.SanitizeFile(css_file);
}

TString SanitizeInline(TString config, TString inline_css, int& error_count) {
    using namespace Yandex::NCssSanitize;

    //TUnbufferedFileOutput f("out.log" );
    //f << "Sanitizing test: " << inline_css << "\n"
    //    << "\nResult: ";

    TCssSanitizer ss;

    ss.OpenConfigString(config);

    TString res = ss.SanitizeInline(inline_css);

    //f << res << "\n";
    error_count = ss.GetErrorCount();

    return res;
}

TString SanitizeClass(TString config, TString src) {
    using namespace Yandex::NCssSanitize;

    TClassSanitizer ss;
    ss.OpenConfigString(config);

    return ss.Sanitize(src);
}

UNIT_TEST_SUITE_REGISTRATION(TCSSSanitizeTest);
