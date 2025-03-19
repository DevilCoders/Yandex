#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc61() {
    // More Google rules. BUKI-1857.
    {
        TInfo info(SE_GOOGLE);
        info.Flags = SF_SEARCH;
        info.Type = ST_WEB;
        info.Query = "одноклассники site:www.km.ru";
        KS_TEST_URL("www.google.ru/custom?&q=%D0%BE%D0%B4%D0%BD%D0%BE%D0%BA%D0%BB%D0%B0%D1%81%D1%81%D0%BD%D0%B8%D0%BA%D0%B8%20site:www.km.ru&client=pub-6204558141800493&forid=1&channel=9859602781&ie=utf-8&oe=utf-8&cof=GALT%3A%23008000%3BGL%3A1%3BDIV%3A%23F4F4F4%3BVLC%3A663399%3BAH%3Acenter%3BBGC%3AFFFFFF%3BLBGC%3A336699%3BALC%3A0000FF%3BLC%3A0000FF%3BT%3A000000%3BGFNT%3A0000FF%3BGIMP%3A0000FF%3BFORID%3A11&hl=ru&num=", info);

        info.Query = "рецeпты из говядины";
        info.SetPageSizeRaw(12);
        KS_TEST_URL("http://www.google.com.ua/cse?cref=http%3A%2F%2Fi.bigmir.net%2Fcse%2F2_ru.xml&cof=FORID%3A11&channel=5988463740&cr=countryUA&q=%D1%80%D0%B5%D1%86e%D0%BF%D1%82%D1%8B+%D0%B8%D0%B7+%D0%B3%D0%BE%D0%B2%D1%8F%D0%B4%D0%B8%D0%BD%D1%8B&ad=n9&num=12&rurl=http%3A%2F%2Fsearch.bigmir.net%2F%3Fcref%3Dhttp%253A%252F%252Fi.bigmir.net%252Fcse%252F2_ru.xml%26cof%3DFORID%253A11%26channel%3D5988463740%26cr%3DcountryUA%26z%3D%25D1%2580%25D0%25B5%25D1%2586%25D0%25BF%25D1%2582%25D1%258B%2B%25D0%25B8%25D0%25B7%2B%25D0%25B3%25D0%25BE%25D0%25B2%25D1%258F%25D0%25B4%25D0%25B8%25D0%25BD%25D1%258B&siteurl=http%3A%2F%2Fwww.bigmir.net%2F#gsc.tab=0&gsc.q=%D1%80%D0%B5%D1%86%D0%BF%D1%82%D1%8B%20%D0%B8%D0%B7%20%D0%B3%D0%BE%D0%B2%D1%8F%D0%B4%D0%B8%D0%BD%D1%8B&gsc.page=1", info);
        info.SetPageSizeRaw(TMaybe<ui32>());

        info.Query = "избавится от комлексов";
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("www.google.com/m?q=%D0%B8%D0%B7%D0%B1%D0%B0%D0%B2%D0%B8%D1%82%D1%81%D1%8F+%D0%BE%D1%82+%D0%BA%D0%BE%D0%BC%D0%BB%D0%B5%D0%BA%D1%81%D0%BE%D0%B2&client=ms-opera-mini&channel=new", info);

        info.Query = "Страшные истории на английском";
        info.Flags = SF_SEARCH;
        KS_TEST_URL("www.google.com/xhtml?q=%D0%A1%D1%82%D1%80%D0%B0%D1%88%D0%BD%D1%8B%D0%B5%20%D0%B8%D1%81%D1%82%D0%BE%D1%80%D0%B8%D0%B8%20%D0%BD%D0%B0%20%D0%B0%D0%BD%D0%B3%D0%BB%D0%B8%D0%B9%D1%81%D0%BA%D0%BE%D0%BC&client=ms-opera_mb_no&channel=bh", info);

        info.Query = "metallica unforgiven табы";
        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        KS_TEST_URL("www.google.com/search?q=metallica+unforgiven+%D1%82%D0%B0%D0%B1%D1%8B&hl=ru&client=ms-opera-mobile&gs_l=mobile-heirloom-hp.12..0i13l2j0i13i30l3.16656.32972.0.35823.5.4.0.1.1.0.2783.7824.6-1j0j1j2.4.0...0.0...1c.1.wx3wOIqrwUg&sa=X&as_q=&nfpr=&spell=1", info);

        info.Query = "достопримечательности ноябрьска";
        info.Flags = SF_SEARCH;
        info.Type = ST_BYIMAGE;
        KS_TEST_URL("http://www.google.ru/imgres?q=%D0%B4%D0%BE%D1%81%D1%82%D0%BE%D0%BF%D1%80%D0%B8%D0%BC%D0%B5%D1%87%D0%B0%D1%82%D0%B5%D0%BB%D1%8C%D0%BD%D0%BE%D1%81%D1%82%D0%B8+%D0%BD%D0%BE%D1%8F%D0%B1%D1%80%D1%8C%D1%81%D0%BA%D0%B0&hl=ru&newwindow=1&tbo=d&rlz=1W1ADSA_ruRU471&biw=1280&bih=586&tbm=isch&tbnid=ILDeESgxDbTaJM:&imgrefurl=http://www.yamal.ru/new/gor03.htm&docid=FoMHGeejnP1rxM&imgurl=http://www.yamal.ru/new/foto/noyabrsk.JPG&w=1166&h=641&ei=86MPUYLlM8jHswaN3IG4Ag&zoom=1&iact=rc&dur=141&sig=108072230898154502723&page=1", info);

        // TODO: think here
        info.Query = "вакансии учитель в казань, республика татарстан";
        info.Type = ST_WEB;
        info.SetPageSizeRaw(0); // Wtf?!
        KS_TEST_URL("www.google.com/uds/afs?q=%D0%B2%D0%B0%D0%BA%D0%B0%D0%BD%D1%81%D0%B8%D0%B8%20%D1%83%D1%87%D0%B8%D1%82%D0%B5%D0%BB%D1%8C%20%D0%B2%20%D0%BA%D0%B0%D0%B7%D0%B0%D0%BD%D1%8C%2C%20%D1%80%D0%B5%D1%81%D0%BF%D1%83%D0%B1%D0%BB%D0%B8%D0%BA%D0%B0%20%D1%82%D0%B0%D1%82%D0%B0%D1%80%D1%81%D1%82%D0%B0%D0%BD&client=trovit-browse&channel=ru_jobs&hl=ru&adtest=off&oe=utf8&ie=utf8&r=s&qry_lnk=%D1%83%D1%87%D0%B8%D1%82%D0%B5%D0%BB%D1%8C&qry_ctxt=%D0%B2%D0%B0%D0%BA%D0%B0%D0%BD%D1%81%D0%B8%D0%B8&adpage=2&adrep=2&fexp=21404&format=p3&ad=n0&nocache=111360006441431&num=0&output=uds_ads_only&v=3&adext=as1&rurl=http%3A%2F%2Fru.trovit.com%2Frabota%2F%25D1%2583%25D1%2587%25D0%25B8%25D1%2582%25D0%25B5%25D0%25BB%25D1%258C-%25D1%2580%25D0%25B0%25D0%25B1%25D0%25BE%25D1%2582%25D0%25B0-%25D0%25B2-%25D0%25BA%25D0%25B0%25D0%25B7%25D0%25B0%25D0%25BD%25D1%258C%2C-%25D1%2580%25D0%25B5%25D1%2581%25D0%25BF%25D1%2583%25D0%25B1%25D0%25BB%25D0%25B8%25D0%25BA%25D0%25B0-%25D1%2582%25D0%25B0%25D1%2582%25D0%25B0%25D1%2580%25D1%2581%25D1%2582%25D0%25B0%25D0%25BD%2F2&referer=http%3A%2F%2Fru.trovit.com%2Frabota%2F%25D1%2583%25D1%2587%25D0%25B8%25D1%2582%25D0%25B5%25D0%25BB%25D1%258C-%25D1%2580%25D0%25B0%25D0%25B1%25D0%25BE%25D1%2582%25D0%25B0-%25D0%25B2-%25D0%25BA%25D0%25B0%25D0%25B7%25D0%25B0%25D0%25BD%25D1%258C%2C-%25D1%2580%25D0%25B5%25D1%2581%25D0%25BF%25D1%2583%25D0%25B1%25D0%25BB%25D0%25B8%25D0%25BA%25D0%25B0-%25D1%2582%25D0%25B0%25D1%2582%25D0%25B0%25D1%2580%25D1%2581%25D1%2582%25D0%25B0%25D0%25BD&loader=alt", info);
    }
}
