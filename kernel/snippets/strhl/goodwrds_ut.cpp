#include <library/cpp/testing/unittest/registar.h>
#include <kernel/lemmer/core/langcontext.h>
#include <kernel/qtree/richrequest/wordnode.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <library/cpp/charset/wide.h>
#include <util/stream/mem.h>
#include "goodwrds.h"
#include "zonedstring.h"
#include "forms.h"
#include "glue_common.h"

class TInlineHighlighterTest: public TTestBase
{
    UNIT_TEST_SUITE(TInlineHighlighterTest);
        UNIT_TEST(testLemmCount);
        UNIT_TEST(testPaintPassages);
        UNIT_TEST(testComment);
        UNIT_TEST(testMiscOps);
        UNIT_TEST(testJoker);
        UNIT_TEST(testTwoQueries);
        UNIT_TEST(testMultitokens);
        UNIT_TEST(testSmartQuotes);
        UNIT_TEST(testNot);
        UNIT_TEST(testFio);
        UNIT_TEST(testAbbrev);
        UNIT_TEST(testSuffix);
        UNIT_TEST(testDiacritic);
        UNIT_TEST(testZoned);
        UNIT_TEST(testExternalZone);
    UNIT_TEST_SUITE_END();
public:
    void testLemmCount();
    void testPaintPassages();
    void testComment();
    void testMiscOps();
    void testJoker();
    void testTwoQueries();
    void testMultitokens();
    void testSmartQuotes();
    void testNot();
    void testFio();
    void testAbbrev();
    void testSuffix();
    void testDiacritic();
    void testZoned();
    void testExternalZone();
};

void AddRequest(TInlineHighlighter& ih, const char* request, const THiliteMark* marks = nullptr, bool additional = false)
{
    try {
        TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
        TRichTreePtr tree = CreateRichTree(UTF8ToWide(request), opts);
        ih.AddRequest(*tree->Root.Get(), marks, additional);
    } catch (...) {
    }
}

void AddForma(TInlineHighlighter& ih, const char* forma, const THiliteMark* mark)
{
    Y_ASSERT(forma);
    TLanguageContext langcontext(LI_BASIC_LANGUAGES);
    TUtf16String s = UTF8ToWide(forma);
    TRichNodePtr node = CreateEmptyRichNode();
    node->WordInfo.Reset(TWordNode::CreateLemmerNode(s, TCharSpan(0, s.size()), fGeneral, langcontext).Release());
    ih.AddRequest(*node.Get(), mark, true);
}

void DoHilite(TInlineHighlighter& ih, TString& hl) {
    TUtf16String whl = UTF8ToWide(hl);
    ih.PaintPassages(whl);
    hl = WideToUTF8(whl);
}

void DoHilite(TInlineHighlighter& ih, TUtf16String& whl) {
    ih.PaintPassages(whl);
}

void DoPaint(TInlineHighlighter::TIPainterPtr p, TString& hl) {
    TUtf16String whl = UTF8ToWide(hl);
    p->AddJob(whl);
    p->Paint();
    hl = WideToUTF8(whl);
}

void TInlineHighlighterTest::testLemmCount()
{
    TInlineHighlighter ih;
    AddRequest(ih, "печем");

    TUtf16String wInput = u"на-\r\nпечью \n";
    TString input;
    WideToChar(wInput, input, CODES_YANDEX);

    size_t count = LemmCount(ih, input.c_str(), input.size());
    UNIT_ASSERT_VALUES_EQUAL(count, 0u);

    wInput = u"на\r\n-печью \n";
    WideToChar(wInput, input, CODES_YANDEX);
    count = LemmCount(ih, input.c_str(), input.size());
    UNIT_ASSERT_VALUES_EQUAL(count, 0u);

    wInput = u"каждый Печ в печи пироги печет";
    WideToChar(wInput, input, CODES_YANDEX);
    count = LemmCount(ih, input.c_str(), input.size());
    UNIT_ASSERT_VALUES_EQUAL(count, 3u);
}

void TInlineHighlighterTest::testMiscOps()
{
    TInlineHighlighter ih;
    AddRequest(ih, "first <- second");

    TString hl("first and second");
    DoHilite(ih, hl);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(UTF8ToWide(hl)), "\007[first\007] and \007[second\007]");

    TInlineHighlighter ih2;
    AddRequest(ih2, "first << second");

    TString hl2("first and second");
    DoHilite(ih2, hl2);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(UTF8ToWide(hl2)), "\007[first\007] and \007[second\007]");
}

void TInlineHighlighterTest::testJoker()
{
    TInlineHighlighter ih;
    THiliteMark m("<", ">");
    AddRequest(ih, "\"first * second\" and \"third * * sixth\" hmm \"first * * * first\" \"1 * 3 * * 6 7\"", &m);

    TString hl;
    hl = "first and second";
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "<first> <and> <second>");
    hl = "third first one sixth";
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "<third> <first> <one> <sixth>");
    hl = "zz third one third sixth one sixth qq and";
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "zz <third> <one> <third> <sixth> <one> <sixth> qq <and>");
    hl = "w first und second der first q second w w w w";
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "w <first> <und> <second> <der> <first> <q> <second> w w w w");
    hl = "0 1 2 3 4 5 6 7 8";
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "0 <1> <2> <3> <4> <5> <6> <7> 8");
}

static void AssertPaintPassagesInStroka(TInlineHighlighter& hl, const TString& s, const TString& expected)
{
    TUtf16String w = UTF8ToWide(s);
    DoHilite(hl, w);
    TString actual = WideToUTF8(w);
    UNIT_ASSERT_NO_DIFF(actual, expected);
}

void TInlineHighlighterTest::testPaintPassages()
{
    TInlineHighlighter ih;
    AddRequest(ih, "цветов");

    TUtf16String hl = u"каждый Цветок имеет свой цвет";
    DoHilite(ih, hl);
    UNIT_ASSERT_NO_DIFF(WideToUTF8(hl), "каждый \007[Цветок\007] имеет свой \007[цвет\007]");

    THiliteMark myMark("<b>", "</b>");
    AddForma(ih, "окон", &myMark);
    const TString hl2_orig = "на окне\n стоят \tцветы";
    TString hl2 = hl2_orig;
    AssertPaintPassagesInStroka(ih,
            hl2,
            "на <b>окне</b>\n стоят \t\007[цветы\007]");

    TUtf16String whl2 = UTF8ToWide(hl2_orig);
    DoHilite(ih, whl2);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(whl2), "на <b>окне</b>\n стоят \t\007[цветы\007]");

    TInlineHighlighter ih2;
    THiliteMark mark1("<font color=\"blue\">", "</font>");
    THiliteMark mark2("<font color=\"green\">", "</font>");
    THiliteMark mark3 ("<font color=\"red\">", "</font>");
    AddForma(ih2, "цвет", &mark1);
    AddForma(ih2, "москва", &mark2);
    AddForma(ih2, "тент", &mark2);
    AddForma(ih2, "фирма", &mark3);
    AddForma(ih2, "логистика", &mark2);
    AddForma(ih2, "кот", &mark3);
    AddForma(ih2, "крот", &mark3);
    AddForma(ih2, "пробел", &mark3);
    AddForma(ih2, "собака", &mark3);
    AddForma(ih2, "марка", &mark3);

    AssertPaintPassagesInStroka(ih2,
       "Международные перевозки, таможенный склад в Финляндии, логистика ",
       "Международные перевозки, таможенный склад в Финляндии, <font color=\"green\">логистика</font> ");

    AssertPaintPassagesInStroka(ih2,
       "логистика логистика",
       "<font color=\"green\">логистика</font> <font color=\"green\">логистика</font>");

    AssertPaintPassagesInStroka(ih2,
        "логистика логистика ",
        "<font color=\"green\">логистика</font> <font color=\"green\">логистика</font> ");

    TInlineHighlighter ih3;
    TString stopWordSrc = WideToChar(u"Version: 2\n[Russian]\nRIGHT: в\nLEFT: бы\nBOTH: и\nNONE: я", CODES_YANDEX);
    TWordFilter stopWords;
    TLanguageContext langContext;

    TMemoryInput stopWordInput(stopWordSrc);
    stopWords.InitStopWordsList(stopWordInput);
    langContext.SetStopWords(stopWords);

    TRichNodePtr tree = CreateRichNode(u"шушпанчики и упячки в сиропе я не понял бы вообще", TCreateTreeOptions(langContext));
    ih3.AddRequest(*tree.Get());

    AssertPaintPassagesInStroka(ih3,
        "шушпанчики и упячки, упячки и шушпанчики, шушпанчики и куры, но вряд ли куры и гуси",
        "\007[шушпанчики\007] \007[и\007] \007[упячки\007], \007[упячки\007] и \007[шушпанчики\007], \007[шушпанчики\007] \007[и\007] куры, но вряд ли куры и гуси");

    AssertPaintPassagesInStroka(ih3,
        "упячки в сиропе, сироп в упячках, упячки в полете, ягоды в сиропе, но едва ли куры в полете",
        "\007[упячки\007] \007[в\007] \007[сиропе\007], \007[сироп\007] в \007[упячках\007], \007[упячки\007] \007[в\007] полете, ягоды \007[в\007] \007[сиропе\007], но едва ли куры в полете");

    AssertPaintPassagesInStroka(ih3,
        "понял бы вообще, понял бы наконец, съел бы вообще, разгрыз бы вряд ли",
        "\007[понял\007] \007[бы\007] \007[вообще\007], \007[понял\007] \007[бы\007] наконец, съел \007[бы\007] \007[вообще\007], разгрыз бы вряд ли");

    AssertPaintPassagesInStroka(ih3,
        "в сиропе я не плавал, в итоге я понял, я съел бы и разгрыз бы я",
        "\007[в\007] \007[сиропе\007] \007[я\007] \007[не\007] плавал, в итоге \007[я\007] \007[понял\007], \007[я\007] съел бы и разгрыз бы \007[я\007]");

    // Tests for accents and soft hyphens
    AssertPaintPassagesInStroka(ih3,
        "ку­ры с гуся­ми, а также представи­тели отря­да ястреби­ных",
        "куры с гусями, а также представители отряда ястребиных");

    AssertPaintPassagesInStroka(ih3,
        "шушпа­нчики и упя­чки в сиро­пе я не по­нял бы вообще",
        "\007[шушпанчики\007] \007[и\007] \007[упячки\007] \007[в\007] \007[сиропе\007] \007[я\007] \007[не\007] \007[понял\007] \007[бы\007] \007[вообще\007]");

    TInlineHighlighter ih4;
    TRichNodePtr tree1 = CreateRichNode(u"a b c", TCreateTreeOptions(langContext));
    TString hohloma("hohloma1 hohloma2");
    tree1->AddMarkup(1, 1, new TSynonym(CreateRichNode(UTF8ToWide(hohloma), TCreateTreeOptions(langContext)), TE_TRANSLIT));
    ih4.AddRequest(*tree1.Get());

    TString s10("d a b c hohloma e");
    TString m10("d \007[a\007] \007[b\007] \007[c\007] hohloma e");
    DoHilite(ih4, s10);
    UNIT_ASSERT_VALUES_EQUAL(s10, m10);

    AssertPaintPassagesInStroka(ih4,
        "d a hohloma1 b e",
        "d \007[a\007] \007[hohloma\007]\007[1\007] \007[b\007] e");

    AssertPaintPassagesInStroka(ih4,
        "d hohloma1 hohloma2 e",
        "d \007[hohloma\007]\007[1\007] \007[hohloma\007]\007[2\007] e");

    AssertPaintPassagesInStroka(ih4,
        "d hohloma2 a hohloma1 c",
        "d \007[hohloma\007]\007[2\007] \007[a\007] \007[hohloma\007]\007[1\007] \007[c\007]");
}

void TInlineHighlighterTest::testComment()
{
    TInlineHighlighter ih;
    AddRequest(ih, "nokia");

    AssertPaintPassagesInStroka(ih,
        "<!-- direct-banner -->Nokia<!-- /direct-banner -->",
        "<!-- direct-banner -->\007[Nokia\007]<!-- /direct-banner -->");
}

void TInlineHighlighterTest::testTwoQueries()
{
    TInlineHighlighter ih;
    AddRequest(ih, "first");
    AddRequest(ih, "second", nullptr, true);
    AssertPaintPassagesInStroka(ih,
        "first and second",
        "\007[first\007] and \007[second\007]");
}

void TInlineHighlighterTest::testMultitokens() {
    TInlineHighlighter ih;
    AddRequest(ih, "(одноклассники::42470. ^ odnoklassniki::845038 ^ одноклассница::800531 ^ однокласник::1132674) &/(1 1) ru::432 softness:6");
    AssertPaintPassagesInStroka(ih,
        "leonidas@kp.md. Фото с сайта Odnoklassniki.ru. Оцени материал: 1.",
        "leonidas@kp.md. Фото с сайта \007[Odnoklassniki\007].\007[ru\007]. Оцени материал: 1.");
}

void TInlineHighlighterTest::testSmartQuotes() {
    {
        TInlineHighlighter ih;
        THiliteMark m("<", ">");
        AddRequest(ih, "\"first * second\"", &m);
        TString hl = "first and second";
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintClosePositionsSmart = true;
        DoPaint(p, hl);
        UNIT_ASSERT_VALUES_EQUAL(hl, "<first> <and> <second>");
    }
    TInlineHighlighter ih;
    AddRequest(ih, "\"test case\"");
    TString hl("test case in your case");
    TString hl2 = hl;
    TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
    p->Options.PaintClosePositionsSmart = true;
    DoPaint(p, hl);
    TInlineHighlighter::TIPainterPtr p2 = ih.GetPainter();
    p2->Options.PaintClosePositionsSmart = false;
    DoPaint(p2, hl2);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[test\007] \007[case\007] in your case");
    UNIT_ASSERT_STRINGS_EQUAL(hl2.data(), "\007[test\007] \007[case\007] in your \007[case\007]");
}

void TInlineHighlighterTest::testNot() {
    TInlineHighlighter ih;
    AddRequest(ih, "test -case");
    TString hl("test case in your case");
    DoHilite(ih, hl);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[test\007] case in your case");
}

void TInlineHighlighterTest::testFio() {
    TInlineHighlighter ih;
    AddRequest(ih, "john smith <- (fioname:((\"johnfi\" smith)))");
    TString hl("really john smith is not a smith, he is just john");
    TString hl1("john really smith is not a smith, he is just john");
    TString hl2 = hl;
    TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
    p->Options.PaintFios = true;
    p->Options.SmartUnpaintFios = true;
    DoPaint(p, hl);
    DoPaint(p, hl1);
    TInlineHighlighter::TIPainterPtr p2 = ih.GetPainter();
    p2->Options.PaintFios = true;
    p2->Options.SmartUnpaintFios = false;
    DoPaint(p2, hl2);
    UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "really \007[john\007] \007[smith\007] is not a smith, he is just john");
    UNIT_ASSERT_STRINGS_EQUAL(hl1.data(), "\007[john\007] really \007[smith\007] is not a smith, he is just john");
    UNIT_ASSERT_STRINGS_EQUAL(hl2.data(), "really \007[john\007] \007[smith\007] is not a \007[smith\007], he is just \007[john\007]");
    {
        TInlineHighlighter ih;
        AddRequest(ih, "john really smith <- (fioname:((\"johnfi\" \"reallyfo\" smith)))");
        TString hl("john really smith is not a john really blacksmith");
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintFios = true;
        p->Options.SmartUnpaintFios = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[john\007] \007[really\007] \007[smith\007] is not a john really blacksmith");
    }
    {
        TInlineHighlighter ih;
        AddRequest(ih, "alex smith <- (fioname:((\"afi\" smith)))");
        TString hl("a smith whose name is A Smith");
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintFios = true;
        p->Options.SmartUnpaintFios = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "a smith whose name is \007[A\007] \007[Smith\007]");
    }
    {
        TInlineHighlighter ih;
        AddRequest(ih, "alex smith <- ((fioname:((\"afi\" smith))) | (fioname:((\"alexfi\" smith))))");
        TString hl("Alex knows a smith whose name is A. Smith");
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintFios = true;
        p->Options.SmartUnpaintFios = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "Alex knows a smith whose name is \007[A\007]. \007[Smith\007]");
    }
    {
        TInlineHighlighter ih;
        AddRequest(ih, "alex smith <- ((fioname:((\"afi\" smith))) | (fioname:((\"alexfi\" smith))))");
        TString hl("a smith");
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintFios = true;
        p->Options.SmartUnpaintFios = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "a \007[smith\007]");
    }
    {
        TInlineHighlighter ih;
        TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
        TRichTreePtr tree = CreateRichTree(u"Pyotr Petrov <- (fioname:((\"Pyotrfi\" Petrov)))", opts);
        TRichTreePtr tree2 = CreateRichTree(u"Petya", opts);
        tree->Root->AddMarkup(0, 0, new TSynonym(tree2->Root, TE_NONE));
        ih.AddRequest(*tree->Root.Get());
        TString hl = "Petya Petrov is not Pyotr Ivanov, not Petya Ivanov, not Ivan Petrov";
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintFios = true;
        p->Options.SmartUnpaintFios = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[Petya\007] \007[Petrov\007] is not Pyotr Ivanov, not Petya Ivanov, not Ivan Petrov");
    }
}

void TInlineHighlighterTest::testAbbrev() {
    TCreateTreeOptions opts(LI_DEFAULT_REQUEST_LANGUAGES);
    TRichTreePtr tree = CreateRichTree(u"gcc", opts);
    TRichTreePtr tree2 = CreateRichTree(u"gnu compiler collection", opts);
    tree->Root->AddMarkup(0, 0, new TSynonym(tree2->Root, TE_ABBREV));
    TInlineHighlighter ih;
    ih.AddRequest(*tree->Root.Get());
    {
        TString hl = "gnu compiler collection is a gnu tool";
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintAbbrevSmart = true;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[gnu\007] \007[compiler\007] \007[collection\007] is a gnu tool");
    }
    {
        TString hl = "gnu compiler collection is a gnu tool";
        TInlineHighlighter::TIPainterPtr p = ih.GetPainter();
        p->Options.PaintAbbrevSmart = false;
        DoPaint(p, hl);
        UNIT_ASSERT_STRINGS_EQUAL(hl.data(), "\007[gnu\007] \007[compiler\007] \007[collection\007] is a \007[gnu\007] tool");
    }
}

void TInlineHighlighterTest::testSuffix()
{
    TInlineHighlighter ih;
    AddRequest(ih, "first second++ flylinkdc");

    TString hl("first++ first second++ third FlylinkDC++");
    DoHilite(ih, hl);
    UNIT_ASSERT_VALUES_EQUAL(hl, "\007[first\007]++ \007[first\007] \007[second++\007] third \007[FlylinkDC\007]++");
}

void TInlineHighlighterTest::testDiacritic()
{
    TInlineHighlighter ih;
    ih.AddRequest(*CreateRichTree(u"K\u00C4RCHER", TCreateTreeOptions(LI_DEFAULT_REQUEST_LANGUAGES))->Root.Get(), nullptr, false);


    TUtf16String hl(u"K\u00C4RCHER");
    DoHilite(ih, hl);
    TString s = WideToUTF8(hl);
    UNIT_ASSERT_VALUES_EQUAL(s, "\007[K\xc3\x84RCHER\007]");
}

void TInlineHighlighterTest::testZoned() {
    TInlineHighlighter ih5;
    AddRequest(ih5, "test test++");
    TUtf16String hl = u"#Test $test 'te\u00ADst' #tset test@ test++ testt tests.";
    TZonedString zoned = hl;
    TInlineHighlighter::TIPainterPtr p = ih5.GetPainter();

    p->Options.SrcOutput = false;
    p->AddJob(zoned);
    p->Paint();
    UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(zoned.String),
        "#Test $test 'test' #tset test@ test++ testt tests.");
    UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(NSnippets::MergedGlue(zoned)),
        "#\007[Test\007] $\007[test\007] '\007[test\007]' #tset \007[test\007]@ \007[test++\007] testt \007[tests\007].");
    const auto& spans = zoned.Zones[+TZonedString::ZONE_UNKNOWN].Spans;
    UNIT_ASSERT_VALUES_EQUAL(spans.size(), 6);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(spans[0].Span), "Test");
    UNIT_ASSERT_VALUES_EQUAL(~spans[0], zoned.String.data() + 1);
    UNIT_ASSERT_VALUES_EQUAL(+spans[0], 4);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(spans[2].Span), "test");
    UNIT_ASSERT_VALUES_EQUAL(~spans[2], zoned.String.data() + 13);
    UNIT_ASSERT_VALUES_EQUAL(+spans[2], 4);
    UNIT_ASSERT_VALUES_EQUAL(~spans[3], zoned.String.data() + 25);
    UNIT_ASSERT_VALUES_EQUAL(+spans[3], 4);
    UNIT_ASSERT_VALUES_EQUAL(~spans[4], zoned.String.data() + 31);
    UNIT_ASSERT_VALUES_EQUAL(+spans[4], 6);

    zoned = hl;
    p->Options.SrcOutput = true;
    p->AddJob(zoned);
    p->Paint();
    UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(zoned.String),
        "#Test $test 'te\xC2\xADst' #tset test@ test++ testt tests.");
    UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(NSnippets::MergedGlue(zoned)),
        "#\007[Test\007] $\007[test\007] '\007[te\xC2\xADst\007]' #tset \007[test\007]@ \007[test++\007] testt \007[tests\007].");
    const auto& spans2 = zoned.Zones[+TZonedString::ZONE_UNKNOWN].Spans;
    UNIT_ASSERT_VALUES_EQUAL(spans2.size(), 6);
    UNIT_ASSERT_VALUES_EQUAL(WideToUTF8(spans2[2].Span), "te\xC2\xADst");
    UNIT_ASSERT_VALUES_EQUAL(~spans2[2], zoned.String.data() + 13);
    UNIT_ASSERT_VALUES_EQUAL(+spans2[2], 5);
}

void TInlineHighlighterTest::testExternalZone() {
    TInlineHighlighter ih;
    AddRequest(ih, "(ооо::3930 ^ ooo::156076) &&/(-3 5) промэкспресс::840469560 &/(-3 3) (45::840469560- &/(1 1) 83::840469560- &/(1 1) 83::840469560) ^ tel_local:\"458383\"::840469560");
    TZonedString zoned = TUtf16String(u"(8422) 45-83-83. Компания \"ООО \"Промэкспресс\"\", выставочная продукция, виды услуг.");
    auto& phones = zoned.Zones[+TZonedString::ZONE_MATCHED_PHONE];
    phones.Mark = &DEFAULT_MARKS;
    phones.Spans.push_back(TZonedString::TSpan(TWtringBuf(zoned.String).Skip(1).Head(4)));
    phones.Spans.push_back(TZonedString::TSpan(TWtringBuf(zoned.String).Skip(7).Head(2)));
    phones.Spans.push_back(TZonedString::TSpan(TWtringBuf(zoned.String).Skip(10).Head(2)));
    phones.Spans.push_back(TZonedString::TSpan(TWtringBuf(zoned.String).Skip(13).Head(2)));
    auto p = ih.GetPainter();
    p->Options = TPaintingOptions::DefaultSnippetOptions();
    p->AddJob(zoned);
    p->Paint();
    UNIT_ASSERT_STRINGS_EQUAL(WideToUTF8(NSnippets::MergedGlue(zoned)),
        "(\007[8422\007]) \007[45\007]-\007[83\007]-\007[83\007]. Компания \"\007[ООО\007] \"\007[Промэкспресс\007]\"\", выставочная продукция, виды услуг.");
}

UNIT_TEST_SUITE_REGISTRATION(TInlineHighlighterTest);
