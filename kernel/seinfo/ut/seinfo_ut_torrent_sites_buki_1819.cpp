#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc57() {
    // Torrent sites. BUKI-1819
    {
        TInfo info(SE_UNKNOWN, ST_TORRENTS, "ford focus", ESearchFlags(SF_SEARCH | SF_LOCAL));

        info.Name = SE_RUTRACKER_ORG;
        KS_TEST_URL("http://rutracker.org/forum/tracker.php?nm=ford%20focus", info);
        KS_TEST_URL("http://rutracker.org/forum/tracker.php?f=2093&nm=ford%20focus", info);

        info.Name = SE_RUTOR_ORG;
        KS_TEST_URL("http://rutor.org/search/0/0/000/0/ford%20focus", info);

        info.Name = SE_TORRENTINO_COM;
        KS_TEST_URL("http://www.torrentino.com/search?search=ford+focus", info);

        info.Name = SE_FAST_TORRENT_RU;
        KS_TEST_URL("http://www.fast-torrent.ru/search/ford%20focus/1.html", info);

        info.Name = SE_TFILE_ME;
        KS_TEST_URL("http://tfile.me/forum/search?q=ford+focus", info);

        info.Name = SE_KINOZAL_TV;
        KS_TEST_URL("http://kinozal.tv/browse.php?s=ford+focus", info);

        info.Name = SE_NNM_CLUB_RU;
        KS_TEST_URL("http://nnm-club.ru/forum/search.php?cx=partner-pub-7768347321290299%3Akaazje-y1e4&cof=FORID%3A10&ie=windows-1251&q=ford+focus&sa=%CF%EE%E8%F1%EA", info);

        info.Name = SE_EX_UA;
        KS_TEST_URL("http://www.ex.ua/search?s=ford+focus", info);

        info.Name = SE_TORRENT_GAMES_NET;
        KS_TEST_URL("http://torrent-games.net/search/?sfSbm=search&sfSbm=search&q=ford+focus&x=0&y=0", info);

        info.Name = SE_MY_HIT_RU;
        KS_TEST_URL("http://my-hit.ru/index.php?module=search&func=view&result_orderby=score&result_order_asc=0&search_string=ford+focus&x=0&y=0", info);

        info.Name = SE_TORRENTINO_RU;
        KS_TEST_URL("http://www.torrentino.ru/search?utf8=%E2%9C%93&kind=0&search=ford+focus", info);

        info.Name = SE_X_TORRENTS_ORG;
        KS_TEST_URL("http://x-torrents.org/browse.php?search=ford+focus&x=0&y=0", info);

        info.Name = SE_TORRENTSMD_COM;
        KS_TEST_URL("http://torrentsmd.com/search.php?search_str=ford+focus", info);
        KS_TEST_URL("http://www.torrentsmd.com/search.php?search_str=ford+focus", info);

        info.Name = SE_TORRNADO_RU;
        KS_TEST_URL("http://www.torrnado.ru/search_new.php?q=ford+focus", info);

        info.Name = SE_RIPER_AM;
        KS_TEST_URL("http://riper.am/search.php?keywords=ford+focus&sr=topics&sf=titleonly&fp=1&tracker_search=torrent&sid=8b1a0f804d2c6e1a5d350865a17798a5#sr", info);

        info.Name = SE_KINOVIT_RU;
        KS_TEST_URL("http://www.kinovit.ru/search.html?s=ford+focus&x=0&y=0", info);

        info.Name = SE_KATUSHKA_NET;
        KS_TEST_URL("http://katushka.net/torrents/?tags=&search=ford+focus&type_search=groups&incldead=0&sorting=0&type_sort=desc", info);

        info.Name = SE_KAT_PH;
        KS_TEST_URL("http://kat.ph/usearch/ford%20focus/", info);

        info.Name = SE_WEBURG_NET;
        KS_TEST_URL("http://weburg.net/search/?where=0&search=1&q=ford+focus", info);

        info.Name = SE_KINOKOPILKA_TV;
        KS_TEST_URL("http://www.kinokopilka.tv/search?search_mode=movies&q=ford+focus", info);

        info.Name = SE_FILEBASE_WS;
        KS_TEST_URL("http://www.filebase.ws/torrents/search/?search=ford+focus&c=0&t=liveonly", info);

        info.Name = SE_EVRL_TO;
        KS_TEST_URL("http://evrl.to/games/search/?search=ford%20focus", info);

        info.Name = SE_ARENABG_COM;
        info.Query = "winter is coming";
        KS_TEST_URL("http://0.arenabg.com/torrents/search:winter+is+coming/status:0/", info);

        info.Name = SE_4FUN_MKSAT_NET;
        info.Query = "call of duty";
        KS_TEST_URL("http://4fun.mksat.net/browse.php?search=call+of+duty&x=-228&y=-195", info);

        info.Name = SE_BEETOR_ORG;
        info.Query = "winter is coming";
        KS_TEST_URL("http://beetor.org/index.php?page=torrents&c%5B%5D=1&s=winter+is+coming&a%5B%5D=1&a%5B%5D=2&u=", info);

        info.Name = SE_EXTRATORRENT_COM;
        info.Query = "winter is coming";
        KS_TEST_URL("http://extratorrent.com/search/?new=1&search=winter+is+coming&s_cat=8", info);

        info.Name = SE_ISOHUNT_COM;
        info.Query = "winter is coming";
        KS_TEST_URL("http://isohunt.com/torrents/?ihq=winter+is+coming", info);

        info.Name = SE_NEWTORR_ORG;
        info.Query = "winter is coming";
        KS_TEST_URL("http://newtorr.org/index.php?page=torrents&search=winter+is+coming&active=0", info);

        info.Name = SE_RARBG_COM;
        info.Query = "AbbyWinters";
        KS_TEST_URL("http://rarbg.com/torrents.php?search=AbbyWinters&category=0&page=4", info);

        info.Name = SE_TRACKER_NAME;
        info.Query = "winter";
        KS_TEST_URL("http://tracker.name/index.php?page=torrents&search=winter&category=36&active=0", info);

        info.Name = SE_BIGFANGROUP_ORG;
        info.Query = "winter";
        KS_TEST_URL("http://www.bigfangroup.org/browse.php?search=winter&cat=0&incldead=0&year=\
0&format=0#%2Fbrowse.php%3Fajax%3D1%26search%3Dwinter%26cat%3D0%26incldead%3D0%26year%3D0%26format%\
3D0",
                    info);

        info.Name = SE_FAST_TORRENT_RU;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.fast-torrent.ru/search/winter%20is%20coming/1.html", info);

        info.Name = SE_FILE_LU;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.file.lu/browse/pub/added:desc/(cats:ba612313,8aa23353)(str:winter+is+coming).html", info);

        info.Name = SE_FILEBASE_WS;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.filebase.ws/torrents/search/?search=winter+is+coming&c=0&t=liveonly", info);

        info.Name = SE_LINKOMANIJA_NET;
        info.Query = "windows 7";
        KS_TEST_URL("http://www.linkomanija.net/browse.php?incldead=0&search=windows+7", info);

        info.Name = SE_OMGTORRENT_COM;
        info.Query = "winter";
        KS_TEST_URL("http://www.omgtorrent.com/recherche/?query=winter", info);

        info.Name = SE_RUTOR_ORG;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.rutor.org/search/winter%20is%20coming", info);

        info.Name = SE_SMARTORRENT_COM;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.smartorrent.com/?page=search&term=winter+is+coming&x=-1288&y=-101", info);

        info.Name = SE_T411_ME;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.t411.me/torrents/search/?search=winter+is+coming", info);

        info.Name = SE_TORRENT_AI;
        info.Query = "Les visiteurs";
        KS_TEST_URL("http://www.torrent.ai/v2/torrents-search.php?in=1&search=Les+visiteurs", info);

        info.Name = SE_TORRENTDOWNLOADS_ME;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.torrentdownloads.me/search/?search=winter+is+coming", info);

        info.Name = SE_TORRENTREACTOR_NET;
        info.Query = "winter is coming";
        KS_TEST_URL("http://www.torrentreactor.net/torrent-search/winter+is+coming", info);

        info.Name = SE_TORRENTSMD_COM;
        info.Query = "crazy frog";
        KS_TEST_URL("http://www.torrentsmd.com/search.php?search_str=crazy+frog", info);

        info.Name = SE_ZAMUNDA_NET;
        info.Query = "flight simulator";
        KS_TEST_URL("http://zamunda.net/browse.php?search=flight+simulator+&cat=4&incldead=1&field=name", info);

        info.Name = SE_RETRE_ORG;
        info.Query = "Nurse 3-D";
        KS_TEST_URL("http://retre.org/?search=Nurse+3-D", info);

        info.Name = SE_MIX_SIBNET_RU;
        info.Query = "GTA IV";
        KS_TEST_URL("http://mix.sibnet.ru/movie/search/?keyword=GTA+IV", info);
    }
}
