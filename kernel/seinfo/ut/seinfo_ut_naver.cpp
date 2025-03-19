#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc55() {
    // naver
    {
        TInfo info(SE_NAVER, ST_WEB, "ford focus", SF_SEARCH);

        KS_TEST_URL("http://search.naver.com/search.naver?where=nexearch&query=ford+focus&sm=top_hty&fbm=1&ie=utf8", info);
        KS_TEST_URL("http://search.naver.com/search.naver?where=nexearch&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        KS_TEST_URL("http://web.search.naver.com/search.naver?where=webkr&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        KS_TEST_URL("http://web.search.naver.com/search.naver?where=site&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        KS_TEST_URL("http://realtime.search.naver.com/search.naver?where=realtime&sm=tab_jum&ie=utf8&query=ford+focus", info); // ??
        KS_TEST_URL("http://time.search.naver.com/search.naver?where=timentopic&sm=tab_jum&ie=utf8&query=ford+focus", info);   // ??

        info.Type = ST_BLOGS;
        KS_TEST_URL("http://cafeblog.search.naver.com/search.naver?where=post&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        info.Flags = ESearchFlags(SF_LOCAL | SF_SEARCH);
        KS_TEST_URL("http://cafeblog.search.naver.com/search.naver?where=cafeblog&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        info.Flags = SF_SEARCH;

        info.Type = ST_ENCYC;
        KS_TEST_URL("http://kin.search.naver.com/search.naver?where=kin&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        KS_TEST_URL("http://dic.search.naver.com/search.naver?where=ldic&sm=tab_gnb&ie=utf8&query=ford+focus", info);
        KS_TEST_URL("http://dic.search.naver.com/search.naver?where=kdic&sm=tab_gnb&ie=utf8&query=ford+focus", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://news.search.naver.com/search.naver?where=news&sm=tab_gnb&ie=utf8&query=ford+focus", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://image.search.naver.com/search.naver?where=image&sm=tab_gnb&ie=utf8&query=ford+focus", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.search.naver.com/search.naver?where=video&sm=tab_gnb&ie=utf8&query=ford+focus", info);

        info.Type = ST_COM; // market
        KS_TEST_URL("http://shopping.naver.com/search/all_search.nhn?where=all&frm=NVSCTAB&query=ford+focus", info);
        KS_TEST_URL("http://shopping.naver.com/search/all_search.nhn?where=all&frm=NVSCTAB&query=ford+focus#", info);
        KS_TEST_URL("http://shopping.naver.com/search/all_search.nhn?query=ford+focus&iq=&cat_id=&frm=NVSHSRC", info);

        info.Type = ST_MAPS;
        info.Query = "Zm9yZCBmb2N1cw"; // Base64
        KS_TEST_URL("http://map.naver.com/index.nhn?query=Zm9yZCBmb2N1cw&enc=b64&tab=1", info);
        info.Query = "ford focus";

        info.Type = ST_MUSIC;
        KS_TEST_URL("http://music.naver.com/search/search.nhn?query=ford+focus", info);

        info.Type = ST_MAGAZINES;
        KS_TEST_URL("http://magazine.search.naver.com/search.naver?where=magazine&sm=tab_jum&ie=utf8&query=ford+focus", info);

        info.Type = ST_BOOKS;
        KS_TEST_URL("http://book.naver.com/search/search.nhn?query=ford+focus", info);

        info.Type = ST_SCIENCE;
        KS_TEST_URL("http://academic.naver.com/search.nhn?query=ford+focus", info);
        KS_TEST_URL("http://academic.naver.com/search.nhn?field=0&dir_id=1&query=ford+focus&gk_qdt=&qvt=1&q_title=&q_author=&q_journal=&q_source=&q_keyword=&q_volume_issue=&q_applicationnum=", info);
        KS_TEST_URL("http://academic.naver.com/noResult.nhn?dir_id=4&field=0&unFold=false&gk_adt=0&sort=0&qvt=1&query=ford%20focus&gk_qvt=1&citedSearch=false", info);

        info.Flags = ESearchFlags(SF_MOBILE | SF_SEARCH);
        info.Type = ST_WEB;
        KS_TEST_URL("http://m.search.naver.com/search.naver?query=ford+focus&where=m&sm=mtp_hty", info);
        info.Type = ST_BLOGS;
        KS_TEST_URL("http://m.search.naver.com/search.naver?where=m_blog&sm=mtb_jum&query=ford+focus", info);
        info.Type = ST_ENCYC;
        KS_TEST_URL("http://m.search.naver.com/search.naver?where=m_kin&sm=mtb_jum&query=ford+focus", info);
    }
}
