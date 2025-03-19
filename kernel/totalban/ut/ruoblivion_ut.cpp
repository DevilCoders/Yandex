#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/totalban/ruoblivion.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/str.h>

namespace NTotalBan {
    class TRuOblivionTest: public TTestBase {
        UNIT_TEST_SUITE(TRuOblivionTest);
            UNIT_TEST(TestMerge)
            UNIT_TEST(TestSimpleMatch);
            UNIT_TEST(TestSubsetMatch);
            UNIT_TEST(TestDoubleNameMatch);
            UNIT_TEST(TestFlection)
        UNIT_TEST_SUITE_END();
    public:
        TString DoFindRuOblivionMatch(TStringBuf params, TStringBuf query) {
            TLangMask langs = NLanguageMasks::DefaultRequestLanguages();
            TRichTreeConstPtr tree = CreateRichTree(UTF8ToWide(query), TCreateTreeOptions(), &langs, &langs);
            auto res = FindRuOblivionMatch(params, tree);
            return res ? TString::Join(res.QueryMatch, "/", ToString(res.FlagRTBF)) : TString();
        }

        void TestMerge() {
            UNIT_ASSERT_STRINGS_EQUAL(MergeRuOblivionParameters({
                    "names=маша+григорьева;flags=RTBF,Foo", "names=варвара+лесных;flags=RTBF"}),
                    "names=варвара+лесных,маша+григорьева;flags=Foo,RTBF");
            UNIT_ASSERT_STRINGS_EQUAL(MergeRuOblivionParameters({
                    "names=маша+григорьева;flags=RTBF,Foo", "names=варвара+лесных;names=руслан+ковалев;flags=RTBF"}),
                    "names=варвара+лесных,маша+григорьева,руслан+ковалев;flags=Foo,RTBF");
            UNIT_ASSERT_STRINGS_EQUAL(MergeRuOblivionParameters({
                    "names=маша+григорьева;flags=Foo", "names=варвара+лесных;flags=RTBF"}),
                    "names=варвара+лесных,маша+григорьева;flags=Foo");
            UNIT_ASSERT_STRINGS_EQUAL(MergeRuOblivionParameters({
                    "names=маша+григорьева", "names=варвара+лесных;flags=RTBF"}),
                    "names=варвара+лесных,маша+григорьева;");
        }

        void TestSimpleMatch() {
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "", "Боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=", "Боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=;flags=", "Боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "foobar", "Боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=михаил,миша,миха;flags=Foo,RTBF,Bar", "Боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=Михаил,Миша,Миха;flags=Foo,RTBF,Bar", "Михаил"), "михаил/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=михаил,миша,миха;flags=RTBF,Foo", "Михаил Миша"), "михаил/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=михаил,миша,миха;flags=Foo,RTBF", "Михаил Зина"), "михаил/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=михаил,миша,миха", "Зина Михаил"), "михаил/0");
        }

        void TestSubsetMatch() {
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "михаил миша"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "боря"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "Михаила Миши Ивановских"), "михаил+ивановских/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=миша+ивановских;names=михаил+ивановских,", "Ивановских Михаилу"), "михаил+ивановских/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "Михаилу Ивановских нездоровится"), "михаил+ивановских/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "нездоровится Михаилу Ивановских"), "михаил+ивановских/1");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "flags=RTBF;names=михаил+ивановских,миша+ивановских", "Мише нездоровится Ивановских"), "миша+ивановских/1");
        }

        void TestDoubleNameMatch() {
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=иван+иван-гецци", "Иван"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=иван+иван-гецци", "Иван Иван"), "");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=иван+иван-гецци", "Иван Гецци"), "иван+иван+гецци/0");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=иван+иван-гецци", "товарищ Иван Иван-Гецци, вам слово"), "иван+иван+гецци/0");
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=иван+иван-гецци", "я Иван-Гецци, зовут Иваном"), "иван+иван+гецци/0");
        }

        void TestFlection() {
            UNIT_ASSERT_STRINGS_EQUAL(DoFindRuOblivionMatch(
                    "names=варвара+лесных", "(Варвару & Полевых-Лесных) -( назвали лучшим Платоном ) << host:yandex.ru"), "варвара+лесных/0");
        }
    };
}

UNIT_TEST_SUITE_REGISTRATION(NTotalBan::TRuOblivionTest);
