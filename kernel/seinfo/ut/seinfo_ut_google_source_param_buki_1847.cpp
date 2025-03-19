#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc60() {
    // Google 'source' param. BUKI-1847
    {
        TInfo info(SE_GOOGLE);

        info.Type = ST_BLOGS;
        info.Flags = ESearchFlags(SF_SEARCH | SF_REDIRECT);
        info.Query = "หีเด็ก4ขวบเนียนๆ";
        KS_TEST_URL("http://www.google.co.th/url?sa=t&rct=j&q=%E0%B8%AB%E0%B8%B5%E0%B9%80%E0%B8%94%E0%B9%87%E0%B8%814%E0%B8%82%E0%B8%A7%E0%B8%9A%E0%B9%80%E0%B8%99%E0%B8%B5%E0%B8%A2%E0%B8%99%E0%B9%86&source=blogsearch", info);

        info.Type = ST_NEWS;
        info.Query = "ютуб";
        KS_TEST_URL("http://www.google.com.ua/url?sa=t&rct=j&q=%D1%8E%D1%82%D1%83%D0%B1&source=newssearch", info);

        info.Type = ST_COM;
        info.Query = "obd2 bluetooth adapter";
        KS_TEST_URL("http://www.google.de/url?sa=t&rct=j&q=obd2%20bluetooth%20adapter&source=productsearch", info);

        info.Type = ST_IMAGES;
        info.Query = "";
        KS_TEST_URL("http://www.google.ru/url?sa=i&rct=j&q=&esrc=s&source=images&cd=&docid=Tywok1AH_NvKYM&tbnid=9xrr8Zb-_6h0vM:&ved=0CAQQjB0&url=http%3A%2F%2Flifeglobe.net%2F%3Ft%3DCINEMA&ei=_RsJUczkIrH24QSZ14DwDg&bvm=bv.41642243,d.bGE&psig=AFQjCNGz9uA8l3vXGKMpYkL3_Bdb8cdw_g&ust=1359", info);

        info.Type = ST_WEB; // ?
        info.Query = "youtube";
        KS_TEST_URL("http://www.google.com/url?url=http://www.youtube.com/&q=youtube&rct=j&sa=X&source=suggest", info);

        info.Type = ST_VIDEO;
        info.Query = "dersim soykırımı tanıkları";
        KS_TEST_URL("http://www.google.com.tr/url?sa=t&rct=j&q=dersim+soyk%C4%B1r%C4%B1m%C4%B1+tan%C4%B1klar%C4%B1&source=video", info);

        info.Type = ST_WEB;
        info.Query = "ra2 скачать торрент";
        KS_TEST_URL("http://www.google.ru/url?sa=t&rct=j&q=ra2%20%D1%81%D0%BA%D0%B0%D1%87%D0%B0%D1%82%D1%8C%20%D1%82%D0%BE%D1%80%D1%80%D0%B5%D0%BD%D1%82&source=w", info);

        info.Type = ST_BOOKS;
        info.Query = "пятикнижие моисея";
        KS_TEST_URL("http://www.google.com/url?sa=t&rct=j&q=%D0%BF%D1%8F%D1%82%D0%B8%D0%BA%D0%BD%D0%B8%D0%B6%D0%B8%D0%B5%20%D0%BC%D0%BE%D0%B8%D1%81%D0%B5%D1%8F&source=books", info);
        info.Query = "william somerset maugham";
        KS_TEST_URL("http://www.google.am/url?sa=t&rct=j&q=william%20somerset%20maugham&source=books", info);
    }
}
