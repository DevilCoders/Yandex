#include "dehtml.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/memory/tempbuf.h>
#include <util/charset/wide.h>
#include <util/string/strip.h>

#include <array>

namespace {
    void CheckEntityStripper(const TString& from, const TString& etalon, const TString& message = {}) {
        THtmlStripper stripper(HSM_ENTITY, CODES_UTF8);
        const TString res = stripper(from);
        UNIT_ASSERT_VALUES_EQUAL_C(res, etalon, message.data());
    }
}

class TTestDehtml: public TTestBase {
    UNIT_TEST_SUITE(TTestDehtml);
    UNIT_TEST(CommonTest);
    UNIT_TEST(CommonTest1);
    UNIT_TEST(EntityTest);
    UNIT_TEST(TrashTest);
    UNIT_TEST(SemicolonTest);
    UNIT_TEST(SpaceTest);
    UNIT_TEST(NoCopyTest);
    UNIT_TEST(NoSpaceInsertTest);
    UNIT_TEST_SUITE_END();

public:
    void CommonTest() {
        TString dirty("Интернет-магазин; < a href=\"https://www.someurl.ru\" >Детские коляски</a>, Коляски-трансформеры, Otutto 3 (PRAM), Inglesina. <P>Система <B>OTUTTO</B> - все, что вам нужно!</P><P><STRONG>В комплекте: </STRONG>Люлька, прогулочная коляска, автокресло с базой, накидка на ножки, капюшон, сумка, корзина для покупок</P><P><B>Люлька:</B></P><LI><UL type=circle><LI>Внутренняя обивка выполнена из хлопка. <LI>Размеры спального места: длина см х ширина см х глубина см. <LI>Система EASY CLIP. </LI></UL><BR><B>Прогулочное сидение:</B> <LI><UL type=circle><LI>5 точечные ремни безопасности. <LI>Обшивка коляски легко снимается. Стирается при температуре 30 градусов. <LI>Сетка для продуктов. <LI>Утепленный мешок для ножек. <LI>Регулируемая спинка и подножка. </LI></UL><BR><B>Inglesina Haggy + база Автокресло-переноска от 0 до 13 кг</B><BR><UL type=circle><LI>Автомобильное кресло относится к группе 0-0+, подходит детям от 0 до 1 года или до 13 кг. <LI>Соответствует европейскому стандарту безопасности ECE R44/03 (0+/1 Группе). <LI>Может быть установлено на шасси Inglesina. <LI>Имеет защитный капюшон от солнца, вставку для новорожденного ребенка для поддержки головы. <LI>Сидение необыкновенно комфортное благодаря мягкой набивке и использования специальных хлопчатобумажных материалов. <LI>Съемное покрытие можно стирать при температуре не выше 30 градусов. </LI></UL><LI><B>Ручка:</B> <UL type=circle><LI>Удобная эргономическая ручка. </LI></UL><LI><B>Колеса:</B> <UL type=circle><LI>4 пары сдвоенных колес, плавающие с фиксацией. <LI>колеса с тормозом. <LI>Размер: 18,5 см </LI></UL><LI><B>Шасси:</B> <UL type=circle><LI>Алюминиевое. <LI>Размер коляски в сложенном виде: ширина 39 см длина 36 х высота 100 см. <LI>Размер коляски в разложенном виде: ширина 59.9 см х длина 92 см х высота 110 см. <LI>Вес: 7,4 кг </LI></UL></LI><!--comment-->< !-- comment -- >");
        TString clear("Интернет-магазин;  Детские коляски , Коляски-трансформеры, Otutto 3 (PRAM), Inglesina.  Система  OTUTTO  - все, что вам нужно!   В комплекте:  Люлька, прогулочная коляска, автокресло с базой, накидка на ножки, капюшон, сумка, корзина для покупок   Люлька:     Внутренняя обивка выполнена из хлопка.  Размеры спального места: длина см х ширина см х глубина см.  Система EASY CLIP.     Прогулочное сидение:     5 точечные ремни безопасности.  Обшивка коляски легко снимается. Стирается при температуре 30 градусов.  Сетка для продуктов.  Утепленный мешок для ножек.  Регулируемая спинка и подножка.     Inglesina Haggy + база Автокресло-переноска от 0 до 13 кг    Автомобильное кресло относится к группе 0-0+, подходит детям от 0 до 1 года или до 13 кг.  Соответствует европейскому стандарту безопасности ECE R44/03 (0+/1 Группе).  Может быть установлено на шасси Inglesina.  Имеет защитный капюшон от солнца, вставку для новорожденного ребенка для поддержки головы.  Сидение необыкновенно комфортное благодаря мягкой набивке и использования специальных хлопчатобумажных материалов.  Съемное покрытие можно стирать при температуре не выше 30 градусов.     Ручка:    Удобная эргономическая ручка.     Колеса:    4 пары сдвоенных колес, плавающие с фиксацией.  колеса с тормозом.  Размер: 18,5 см     Шасси:    Алюминиевое.  Размер коляски в сложенном виде: ширина 39 см длина 36 х высота 100 см.  Размер коляски в разложенном виде: ширина 59.9 см х длина 92 см х высота 110 см.  Вес: 7,4 кг      ");

        const size_t dirtyTextLen = dirty.size();
        size_t toLen = dirtyTextLen * 5;
        TTempBuf to(toLen);

        THtmlStripper::Strip(HSM_ENTITY, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(clear, TString(to.Data(), toLen));
    }

    void CommonTest1() {
        TString dirty("<html><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\
<center><table border=\"0\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">\
<tr><td><table border=\"0\" width=\"100%\" cellspacing=\"0\" cellpadding=\"0\">\
<tr height=\"16\"><td><p></td></tr>\
<tr><td><em><strong>svn+ssh://svn.yandex.ru/market/indexer/feedparser/trunk/debian/changelog</strong></em></td></tr>\
<tr><td bgcolor=\"#eeeeee\"><pre><tag3>");

        TString clear("svn+ssh://svn.yandex.ru/market/indexer/feedparser/trunk/debian/changelog");

        const size_t dirtyTextLen = dirty.size();
        size_t toLen = dirtyTextLen * 5;
        TTempBuf to(toLen);

        THtmlStripper::Strip(HSM_ENTITY | HSM_TRASH, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(clear, StripString(TString(to.Data(), toLen)));
    }

    void EntityTest() {
        const TString lt("&lt;&#60;&#x3C;&#x3c;&#X3C;");
        const TString ltEtalon("\x3C\x3C\x3C\x3C\x3C");
        CheckEntityStripper(lt, ltEtalon);

        const TString nbsp("&nbsp;&#160;&#xA0;");
        const TString nbspEtalon("\xC2\xA0\xC2\xA0\xC2\xA0");
        CheckEntityStripper(nbsp, nbspEtalon);

        const TString misc("&#1; &#10; &#100; &#1000; &#10000; &#100000; &#1000000;"
                           " &#x1; &#x10; &#x100; &#x1000; &#x10000; &#x100000;");
        const TString miscEtalon("\x01 \x0A \x64 \xCF\xA8 \xE2\x9C\x90 \xF0\x98\x9A\xA0 &#1000000;"
                                 " \x01 \x10 \xC4\x80 \xE1\x80\x80 \xF0\x90\x80\x80 &#x100000;");
        CheckEntityStripper(misc, miscEtalon);

        for (const auto& emoji : TVector<std::array<TString, 4>>{
                {"&#128512;", "&#x1F600;", "\xF0\x9F\x98\x80", "Grinning smiley"},
                {"&#127817;", "&#x1F349;", "\xF0\x9F\x8D\x89", "Watermelon"},
                {"&#127799;", "&#x1F337;", "\xF0\x9F\x8C\xB7", "Tulip"}})
        {
            CheckEntityStripper(emoji[0], emoji[2], emoji[3]);
            CheckEntityStripper(emoji[1], emoji[2], emoji[3]);
        }
    }

    void TrashTest() {
        const TString dirty("&ltимператор <incorrect tag> &lt;Всероссийский&gt; с <abbr title=\"по...");
        const TString trashRemovedResult("император  <Всероссийский> с ");
        const TString trashRemovedWithoutEntitiesResult("&ltимператор  &lt;Всероссийский&gt; с ");
        const TString trashNotRemovedResult("&ltимператор <incorrect tag> <Всероссийский> с <abbr title=\"по...");

        size_t toLen = dirty.size() * 5;
        TTempBuf to(toLen);

        THtmlStripper::Strip(HSM_ENTITY | HSM_TRASH, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        const TString trashRemovedStripped(to.Data(), toLen);

        toLen = dirty.size() * 5;
        THtmlStripper::Strip(HSM_TRASH, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        const TString trashRemovedWithoutEntitiesStripped(to.Data(), toLen);

        toLen = dirty.size() * 5;
        THtmlStripper::Strip(HSM_ENTITY, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        const TString trashNotRemovedStripped(to.Data(), toLen);

        UNIT_ASSERT_STRINGS_EQUAL(trashRemovedResult, trashRemovedStripped);
        UNIT_ASSERT_STRINGS_EQUAL(trashRemovedWithoutEntitiesResult, trashRemovedWithoutEntitiesStripped);
        UNIT_ASSERT_STRINGS_EQUAL(trashNotRemovedResult, trashNotRemovedStripped);
    }

    void SemicolonTest() {
        TString dirty("&lt;1;2,3&gt;"); // test whether stripper doesn't remove ';' from "&lt;"
        TString clear("<1,2,3>");

        const size_t dirtyTextLen = dirty.size();
        size_t toLen = dirtyTextLen * 5;
        TTempBuf to(toLen);

        THtmlStripper::Strip(HSM_ENTITY | HSM_SEMICOLON, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(clear, TString(to.Data(), toLen));
    }

    void SpaceTest() {
        const TString dirty(" <tag> text, text; text ; text , text\n </tag>\t");
        const TString spaceOnly("text, text; text ; text , text");
        const TString spaceDelim("text,text; text ; text text");
        const TString spaceDelimSemicolon("text,text,text text text");

        const size_t dirtyTextLen = dirty.size();
        size_t toLen = dirtyTextLen * 5;
        TTempBuf to(toLen);

        THtmlStripper::Strip(HSM_SPACE, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(spaceOnly, TString(to.Data(), toLen));

        toLen = dirtyTextLen * 5;
        THtmlStripper::Strip(HSM_SPACE | HSM_DELIMSPACE, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(spaceDelim, TString(to.Data(), toLen));

        toLen = dirtyTextLen * 5;
        THtmlStripper::Strip(HSM_SPACE | HSM_DELIMSPACE | HSM_SEMICOLON, to.Data(), toLen, dirty.data(), dirty.size(), CODES_UTF8);
        UNIT_ASSERT_STRINGS_EQUAL(spaceDelimSemicolon, TString(to.Data(), toLen));
    }

    void NoCopyTest() {
        TString dirty("Simple string without html formatting,semicolons and too many spaces.");

        // TString functions
        {
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_ENTITY, CODES_UTF8)(dirty).data());
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_ENTITY | HSM_SPACE, CODES_UTF8)(dirty).data());
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_ENTITY | HSM_SPACE | HSM_DELIMSPACE, CODES_UTF8)(dirty).data());
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_ENTITY | HSM_SPACE | HSM_SEMICOLON | HSM_DELIMSPACE | HSM_TRASH, CODES_UTF8)(dirty).data());
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_TRASH, CODES_UTF8)(dirty).data());
            UNIT_ASSERT_EQUAL(dirty.data(), THtmlStripper(HSM_ENTITY | HSM_TRASH, CODES_UTF8)(dirty).data());
        }

        // TStringBuf functions
        {
            TString out;
            UNIT_ASSERT(!THtmlStripper(HSM_ENTITY, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(!THtmlStripper(HSM_ENTITY | HSM_SPACE, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(!THtmlStripper(HSM_ENTITY | HSM_SPACE | HSM_DELIMSPACE, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(!THtmlStripper(HSM_ENTITY | HSM_SPACE | HSM_SEMICOLON | HSM_DELIMSPACE | HSM_TRASH, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(!THtmlStripper(HSM_TRASH, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(!THtmlStripper(HSM_ENTITY | HSM_TRASH, CODES_UTF8)(TStringBuf(dirty), out));
            UNIT_ASSERT(out.empty());
        }

        // Additional test for correct space removing
        const TString spaces("spaces   ");
        const TString spacesTrimmed = THtmlStripper(HSM_ENTITY | HSM_SPACE, CODES_UTF8)(spaces);
        UNIT_ASSERT_STRINGS_EQUAL(spacesTrimmed, "spaces");
        TString out;
        UNIT_ASSERT(THtmlStripper(HSM_ENTITY | HSM_SPACE, CODES_UTF8)(TStringBuf(spaces), out));
        UNIT_ASSERT_STRINGS_EQUAL(out, "spaces");
    }

    void NoSpaceInsertTest() {
        TString dirty("<b>html</b> string");
        UNIT_ASSERT_STRINGS_EQUAL("html string", THtmlStripper(HSM_NO_SPACE_INSERT, CODES_UTF8)(dirty));
    }
};

UNIT_TEST_SUITE_REGISTRATION(TTestDehtml);
