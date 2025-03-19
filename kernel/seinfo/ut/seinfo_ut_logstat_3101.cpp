#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc73() {
    // LOGSTAT-3101
    {
        TInfo info(SE_YANDEX, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://yandex.ru/yandsearch?text=ford%20focus&clid=1955453&lr=213", info);
        UNIT_ASSERT_EQUAL(info.GetPageNum(), 0u);

        info.Name = SE_GOOGLE;
        KS_TEST_URL("http://www.google.ru/search?hl=ru&newwindow=1&q=ford%20focus&cad=h", info);

        info.Name = SE_YANDEX;
        info.SetPageNumRaw(3);
        KS_TEST_URL("http://yandex.ru/yandsearch?text=ford%20focus&clid=1955453&lr=213&p=3", info);
        info.SetPageNumRaw(2);
        info.SetPageSizeRaw(13);
        KS_TEST_URL("http://yandex.ru/yandsearch?text=ford%20focus&clid=1955453&lr=213&p=2&numdoc=13", info);
        info.SetPageNumRaw(TMaybe<ui32>());

        info.Name = SE_GOOGLE;
        info.Flags = ESearchFlags(SF_SEARCH);
        info.Query = "ку";

        info.SetPageStartRaw(20);
        info.SetPageSizeRaw(3);
        KS_TEST_URL("https://www.google.ru/search?ie=UTF-8&hl=ru&q=ку&start=20#hl=ru&newwindow=1&num=3&q=%D0%BA%D1%83", info);

        info.SetPageStartRaw(77);
        info.SetPageSizeRaw(20);
        KS_TEST_URL("https://www.google.ru/search?ie=UTF-8&hl=ru&q=ку&start=77#hl=ru&newwindow=1&num=20&q=%D0%BA%D1%83", info);
        KS_TEST_URL("https://www.google.ru/search?hl=ru&newwindow=1&q=ку&start=77&num=20&cad=h", info);

        info.SetPageStartRaw(40);
        info.SetPageSizeRaw(TMaybe<ui32>());
        KS_TEST_URL("https://www.google.ru/#newwindow=1&q=%D0%BA%D1%83&start=40", info);
        UNIT_ASSERT_EQUAL(info.GetPageNum(), 4u);
    }
}
