#include "conf.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/vector.h>
#include <util/datetime/base.h>
#include <util/generic/ymath.h>

class TConfTest: public TTestBase {
    UNIT_TEST_SUITE(TConfTest);
    UNIT_TEST(TestFillArray)
    UNIT_TEST(TestReadConf)
    UNIT_TEST(TestGetValue)
    UNIT_TEST(TestTryFillArray)
    UNIT_TEST_SUITE_END();

private:
    struct TDir: public TYandexConfig::Directives {
        TDir()
            : Directives(false)
        {
        }
    };

    class TAnyYConf: public TYandexConfig {
    public:
        bool OnBeginSection(Section& s) override {
            if (!s.Cookie) {
                s.Cookie = new TDir;
                s.Owner = true;
            }
            return true;
        }
    };

    void DoParseConfig(TYandexConfig& c, const TString& in) {
        TString errors;
        const size_t len = in.Size();
        TString inWithGarbage = in + "ещё какой-то мусор за концом буфера";
        TStringBuf buf(inWithGarbage.Data(), len);
        UNIT_ASSERT_C(c.ParseMemory(buf, true), (c.PrintErrors(errors), errors));
    }

    bool DoTestFillArray(const TString& in, const TString& key, const TVector<TString>& expected) {
        TAnyYConf c;
        DoParseConfig(c, in);
        TVector<TString> vs;
        bool res = c.GetRootSection()->GetDirectives().FillArray(key.data(), vs);
        UNIT_ASSERT_VALUES_EQUAL(vs.size(), expected.size());

        for (ui32 i = 0, sz = vs.size(); i < sz; ++i) {
            UNIT_ASSERT_VALUES_EQUAL_C(vs[i], expected[i], i);
        }

        return res;
    }

    void TestFillArray() {
        UNIT_ASSERT(!DoTestFillArray("", "Arr", TVector<TString>()));
        UNIT_ASSERT(DoTestFillArray("Arr", "Arr", TVector<TString>()));
        UNIT_ASSERT(DoTestFillArray("Arr ", "Arr", TVector<TString>()));
        UNIT_ASSERT(DoTestFillArray("Arr a", "Arr", {"a"}));
        UNIT_ASSERT(DoTestFillArray("Arr a\tb  c \td\t  e  ", "Arr", {"a", "b", "c", "d", "e"}));
    }

    void DoTestConf(const TString& in, const TString& out) {
        TAnyYConf c;
        DoParseConfig(c, in);
        TStringStream s;
        c.PrintConfig(s);
        UNIT_ASSERT_VALUES_EQUAL_C(s.Str(), out, in);
    }

    void TestTryFillArray() {
        TString in = "Durations 1s, 32m,   32  , 23 \n";
        in += "Durations1 1s 32m,   32  , 23 \n";
        in += "Durations2 a1s, 32m,   32  , 23 \n";
        in += "lengths 12.6, 33 , 44.33443, 554.2222 \n";
        in += "ids 10|21|11 \n";
        TAnyYConf c;
        DoParseConfig(c, in);
        const auto& main = c.GetRootSection()->GetDirectives();
        TVector<TDuration> durations;
        UNIT_ASSERT_VALUES_EQUAL(main.TryFillArray("Durations1", durations), false);
        UNIT_ASSERT_VALUES_EQUAL(main.TryFillArray("Durations2", durations), false);
        UNIT_ASSERT_VALUES_EQUAL(main.TryFillArray("Durations", durations), true);
        UNIT_ASSERT_VALUES_EQUAL(durations.size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(durations[0], TDuration::Seconds(1));
        UNIT_ASSERT_VALUES_EQUAL(durations[1], TDuration::Minutes(32));
        UNIT_ASSERT_VALUES_EQUAL(durations[2], TDuration::Seconds(32));
        UNIT_ASSERT_VALUES_EQUAL(durations[3], TDuration::Seconds(23));
        TVector<double> lengths;
        UNIT_ASSERT_VALUES_EQUAL(main.TryFillArray("lengths", lengths), true);
        UNIT_ASSERT_VALUES_EQUAL(lengths.size(), 4);
        UNIT_ASSERT(Abs(lengths[0] - 12.6) < 1e-6);
        UNIT_ASSERT(Abs(lengths[1] - 33) < 1e-6);
        UNIT_ASSERT(Abs(lengths[2] - 44.33443) < 1e-6);
        UNIT_ASSERT(Abs(lengths[3] - 554.2222) < 1e-6);
        TVector<ui32> ids;
        UNIT_ASSERT_VALUES_EQUAL(main.TryFillArray("ids", ids, '|'), true);
        UNIT_ASSERT_VALUES_EQUAL(ids.size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(ids[0], 10);
        UNIT_ASSERT_VALUES_EQUAL(ids[1], 21);
        UNIT_ASSERT_VALUES_EQUAL(ids[2], 11);
    }

    void TestGetValue() {
        TString in = ("<Server>\n"
                      "</Server>\n"
                      "<Collection>\n"
                      "   EmptyValueDir\n"
                      "   NonEmptyValueDir qqq\n"
                      "</Collection>\n");
        TAnyYConf c;
        DoParseConfig(c, in);
        TYandexConfig::Section* main = c.GetFirstChild("Collection");
        UNIT_ASSERT(main);

        TYandexConfig::Directives& dir = main->GetDirectives();
        const TString reference = "non-empty";
        TString beforeEmptyValue = reference;
        dir.GetNonEmptyValue("EmptyValueDir", beforeEmptyValue);
        UNIT_ASSERT_VALUES_EQUAL(beforeEmptyValue, reference);

        TString beforeNonEmptyValue = reference;
        dir.GetValue("NonEmptyValueDir", beforeNonEmptyValue);
        UNIT_ASSERT_VALUES_EQUAL(beforeNonEmptyValue, "qqq");

        // test case - insensitive
        beforeNonEmptyValue = reference;
        dir.GetValue("nonEmptyValueDir", beforeNonEmptyValue);
        UNIT_ASSERT_VALUES_EQUAL(beforeNonEmptyValue, "qqq");
    }

    void TestReadConf() {
        DoTestConf("", "");
        DoTestConf("#junk", "");
        DoTestConf("!junk", "");
        DoTestConf(";junk", "");
        DoTestConf("<!--junk-->", "");
        DoTestConf("<section>\n"
                   "</section>",
                   "<section>\n"
                   "</section>\n");
        DoTestConf("Key : val", "Key val\n");
        DoTestConf("Key val", "Key val\n");
        DoTestConf("<Server>\n"
                   "    ServerLog : ./logs/webds-server.log\n"
                   "</Server>\n"
                   "\n"
                   "<Collection id=\"webds\">\n"
                   "    IndexDir : ./indexs/webds-index\n"
                   "    TempDir : ./indexs/webds-temp\n"
                   "    <IndexLog>\n"
                   "        FileName : ./logs/webds-index.log\n"
                   "        Level : moredebug verbose\n"
                   "    </IndexLog>\n"
                   "    <DataSrc id=\"webds\">\n"
                   "        <Webds>\n"
                   "            StartUrls : http://help.yandex.ru\n"
                   "            <Extensions>\n"
                   "                text/html : .html, .htm, .shtml\n"
                   "                text/xml : .xml\n"
                   "                application/pdf : .pdf\n"
                   "                application/msword : .doc\n"
                   "            </Extensions>\n"
                   "        </Webds>\n"
                   "    </DataSrc>\n"
                   "</Collection>\n",
                   "<Server>\n"
                   "ServerLog ./logs/webds-server.log\n"
                   "</Server>\n"
                   "<Collection id=\"webds\">\n"
                   "IndexDir ./indexs/webds-index\n"
                   "TempDir ./indexs/webds-temp\n"
                   "<IndexLog>\n"
                   "FileName ./logs/webds-index.log\n"
                   "Level moredebug verbose\n"
                   "</IndexLog>\n"
                   "<DataSrc id=\"webds\">\n"
                   "<Webds>\n"
                   "StartUrls http://help.yandex.ru\n"
                   "<Extensions>\n"
                   "application/msword .doc\n"
                   "application/pdf .pdf\n"
                   "text/html .html, .htm, .shtml\n"
                   "text/xml .xml\n"
                   "</Extensions>\n"
                   "</Webds>\n"
                   "</DataSrc>\n"
                   "</Collection>\n");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TConfTest);
