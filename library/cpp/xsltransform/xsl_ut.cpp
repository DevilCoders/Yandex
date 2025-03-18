#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/buffer.h>
#include <util/string/strip.h>
#include <util/stream/str.h>
#include "xsltransform.h"

class TXsltTest: public TTestBase {
    UNIT_TEST_SUITE(TXsltTest);
    UNIT_TEST(TestTransform);
    // UNIT_TEST(TestHtmlOutput);
    UNIT_TEST(TestParam);
    UNIT_TEST(TestFunction);
    UNIT_TEST_SUITE_END();

public:
    void TestTransform();
    void TestHtmlOutput();
    void TestParam();
    void TestFunction();
};
UNIT_TEST_SUITE_REGISTRATION(TXsltTest);

static void Check(TXslTransform& t, const TString& source, const TString& canon) {
    // test TArrayRef<const char> -> TBuffer interface
    {
        TBuffer res;
        t.Transform(TArrayRef<const char>(source.data(), source.size()), res);
        UNIT_ASSERT_VALUES_EQUAL(Strip(TString(res.data(), res.size())), Strip(canon));
    }

    // test IInputStream* -> IOutputStream intefrace
    {
        TStringInput in(source);
        TString result;
        TStringOutput out(result);
        t.Transform(&in, &out);
        UNIT_ASSERT_VALUES_EQUAL(StripInPlace(result), Strip(canon));
    }

    // test TString -> TString interface
    {
        TString result = t(source);
        UNIT_ASSERT_VALUES_EQUAL(StripInPlace(result), Strip(canon));
    }
}

void TXsltTest::TestTransform() {
    TXslTransform transform(
        "<?xml version=\"1.0\"?>\n"
        "<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
        "<xsl:output method=\"xml\" omit-xml-declaration=\"yes\"/>\n"
        "<xsl:template match=\"a\"/>\n"
        "<xsl:template match=\"b\"><xsl:copy-of select=\"node()\"/></xsl:template>\n"
        "<xsl:template match=\"c\"><xsl:copy/></xsl:template>\n"
        "<xsl:template match=\"c[@name]\"><xsl:value-of select=\"@name\"/></xsl:template>\n"
        "</xsl:stylesheet>\n");

    Check(transform, "<a><x/></a>", "");
    Check(transform, "<b><x/></b>", "<x/>");
    Check(transform, "<c><x/></c>", "<c/>");
    Check(transform, "<c name='q'><x/></c>", "q");

    try {
        TXslTransform("zzz");
        UNIT_FAIL("bad xsl");
    } catch (yexception&) {
    }

    try {
        Check(transform, "qqq", "");
        UNIT_FAIL("bad xml for xsl");
    } catch (yexception&) {
    }
}

void TXsltTest::TestHtmlOutput() {
    TXslTransform transform(
        "<?xml version=\"1.0\"?>\n"
        "<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
        "<xsl:output method=\"html\"/>\n"
        "<xsl:template match=\"a\"/>\n"
        "</xsl:stylesheet>\n");
    Check(transform, "<a><x/></a>", "");
}

void TXsltTest::TestParam() {
    TXslTransform transform(
        "<?xml version=\"1.0\"?>\n"
        "<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
        "<xsl:output method=\"xml\" omit-xml-declaration=\"yes\"/>\n"
        "<xsl:param name=\"SomeParam\" select=\"'DefaultValue'\"/>\n"
        "<xsl:template match=\"a\">\n"
        "  <xsl:copy-of select='$SomeParam'/>\n"
        "</xsl:template>\n"
        "</xsl:stylesheet>\n");
    UNIT_ASSERT_VALUES_EQUAL(transform("<a/>"), "DefaultValue\n");

    TXslParams params;
    params["SomeParam"] = "'SomeValue'"; //< note ' - or you will access empty nonexistent <SomeValue> element with no error
    UNIT_ASSERT_VALUES_EQUAL(transform("<a/>", params), "SomeValue\n");
}

// these includes are only required for function
// function type
#include <contrib/libs/libxslt/libxslt/extensions.h>
// macros
#include <libxml/xpathInternals.h>

void xsltFnIncrement(xmlXPathParserContextPtr ctxt, int nargs) {
    // access xslt transformation context
    xsltTransformContextPtr tctxt = xsltXPathGetTransformContext(ctxt);
    Y_UNUSED(tctxt);
    // void *data = NULL; // maybe get my data from context
    CHECK_ARITY(1);

    double arg = xmlXPathPopNumber(ctxt);
    xmlXPathReturnNumber(ctxt, arg + 1);
}

// this struct should be present in util
template <class T, void (*DestroyFun)(T*)>
struct TFunctionDestroy {
    static inline void Destroy(T* t) noexcept {
        if (t)
            DestroyFun(t);
    }
};

typedef THolder<xmlXPathObject, TFunctionDestroy<xmlXPathObject, xmlXPathFreeObject>>
    TxmlXPathObjectHolder;

#include <library/cpp/uri/http_url.h>
void xsltFnUrlRewrite(xmlXPathParserContextPtr ctxt, int nargs) {
    // now avoid xpathInternals
    UNIT_ASSERT_VALUES_EQUAL(nargs, 1);
    UNIT_ASSERT_VALUES_EQUAL(TString((char*)ctxt->context->function), "urlrewrite");
    UNIT_ASSERT_VALUES_EQUAL(TString((char*)ctxt->context->functionURI), "http://arcadia.yandex.ru/xslns");
    TxmlXPathObjectHolder arg1(valuePop(ctxt));
    if (arg1.Get() == nullptr) {
        xmlXPathErr(ctxt, XML_XPATH_INVALID_OPERAND);
        return;
    }
    if (arg1->type != XPATH_STRING) {
        xmlXPathErr(ctxt, XML_XPATH_INVALID_TYPE);
        return;
    }
    const TString argument((char*)arg1->stringval);
    THttpURL url;
    if (url.Parse(argument) != THttpURL::ParsedOK) {
        xmlXPathErr(ctxt, XML_HTTP_URL_SYNTAX);
        return;
    }
    url.Rewrite();
    TString rewritten = url.PrintS() + " =)";

    valuePush(ctxt, xmlXPathNewString((xmlChar*)rewritten.data()));
}

void TXsltTest::TestFunction() {
    {
        TXslTransform transform(
            "<?xml version=\"1.0\"?>\n"
            "<xsl:stylesheet xmlns:xsltest='http://arcadia.yandex.ru/xslns' xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
            "<xsl:output method=\"xml\" omit-xml-declaration=\"yes\"/>\n"
            "<xsl:param name=\"SomeParam\" select=\"'DefaultValue'\"/>\n"
            "<xsl:template match=\"a\">\n"
            "  <xsl:value-of select='xsltest:increment(1)'/>\n"
            "</xsl:template>\n"
            "</xsl:stylesheet>\n");

        transform.SetFunction("increment", "http://arcadia.yandex.ru/xslns", (void*)xsltFnIncrement);
        UNIT_ASSERT_VALUES_EQUAL(transform("<a/>"), "2\n");
    }

    {
        TXslTransform transform(
            "<?xml version=\"1.0\"?>\n"
            "<xsl:stylesheet xmlns:xsltest='http://arcadia.yandex.ru/xslns' xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\" version=\"1.0\">\n"
            "<xsl:output method=\"xml\" omit-xml-declaration=\"yes\"/>\n"
            "<xsl:param name=\"SomeParam\" select=\"'DefaultValue'\"/>\n"
            "<xsl:template match=\"a\">\n"
            "  <xsl:value-of select=\"xsltest:urlrewrite('http://ya.ru/')\"/>\n"
            "</xsl:template>\n"
            "</xsl:stylesheet>\n");

        transform.SetFunction("urlrewrite", "http://arcadia.yandex.ru/xslns", (void*)xsltFnUrlRewrite);
        UNIT_ASSERT_VALUES_EQUAL(transform("<a/>"), "http://ya.ru/ =)\n");
    }
}
