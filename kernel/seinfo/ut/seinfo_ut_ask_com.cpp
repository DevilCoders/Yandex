#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc42() {
    // ask.com
    {
        TInfo info(SE_ASK_COM, ST_WEB, "123", SF_SEARCH);

        KS_TEST_URL("http://www.ask.com/web?q=123&search=&qsrc=0&o=0&l=dir", info);
        KS_TEST_URL("http://www.search.ask.com/web?l=dis&q=123&o=APN10641A&apn_dtid=^IME002^YY^TR&shad=s_0048&gct=hp&apn_ptnrs=AG2&d=2-0&lang=tr&atb=sysid%3D2%3Auid%3D12af4badfbab4a6c%3Auc2%3D93%3Atypekbn%3Du9092%3Asrc%3Dhmp%3Ao%3DAPN10641A", info);
        KS_TEST_URL("http://ru.ask.com/web?q=123&qsrc=999&l=sem&siteid=2854&qenc=utf-8&ifr=1&ad=semA&an=google_s&mty=b&kwd=%D1%81%D0%BC%D0%BE%D1%82%D1%80%D0%B5%D1%82%D1%8C%20%D1%84%D0%B8%D0%BB%D1%8C%D0%BC%D1%8B%202013%20%D0%BD%D0%BE%D0%B2%D0%B8%D0%BD%D0%BA%D0%B8&net=s&cre=18238482977&pla=&mob=&sou=s&aid=&adp=1t1&kwid=50623714217&agid=2806203857", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.ask.com/pictures?o=0&l=dir&qsrc=272&q=123", info);
        KS_TEST_URL("http://www.search.ask.com/pictures?apn_dtid=%5EIME002%5EYY%5ETR&apn_dbr=&d=2-0&itbv=&crxv=&atb=sysid%3D2%3Auid%3D12af4badfbab4a6c%3Auc2%3D93%3Atypekbn%3Du9092%3Asrc%3Dhmp%3Ao%3DAPN10641A&hdoi=&p2=%5EAG2%5EIME002%5EYY%5ETR&apn_ptnrs=AG2&o=APN10641A&lang=tr&gct=hp&tbv=&pf=&tpid=&shad=s_0048&q=123&trgb=&hpds=&apn_sauid=&apn_uid=&tpr=10&doi=", info);
        KS_TEST_URL("http://ru.ask.com/pictures?qsrc=167&q=123&o=2854&l=sem", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://www.ask.com/news?o=0&l=dir&qsrc=168&q=123", info);
        KS_TEST_URL("http://www.ask.com/ref?o=0&l=dir&qsrc=180&q=123", info);
        KS_TEST_URL("http://www.search.ask.com/news?apn_dtid=%5EIME002%5EYY%5ETR&apn_dbr=&d=2-0&itbv=&crxv=&atb=sysid%3D2%3Auid%3D12af4badfbab4a6c%3Auc2%3D93%3Atypekbn%3Du9092%3Asrc%3Dhmp%3Ao%3DAPN10641A&hdoi=&p2=%5EAG2%5EIME002%5EYY%5ETR&apn_ptnrs=AG2&o=APN10641A&lang=tr&gct=hp&tbv=&pf=&tpid=&shad=s_0048&q=123&trgb=&hpds=&apn_sauid=&apn_uid=&tpr=10&doi=", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.ask.com/videos?o=0&l=dir&qsrc=28&q=123", info);
        KS_TEST_URL("http://www.search.ask.com/videos?q=123&apn_dtid=%5EIME002%5EYY%5ETR&apn_dbr=&d=2-0&itbv=&crxv=&atb=sysid%3D2%3Auid%3D12af4badfbab4a6c%3Auc2%3D93%3Atypekbn%3Du9092%3Asrc%3Dhmp%3Ao%3DAPN10641A&hdoi=&p2=%5EAG2%5EIME002%5EYY%5ETR&apn_ptnrs=AG2&o=APN10641A&lang=tr&gct=hp&tbv=&pf=&tpid=&shad=s_0048&trgb=&hpds=&apn_sauid=&apn_uid=&tpr=2&doi=", info);
        KS_TEST_URL("http://ru.ask.com/youtube?qsrc=168&q=123&o=2854&l=sem", info);

        info.Type = ST_COM;
        KS_TEST_URL("http://www.ask.com/shopping?o=0&l=dir&qsrc=3060&q=123", info);

        info.Query = "Google";
        info.Type = ST_MAPS;
        KS_TEST_URL("http://www.ask.com/local?what=Google&o=0&l=dir&where=Moscow+MOW", info);

        info.Query = "123";
        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://m.ask.com/wap4?sid=appserv02-ask-1322838809253&app=ask&crt=MAIN&trc=&QUERY=123&submit-WEBSEARCH11-BUTTONIMAGE1=Search", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://m.ask.com/wap4?sid=appserv02-ask-1322838809253&app=ask&crt=MAIN&trc=&QUERY=123&submit-IMAGES-BUTTONIMAGE2.x=41&submit-IMAGES-BUTTONIMAGE2.y=43", info);
        info.Type = ST_NEWS;
        KS_TEST_URL("http://m.ask.com/wap4?sid=appserv02-ask-1322838809253&app=ask&crt=MAIN&trc=&QUERY=123&submit-NEWS-BUTTONIMAGENEWS.x=31&submit-NEWS-BUTTONIMAGENEWS.y=43", info);
        info.Query = "Google";
        info.Type = ST_MAPS;
        KS_TEST_URL("http://m.ask.com/wap4?sid=appserv02-ask-1322838809253&app=ask&crt=FINDLOCATION&trc=0&SEARCHFOR=Google&NEAR=&submit-RESULTS-GO=Search", info);
    }
}
