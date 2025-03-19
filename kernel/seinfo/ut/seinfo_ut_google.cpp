#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc9() {
    // google
    {
        TInfo info(SE_GOOGLE, ST_WEB, "ку", SF_SEARCH);

        KS_TEST_URL("http://www.google.ru/search?ie=UTF-8&hl=ru&q=ку", info);

        info.Query = "ku";
        KS_TEST_URL("http://www.google.com/#hl=en&source=hp&q=ku&aq=f&aqi=g10&aql=&oq=&gs_rfai=", info);
        const TInfo info2(SE_GOOGLE, ST_WEB, "ku", ESearchFlags(SF_SEARCH));
        KS_TEST_URL("https://www.google.com/#hl=en&source=hp&q=ku&aq=f&aqi=g10&aql=&oq=&gs_rfai=", info2);

        info.Query = "контакт";
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("https://www.google.ru/search?q=%D0%BA%D0%BE%D0%BD%D1%82%D0%B0%D0%BA%D1%82", info);
        KS_TEST_URL("https://www.google.ru/#q=%D0%BA%D0%BE%D0%BD%D1%82%D0%B0%D0%BA%D1%82", info);
        info.Query = "123";
        info.Type = ST_IMAGES;
        KS_TEST_URL("https://www.google.com/#q=123&tbm=isch", info);
        KS_TEST_URL("https://www.google.com/#tbm=isch&q=123", info);

        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_SEARCH);

        info.Query = "Cow in forest";
        KS_TEST_URL("http://www.google.ru/search?hl=ru&newwindow=1&site=&q=Cow+in+forest&oq=Cow+in+forest&aq=f&aqi=g-vL1&aql=&gs_sm=e&gs_upl=25134l33554l0l35848l10l7l0l2l2l0l689l2769l3-1.1.3l5l0", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.google.ru/search?tbm=isch&source=mog&hl=ru&gl=ru&q=Cow%20in%20forest&sa=N&biw=320&bih=481", info);

        info.Query = "ku";
        //KS_TEST_URL("http://www.google.com/images?hl=en&q=ku&um=1&ie=UTF-8&source=og&sa=N&tab=wi", info);

        info.Query = "хрен";
        //KS_TEST_URL("google.com/images?um=1&hl=ru&tbs=isch%3A1&sa=1&q=%D1%85%D1%80%D0%B5%D0%BD&aq=f&aqi=g4g-s1g5&aql=&oq=&gs_rfai=", info);

        info.Query = "cow in forest";
        info.Type = ST_COM;
        KS_TEST_URL("http://www.google.com/finance?q=cow+in+forest&hl=en&um=1&bav=on.2,or.r_gc.r_pw.,cf.osb&biw=1538&bih=1115&ie=UTF-8&sa=N&tab=pe", info);

        info.Query = "123";
        info.Type = ST_SCIENCE;
        KS_TEST_URL("http://scholar.google.com/scholar?q=123&hl=en&btnG=Search&as_sdt=1%2C5&as_sdtp=on", info);

        info.Type = ST_BLOGS;
        KS_TEST_URL("http://www.google.com/search?q=123&hl=en&tbo=u&tbm=blg&source=og&sa=N&tab=sb", info);

        info.Type = ST_CATALOG;
        info.Flags = ESearchFlags(SF_SEARCH);
        KS_TEST_URL("https://sites.google.com/site/sites/system/app/pages/meta/search?scope=my&q=123", info);

        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);

        info.Type = ST_CATALOG;
        KS_TEST_URL("http://www.google.com/mobile/sitesearch/index.html#q=123", info);
        KS_TEST_URL("http://www.google.com/mobile/sitesearch/index.html#q=123", info);

        info.Flags = NSe::ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("https://www.google.com/mobile/sitesearch/index.html#q=123", info);

        info.Flags = SF_SEARCH;
        KS_TEST_URL("http://www.google.com/sitesearch/index.html#q=123", info);

        info.Query = "Cow";
        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("http://www.google.ru/m/places?source=mog&hl=ru&gl=ru&sa=N#ipd:source=ipd&mode=search&q=Cow", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://www.google.ru/m/search?tbm=nws&hl=ru&source=mog&hl=ru&gl=ru&q=Cow&sa=N", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.google.ru/m/search?tbm=vid&source=mog&hl=ru&gl=ru&q=Cow&sa=N", info);

        info.Query = "хрен";
        //KS_TEST_URL("http://www.google.com/search?um=1&hl=ru&q=%D1%85%D1%80%D0%B5%D0%BD&ie=UTF-8&tbo=u&tbs=isch:1,vid:1&source=og&sa=N&tab=iv", info);

        info.Query = "Культура и религия.";
        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_LOCAL | SF_SEARCH);
        KS_TEST_URL("http://www.google.ru/search?as_q=%CA%F3%EB%FC%F2%F3%F0%E0+%E8+%F0%E5%EB%E8%E3%E8%FF.&as_sitesearch=bestreferat.ru", info);

        info.Query = "123";
        info.Type = ST_IMAGES;
        info.Flags = SF_SEARCH;
        KS_TEST_URL("http://images.google.com/search?tbm=isch&hl=en&source=hp&biw=1538&bih=1115&q=123&gbv=2&oq=123&aq=f&aqi=g10&aql=&gs_sm=e&gs_upl=1849l2006l0l2406l3l2l0l0l0l0l190l266l1.1l2l0", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.google.com/search?q=123&tbo=p&tbm=vid&source=vgc&aq=f", info);

        info.Query = "fake";
        info.Type = ST_MAPS;
        KS_TEST_URL("http://maps.google.ru/maps?um=1&hl=ru&newwindow=1&q=fake&gs_sm=e&gs_upl=6349l6700l0l6788l4l3l0l0l0l0l104l186l1.1l2l0&bav=on.2,or.r_gc.r_pw.r_cp.,cf.osb&biw=1538&bih=1115&ie=UTF-8&sa=N&tab=vl", info);

        info.Query = "Москва";
        KS_TEST_URL("https://www.google.ru/maps/place/Москва/@55.749792,37.632495,10z/data=!3m1!4b1!4m2!3m1!1s0x46b54afc73d4b0c9:0x3d44d6cc5757cf4c", info);
        info.Query = "Банк Москва-Сити";
        KS_TEST_URL("https://www.google.ru/maps/place/Банк+Москва-Сити/@55.7258456,37.6496435,17z/data=!3m1!4b1!4m2!3m1!1s0x46b54b213c4fade7:0xe7a5396fdfc04e5", info);

        info.Query = "5 antoine st rydalmere";
        KS_TEST_URL("https://www.google.com.au/maps?biw=1366&bih=677&q=5%20antoine%20st%20rydalmere&bav=on.2,or.r_cp.r_qf.&bvm=bv.83829542,d.dGY&dpr=1&um=1&ie=UTF-8&sa=X&ei=NXm4VInzEqXOmwX4oYKYDA&ved=0CAYQ_AUoAQ", info);

        info.Query = "islas filipinas";
        KS_TEST_URL("https://www.google.es/maps/search/islas%20filipinas/@41.3925908,2.1165581,15z", info);

        info.Query = "take away food";
        KS_TEST_URL("https://www.google.com.au/maps/search/take%20away%20food/@-33.922878,151.204688,17z/data=!3m1!4b1!4m5!2m4!3m3!1stake%20away%20food!2s1A%20Dougherty%20St,%20Rosebery%20NSW%202018!3s0x6b12b1a3f422a615:0x4e3fa4e76a56769", info);

        info.Query = "cow in forest";
        info.Type = ST_ANSWER;
        KS_TEST_URL("http://otvety.google.ru/otvety/search?hl=ru&sa=N&tab=l2&q=cow+in+forest", info);
        info.Query = "what is cat";
        KS_TEST_URL("http://www.google.com/search?q=what+is+cat&btnG=Search+Answers", info);
    }
}
