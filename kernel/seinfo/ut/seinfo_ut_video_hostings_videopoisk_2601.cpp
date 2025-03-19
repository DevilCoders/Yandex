#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc62() {
    // Video hostings. VIDEOPOISK-2601.
    {
        TInfo info(SE_UNKNOWN, ST_VIDEO, "ford focus");
        info.Flags = ESearchFlags(SF_SEARCH | SF_LOCAL);

        info.Name = SE_MYVI_RU;
        KS_TEST_URL("http://www.myvi.ru/search?on_header=true&text=ford%20focus&category=-1", info);

        info.Name = SE_RUTUBE;
        KS_TEST_URL("http://rutube.ru/search/?query=ford+focus", info);

        info.Name = SE_KINOSTOK_TV;
        KS_TEST_URL("http://kinostok.tv/search/basic/ford%20focus/?", info);

        info.Name = SE_NAMBA_NET;
        KS_TEST_URL("http://namba.net/#!/search/video/ford focus", info);

        info.Name = SE_BIGCINEMA_TV;
        KS_TEST_URL("http://bigcinema.tv/search/topics?q=ford+focus", info);

        info.Name = SE_UKRHOME_NET;
        KS_TEST_URL("http://video.ukrhome.net/search/?q=ford+focus", info);

        info.Name = SE_KAZTUBE_KZ;
        KS_TEST_URL("kaztube.kz/ru/video?s=ford+focus&order_direct=&order_day=&order=", info);

        info.Name = SE_KIWI_KZ;
        KS_TEST_URL("http://test.kiwi.kz/search/?keyword=ford+focus&search-submit=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8", info);
        KS_TEST_URL("http://kiwi.kz/search/?keyword=ford+focus&search-submit=%D0%9D%D0%B0%D0%B9%D1%82%D0%B8", info);

        info.Name = SE_NOW_RU;
        KS_TEST_URL("http://www.now.ru/search?q=ford%20focus", info);

        info.Name = SE_VIMEO;
        KS_TEST_URL("https://vimeo.com/search?q=ford+focus", info);
        KS_TEST_URL("http://vimeo.com/search?q=ford+focus", info);

        info.Name = SE_AKILLI_TV;
        KS_TEST_URL("http://www.akilli.tv/search.aspx?q=ford+focus", info);
        // TODO: think here
        // KS_TEST_URL("http://www.akilli.tv/search.aspx?q=ford+focus", info);

        info.Name = SE_DAILYMOTION_COM;
        KS_TEST_URL("http://www.dailymotion.com/ru/relevance/search/ford+focus/1#", info);

        info.Name = SE_IZLEMEX_ORG;
        KS_TEST_URL("http://src.izlemex.org/?q=ford+focus", info);
        KS_TEST_URL("http://www.izlemex.org/?q=ford+focus", info);
        // TODO: think here
        // KS_TEST_URL("http://src.izlemex.org/?q=ford-focus", info);
        // KS_TEST_URL("http://www.izlemex.org/?q=ford-focus", info);

        info.Name = SE_KUZU_TV;
        KS_TEST_URL("http://www.kuzu.tv/arama?q=ford+focus", info);

        info.Name = SE_TEKNIKTV_COM;
        KS_TEST_URL("http://www.tekniktv.com/ara.php?q=ford+focus", info);

        info.Name = SE_IZLESENE_COM;
        KS_TEST_URL("http://search.izlesene.com/?kelime=ford+focus", info);
    }
}
