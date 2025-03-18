#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <library/cpp/digest/old_crc/crc.h>

#include "xmlsax.h"

Y_UNIT_TEST_SUITE(TXmlSaxTest2) {
    class TXmlSaxTestHandler: public TXmlHandlerBase {
    public:
        void SetOut(IOutputStream* out) {
            Out = out;
        }
        void OnDocumentStart() {
            *Out << "start\n";
        }
        void OnDocumentEnd() {
            *Out << "end\n";
        }
        void OnElementStart(const xmlChar* name, const xmlChar** attrs) {
            *Out << "se " << (const char*)name << "\n";
            while (attrs && *attrs) {
                *Out << "attr " << (const char*)*attrs << "\n";
                ++attrs;
            }
        }
        void OnElementEnd(const xmlChar* name) {
            *Out << "ee " << (const char*)name << "\n";
        }
        void OnText(const xmlChar* text, int len) {
            *Out << "text " << TString((const char*)text, len) << "\n";
        }

    private:
        IOutputStream* Out;
    };

    static void DoTest(const char* text, ui64 result, bool allowHtml = false) {
        TXmlSaxParser<TXmlSaxTestHandler> parser;
        TStringStream ss;
        parser.SetOut(&ss);
        TString xml = text;

        parser.Start(allowHtml);
        parser.Parse(xml.data(), xml.size());
        parser.Final();

        //printf("%s\n%s\n", text, ~ss.Str());
        //printf("%" PRIu64 "\n", Crc<ui64>(~ss.Str(), +ss.Str()));
        UNIT_ASSERT_EQUAL(Crc<ui64>(ss.Str().data(), ss.Str().size()), result);
    }

    Y_UNIT_TEST(TestParse) {
        DoTest("<a><el1></el1><el2>text1</el2><el3><el1></el1>text3</el3><el4 qw=\"qwqw\">text5</el4></a>",
               ULL(6968380102647173389));
    }

    Y_UNIT_TEST(TestEntities) {
        DoTest("<tag a=\"&quot;\"><zzz parm=\"a&amp;b\"/>foo bar baz <a href=\"qqq.&#999;.aaa\">&amp; &lt;"
               " &gt; &quot; &amp;amp; xxx yyy</a> zzz</tag>",
               ULL(15032661362651354903));
    }

    Y_UNIT_TEST(TestHtmlEntities) {
        DoTest("<tag>&lt;&copy;&euro;&Aopf;&gt;</tag>", ULL(4893027377418170159), true);
    }

    Y_UNIT_TEST(TestEncodingUtf8) {
        DoTest("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
               "<all>фывапролджэ&#999;</all>\n",
               ULL(16817504169678301321));
    }

    Y_UNIT_TEST(TestEncodingCp1251) {
        DoTest("<?xml version=\"1.0\" encoding=\"windows-1251\"?>\n"
               "<all>\xfe\xff&#999;</all>\n",
               ULL(3676045625846671837));
    }

    Y_UNIT_TEST(TestPartial) {
        TXmlSaxParser<TXmlSaxTestHandler> parser;
        TStringStream ss;
        parser.SetOut(&ss);
        TString xml;
        parser.Start(false);

        xml = "<?xm";
        parser.Parse(xml.data(), xml.size());
        xml = "l version=\"1.0\"?";
        parser.Parse(xml.data(), xml.size());
        xml = ">";
        parser.Parse(xml.data(), xml.size());
        xml = "\n<all>123456789&#";
        parser.Parse(xml.data(), xml.size());
        xml = "999;</a";
        parser.Parse(xml.data(), xml.size());
        xml = "ll>\n";
        parser.Parse(xml.data(), xml.size());

        parser.Final();
        //printf("%s\n", ~ss.Str());
        //printf("%" PRIu64 "\n", Crc<ui64>(~ss.Str(), +ss.Str()));
        UNIT_ASSERT_EQUAL(Crc<ui64>(ss.Str().data(), ss.Str().size()), ULL(13643520744656686979));
    }
}
