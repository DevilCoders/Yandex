#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc81() {
    // LOGSTAT-4538. Popular online shops
    {
        TInfo info(SE_AVITO, ST_COM, "картонные коробки", SF_SEARCH);

        KS_TEST_URL("https://www.avito.ru/moskva?q=картонные+коробки", info);

        info.Query = "самоделка";
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("https://m.avito.ru/ufa?q=%D1%81%D0%B0%D0%BC%D0%BE%D0%B4%D0%B5%D0%BB%D0%BA%D0%B0", info);
        info.Flags = ESearchFlags(SF_SEARCH);

        info.Name = SE_ALIEXPRESS;
        info.Query = "кровать";
        KS_TEST_URL("http://ru.aliexpress.com/premium/%25D0%25BA%25D1%2580%25D0%25BE%25D0%25B2%25D0%25B0%25D1%2582%25D1%258C.html?ltype=wholesale&SearchText=кровать&d=y&origin=y&initiative_id=SB_20150616032341&isViewCP=y&catId=0", info);

        info.Query = "кукушка";
        KS_TEST_URL("http://ru.aliexpress.com/premium/%25D0%25BA%25D1%2583%25D0%25BA%25D1%2583%25D1%2588%25D0%25BA%25D0%25B0.html?ltype=wholesale&SearchText=кукушка&d=y&origin=y&initiative_id=SB_20150616032404&isViewCP=y&catId=0", info);

        info.Query = "milk";
        KS_TEST_URL("http://www.aliexpress.com/af/milk.html?ltype=wholesale&SearchText=milk&d=y&origin=n&initiative_id=SB_20150616032447&isViewCP=y&catId=0", info);

        info.Name = SE_OZON;
        KS_TEST_URL("http://www.ozon.ru/?context=search&text=milk", info);

        info.Name = SE_WILDBERRIES;
        KS_TEST_URL("http://www.wildberries.ru/catalog/0/search.aspx?search=milk", info);

        info.Name = SE_LAMODA;
        KS_TEST_URL("http://www.lamoda.ru/catalogsearch/result/?q=milk&from=button&submit=y", info);

        info.Name = SE_ULMART;
        KS_TEST_URL("http://www.ulmart.ru/search?string=milk&rootCategory=&sort=6", info);

        info.Name = SE_ASOS;
        KS_TEST_URL("http://www.asos.com/ru/search/milk?q=milk", info);
        info.Query = "beer";
        KS_TEST_URL("http://www.asos.com/ru/search/beer?q=beer", info);

        info.Name = SE_003_RU;
        info.Query = "milk";
        KS_TEST_URL("http://www.003.ru/?find=milk", info);

        info.Name = SE_BOOKING_COM;
        info.Query = "баден баден";
        KS_TEST_URL("http://www.booking.com/searchresults.ru.html?src=index&nflt=&ss_raw=&error_url=http%3A%2F%2Fwww.booking.com%2Findex.ru.html%3Faid%3D397643%3Blabel%3Dyan104jc-index-XX-XX-XX-unspec-ru-com-L%253Aru-O%253Aunk-B%253Aunk-N%253Ayes-S%253Abo%3Bsid%3Da2fa5e32799d42bf93a69e7da0c2d2b1%3Bdcid%3D4%3B&bb_asr=2&aid=397643&dcid=4&label=yan104jc-index-XX-XX-XX-unspec-ru-com-L%3Aru-O%3Aunk-B%3Aunk-N%3Ayes-S%3Abo&sid=a2fa5e32799d42bf93a69e7da0c2d2b1&si=ai%2Cco%2Cci%2Cre%2Cdi&ss=баден+баден&idf=on&no_rooms=1&group_adults=2&group_children=0", info);

        info.Name = SE_AFISHA_RU;
        info.Query = "средние века";
        KS_TEST_URL("http://www.afisha.ru/Search/?Search_str=средние%20века", info);
    }
}
