#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc50() {
    // news + local
    {
        TInfo info(SE_RIA, ST_NEWS, "ford focus", ESearchFlags(SF_SEARCH | SF_LOCAL));
        KS_TEST_URL("http://www.ria.ru/search/?query=ford+focus&s_type=and&start_day=01&start_month=01&start_year=2004&end_day=25&end_month=10&end_year=2012", info);
        info.Name = SE_LENTA_RU;
        KS_TEST_URL("http://lenta.ru/search/?query=ford+focus&x=0&y=0&where=on#%7B%22query%22%3A%22ford%2Bfocus%22%2C%22scope%22%3A0%2C%22count%22%3A20%2C%22sort%22%3A2%2C%22page%22%3A1%2C%22domain%22%3A1%7D", info);
        info.Name = SE_GAZETA_RU;
        KS_TEST_URL("http://www.gazeta.ru/search.shtml?text=ford+focus&how=pt&article=&ind=", info);
        info.Name = SE_NEWSRU;
        KS_TEST_URL("http://newsru.com/search.html?sort=2&qry=ford+focus&x=0&y=0", info);
        info.Name = SE_VESTI_RU;
        KS_TEST_URL("http://www.vesti.ru/search_adv.html?q=ford+focus&stype_n=rnews&stype_v=rvideo&sdate_start=&sdate_end=&sorder=date", info);
        info.Name = SE_RBC_RU;
        KS_TEST_URL("http://www.search.rbc.ru/?query=ford+focus", info);
        info.Name = SE_VZ_RU;
        KS_TEST_URL("http://vz.ru/search/?action=search&s_string=ford+focus", info);
        info.Name = SE_KP_RU;
        KS_TEST_URL("http://www.kp.ru/search/?cx=partner-pub-7127235648224767%3A8g2twv-uz6i&cof=FORID%3A11&ie=windows-1251&q=ford+focus&sa=&siteurl=www.kp.ru%2F&ref=&ss=1796j644442j9", info);
        info.Name = SE_REGNUM;
        KS_TEST_URL("http://regnum.ru/search/?txt=ford+focus", info);
        info.Name = SE_DNI_RU;
        KS_TEST_URL("http://dni.ru/search/?s_string=ford+focus&action=search", info);
        info.Name = SE_UTRO_RU;
        KS_TEST_URL("http://search.utro.ru/?query=ford+focus&howmany=10&morph=on&sort=date", info);
        info.Name = SE_MK_RU;
        KS_TEST_URL("http://www.mk.ru/search2/?search=ford+focus&sa=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA", info);
        info.Name = SE_AIF_RU;
        KS_TEST_URL("http://www.aif.ru/search?q=ford+focus+site%3Awww.aif.ru&cx=015017622151611200386%3A1-qgnyvx9mi&cof=FORID%3A11&ie=windows-1251&sa=%CF%EE%E8%F1%EA", info);
        info.Name = SE_RG_RU;
        KS_TEST_URL("http://search.rg.ru/wwwsearch/?text=ford+focus", info);
        info.Name = SE_VEDOMOSTI;
        KS_TEST_URL("http://www.vedomosti.ru/search/?order=date&s=ford+focus", info);
        info.Name = SE_RB_RU;
        KS_TEST_URL("http://www.rb.ru/search/?type=&search=ford+focus", info);
        info.Name = SE_TRUD_RU;
        KS_TEST_URL("http://www.trud.ru/search/?query=ford+focus", info);
        info.Name = SE_RIN_RU;
        KS_TEST_URL("http://news.rin.ru/search//////ford%20focus/", info);
        info.Name = SE_IZVESTIA;
        KS_TEST_URL("http://izvestia.ru/search?search=ford+focus", info);
        info.Name = SE_KOMMERSANT;
        KS_TEST_URL("http://kommersant.ru/Search/Results?places=1%2C2%2C3%2C5%2C6%2C14%2C17%2C52%2C57%2C61%2C62%2C66%2C84%2C86%2C198%2C210%2C217%2C9999&categories=20&isbankrupt=false&datestart=25.09.2012&dateend=25.10.2012&sort_type=0&sort_dir=0&region_selected=0&results_count=300&saved_query=&saved_statement=&page=1&search_query=ford+focus", info);
        info.Name = SE_EXPERT_RU;
        KS_TEST_URL("http://expert.ru/search/?search_text=ford+focus", info);
        info.Name = SE_INTERFAX;
        KS_TEST_URL("http://interfax.ru/archive.asp?sw=ford+focus&x=0&y=0", info);
        info.Name = SE_ECHO_MSK;
        KS_TEST_URL("http://echo.msk.ru/search/?search_cond%5Bquery%5D=ford+focus", info);
        info.Name = SE_NEWIZV;
        KS_TEST_URL("http://www.newizv.ru/search/?searchid=1943823&text=ford%20focus&web=0", info);
        info.Name = SE_RBC_DAILY;
        KS_TEST_URL("http://www.rbcdaily.ru/search/?query=ford+focus", info);
        info.Name = SE_ARGUMENTI;
        KS_TEST_URL("http://argumenti.ru/search?searchid=1953073&text=ford%20focus&web=0", info);
    }
}
