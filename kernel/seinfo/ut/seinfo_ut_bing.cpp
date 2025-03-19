#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc12() {
    // bing
    {
        TInfo info(SE_BING, ST_WEB, "ку", SF_SEARCH);

        KS_TEST_URL("www.bing.com/search?q=%D0%BA%D1%83", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.bing.com/images/search?q=%D0%BA%D1%83&FORM=BIFD", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("bing.com/videos/search?q=%D0%BA%D1%83&FORM=BVFD", info);

        info.Query = "12 34";
        info.Type = ST_WEB;
        KS_TEST_URL("http://www.bing.com/search?q=12+34&x=91&y=17&form=MSNH81&mkt=ru-ru", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.bing.com/images/search?q=12+34&FORM=BIFD", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.bing.com/videos/search?q=12+34&FORM=BVFD", info);

        info.Query = "123";

        info.Type = ST_WEB;
        KS_TEST_URL("http://www.bing.com/search?q=123&x=167&y=30&form=MSNH14&qs=n&sk=&qs=n&sk=", info);
        info.Type = ST_COM;
        KS_TEST_URL("http://www.bing.com/shopping/search?q=123&mkt=en-US&FORM=BPFD", info);
        info.Type = ST_MAPS;
        KS_TEST_URL("http://www.bing.com/maps/default.aspx?q=123&mkt=en-US&FORM=BYFD", info);

        info.Query = "define innovation";
        info.Type = ST_ENCYC;
        KS_TEST_URL("http://www.bing.com/Dictionary/search?q=define+innovation&go=&form=QB", info);

        info.Query = "Seattle";
        info.Type = ST_EVENTS;
        KS_TEST_URL("http://www.bing.com/events/search?q=Seattle&FORM=L8SP31", info);

        info.Query = "Cheesecake recipe";
        info.Type = ST_RECIPES;
        KS_TEST_URL("http://www.bing.com/recipe/search?q=Cheesecake+recipe&go=&form=QB", info);

        info.Query = "123";
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.bing.com/movies/search?q=123&go=&qs=n&sk=&sc=8-3&form=VBREQY", info);
        info.Type = ST_MUSIC;
        KS_TEST_URL("http://www.bing.com/search?nrv=music&q=123&form=NRMURQ", info);
        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://www.bing.com/social/search?q=123&go=&form=DTPSOH", info);

        info.Query = "Research new cars";
        info.Type = ST_CARS;
        KS_TEST_URL("http://www.bing.com/autos?q=Research+new+cars&go=&form=DTPAUT", info);

        info.Query = "123";
        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("http://m.bing.com/search/search.aspx?Q=123&d=&dl=&pq=&a=results&MID=1", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://m.bing.com/search/search.aspx?A=imageresults&Q=123&D=Image&SCO=0", info);
    }
}
