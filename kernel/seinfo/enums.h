#pragma once

#include <util/stream/output.h>

namespace NSe
{
    enum ESearchEngine {
        SE_YANDEX,
        SE_GOOGLE,
        SE_RAMBLER,
        SE_MAIL,
        SE_BAIDU,
        SE_GOGO,
        SE_BING,
        SE_YAHOO,
        SE_NIGMA,
        SE_APORT,
        SE_ALL_BY,
        SE_TUT_BY,
        SE_BIGMIR,
        SE_META_UA,
        SE_SEARCH_COM,
        SE_AOL,
        SE_LYCOS,
        SE_QIP,
        SE_EXALEAD,
        SE_ALEXA,
        SE_WEBALTA,
        SE_QUINTURA,
        SE_METABOT_RU,
        SE_CONDUIT,
        SE_I_UA,
        SE_DAEMON_SEARCH,
        SE_INCREDIMAIL,
        SE_GDE_RU,
        SE_MY_WEB_SEARCH,
        SE_SEARCH_BABYLON,
        SE_SEZNAM_CZ, // seznam.cz web
        SE_MAPY_CZ,   // seznam.cz maps
        SE_ZBOZI_CZ,  // seznam.cz shopping
        SE_OBRAZKY_CZ,// seznam.cz images
        SE_FIRMY_CZ,  // firmy.cz  companies
        SE_GBG_BG,
        SE_NUR_KZ,
        SE_KAZAKH_RU,
        SE_DUCKDUCKGO,
        SE_GO_KM_RU,
        SE_GIGABASE_RU,
        SE_SEARCH_KAZ_KZ,
        SE_POISK_RU,
        SE_ICQ_COM,
        SE_ASK_COM,
        SE_BLEKKO,
        SE_TOPSY,
        SE_YOUTUBE,
        SE_TINEYE,
        SE_LEMOTEUR,
        SE_NAVER,
        SE_LIVEINTERNET,
        SE_HANDYCAFE,
        SE_SEARCHYA,
        SE_SOGOU,

        // News + Local
        SE_RIA,        // www.ria.ru
        SE_LENTA_RU,   // lenta.ru
        SE_GAZETA_RU,  // www.gazeta.ru
        SE_NEWSRU,     // newsru.com
        SE_VESTI_RU,   // vesti.ru
        SE_RBC_RU,     // rbc.ru
        SE_VZ_RU,      // vz.ru
        SE_KP_RU,      // kp.ru (комсомольская правда)
        SE_REGNUM,     // regnum.ru
        SE_DNI_RU,     // dni.ru
        SE_UTRO_RU,    // utro.ru
        SE_MK_RU,      // mk.ru (московский комсомолец)
        SE_AIF_RU,     // aif.ru (аргументы и факты)
        SE_RG_RU,      // rg.ru (русская газета)
        SE_VEDOMOSTI,  // vedomosti.ru
        SE_RB_RU,      // rb.ru
        SE_TRUD_RU,    // trud.ru
        SE_RIN_RU,     // rin.ru
        SE_IZVESTIA,   // izvestia.ru
        SE_KOMMERSANT, // kommersant.ru
        SE_EXPERT_RU,  // expert.ru
        SE_INTERFAX,   // interfax.ru
        SE_ECHO_MSK,   // echo.msk.ru (Эхо Москвы)
        SE_NEWIZV,     // newizv.ru (Новые известия)
        SE_RBC_DAILY,  // rbcdaily.ru
        SE_ARGUMENTI,  // argumenti.ru (Аргументы Недели)

        // Social networks
        SE_VKONTAKTE,
        SE_FACEBOOK,
        SE_TWITTER,
        SE_ODNOKLASSNIKI,
        SE_LIVEJOURNAL,
        SE_YA_RU,
        SE_LINKEDIN,
        SE_MAMBA,
        SE_SPRASHIVAI_RU,
        SE_TUMBLR,
        SE_MYSPACE,

        // Music
        SE_ITUNES,
        SE_ZVOOQ,
        SE_GROOVESHARK,
        SE_LAST_FM,
        SE_PROSTOPLEER,
        SE_SOUNDCLOUD,
        SE_MUZEBRA,
        // Ignored: SE_NEKTO_ME - query is hidden
        SE_WEBORAMA,
        SE_101_RU,
        // Ignored: SE_FREELAST - query is hidden
        SE_DEEZER,
        SE_PANDORA,

        // Work (popular in Russia)
        SE_MOIKRUG,
        SE_HH,        // hh.ru
        SE_RABOTA_RU, // rabota.ru
        SE_SUPERJOB,  // superjob.ru
        SE_JOB_RU,    // job.ru

        // Torrent portals
        SE_RUTRACKER_ORG,
        SE_RUTOR_ORG,
        SE_TORRENTINO_COM,
        SE_FAST_TORRENT_RU,
        SE_TFILE_ME,
        SE_KINOZAL_TV,
        // SE_PORNOLAB_NET - hidden query
        SE_NNM_CLUB_RU,
        SE_EX_UA,
        SE_TORRENT_GAMES_NET,
        SE_MY_HIT_RU,
        SE_RETRE_ORG,
        // SE_TOP_TORRENT_WS - hidden query
        SE_TORRENTINO_RU,
        // SE_BIGTORRENT_ORG - hidden
        SE_X_TORRENTS_ORG,
        SE_MIX_SIBNET_RU,
        // SE_TORRENTS_VTOMSKE_RU - not answering
        SE_TORRENTFILMS_NET,
        // SE_MANYTORRENTS_ORG - hidden query
        SE_SEEDOFF_NET,
        // SE_IGRI_2012_RU - hidden query
        SE_PORNOSHARA_TV,
        SE_TFILE_ORG,
        // SE_TREKERXXL_ORG - hidden query
        // SE_TRACKER_SPARK_MEDIA_RU - forbidden
        // SE_KAZTORKA_ORG - not answering
        // SE_BIGTORRENTS_ORG - hidden query
        // SE_LEPORNO_ORG - hidden query
        SE_TORRENTSZONA_COM,
        // SE_GIGATORRENT_NET - hidden query
        // SE_PIRATBIT_NET - hidden query
        SE_XXX_TRACKER_COM,
        SE_KUBALIBRE_COM,
        SE_FILMS_IMHONET_RU,
        SE_TORRENTSMD_COM,
        SE_TORRNADO_RU,
        SE_RIPER_AM,
        // SE_ZERX_RU - hidden query
        SE_KINOVIT_RU,
        // SE_HDPICTURE_RU - limited access
        // SE_TLTORRENT_RU - limited access
        SE_KATUSHKA_NET,
        SE_KAT_PH,
        // SE_WWW_NNTT_ORG - need sms to register
        // SE_RUSTORRENTS_NET - hidden query
        SE_WEBURG_NET,
        // SE_HDREACTOR_ORG - hidden query
        SE_KINOKOPILKA_TV,
        // SE_TORRENT_WINDOWS_NET - hidden query
        // SE_FILMS_TORRENT_RU - hiden query
        SE_FILEBASE_WS,
        // SE_MEGATORRENTS_ORG - hidden query
        // SE_RAPIDZONA_COM - hidden query
        // SE_CKOPO_NET - hidden query
        SE_EVRL_TO,
        SE_ARENABG_COM,
        SE_4FUN_MKSAT_NET,
        SE_BEETOR_ORG,
        SE_EXTRATORRENT_COM,
        SE_ISOHUNT_COM,
        SE_NEWTORR_ORG,
        SE_RARBG_COM,
        SE_TRACKER_NAME,
        SE_BIGFANGROUP_ORG,
        SE_FILE_LU,
        SE_LINKOMANIJA_NET,
        SE_OMGTORRENT_COM,
        SE_SMARTORRENT_COM,
        SE_T411_ME,
        SE_TORRENT_AI,
        SE_TORRENTDOWNLOADS_ME,
        SE_TORRENTREACTOR_NET,
        SE_ZAMUNDA_NET,

        // Video hostings
        SE_MYVI_RU,
        SE_RUTUBE,
        SE_KINOSTOK_TV,
        // SE_TVZAVR_RU - hidden query
        SE_UKRHOME_NET,
        SE_NAMBA_NET,
        SE_BIGCINEMA_TV,
        SE_KAZTUBE_KZ,
        SE_KIWI_KZ,
        SE_NOW_RU,
        SE_VIMEO,
        SE_AKILLI_TV,
        SE_DAILYMOTION_COM,
        SE_IZLEMEX_ORG,
        SE_KUZU_TV,
        SE_TEKNIKTV_COM,
        // SE_sendevideoizle.com - hidden query
        SE_IZLESENE_COM,
        // SE_facebooktanvideo.com - hidden query

        // Mail
        SE_LIVE_COM,
        SE_NGS_RU,

        // Advert (http://wiki.yandex-team.ru/users/mkot/Reklamnyeseti)
        SE_BANNERSBROKER_COM,
        SE_ADCASH_COM,
        SE_YIELDMANAGER_COM,
        SE_LUXUP_RU,
        SE_ADONWEB_RU,
        SE_ADVERTLINK_RU,
        SE_ADNXS_COM,
        SE_EXOCLICK_COM,
        SE_RECREATIV_RU,
        SE_RUCLICKS_COM,
        SE_LADYCENTER_RU,
        SE_MEDIALAND_RU,
        SE_ADMITAD_COM,
        SE_RMBN_NET,
        SE_TBN_RU,
        SE_ADNETWORK_PRO,
        SE_DT00_NET,
        SE_YADRO_RU,
        SE_KAVANGA_RU,
        SE_ADWOLF_RU,
        SE_MAGNA_RU,
        SE_GOODADVERT_RU,
        SE_ADRIVER_RU,
        SE_ADFOX_RU,
        SE_ADBLENDER_RU,
        SE_BEGUN_RU,
        SE_DIRECTADVERT_RU,
        SE_GAMELEADS_RU,
        SE_BUYSELLADS_COM,
        SE_CITYADS_RU,
        SE_MARKETGID_COM,

        // Popular porno hostings.
        SE_SEX_COM,
        SE_REDTUBE,
        SE_PORNHUB,
        SE_PORN_COM,
        SE_PORNTUBE,
        SE_XHAMSTER,
        SE_XVIDEOS,
        SE_YOUPORN,
        SE_TUBE8,
        SE_XNXX,
        SE_PORNMD,

        //sputnik.ru
        SE_SPUTNIK_RU,

        // istella.it
        SE_ISTELLA,

        // popular online shops
        SE_AVITO,
        SE_ALIEXPRESS,
        SE_OZON,
        SE_WILDBERRIES,
        SE_LAMODA,
        SE_ULMART,
        SE_ASOS,
        SE_003_RU,
        SE_BOOKING_COM,
        SE_AFISHA_RU,

        // Adclicks (Google)
        SE_IMPL_DOUBLECLICK,

        // ADS.
        SE_CRITEO,

        SE_UNKNOWN,
    };

    enum ESearchType {
        // common
        ST_WEB,
        ST_IMAGES,
        ST_VIDEO,
        // social+blogs+forums
        ST_BLOGS,
        ST_FORUM,
        ST_SOCIAL,   // Search results in various social networks.
        // news+events
        ST_NEWS,
        ST_EVENTS,
        // other
        ST_MUSIC,
        ST_COM,
        ST_TV,
        ST_GAMES,
        ST_ANSWER,
        ST_CATALOG,
        ST_APPS,
        ST_INTERESTS,
        ST_BOOKS,
        ST_PEOPLE,
        ST_ADRESSES,
        ST_MAPS,
        ST_ENCYC,
        ST_SCIENCE,
        ST_CARS,
        ST_SPORT,
        ST_RECIPES,
        ST_ABSTRACTS,
        ST_JOB,
        ST_BYIMAGE,
        ST_MAGAZINES,
        ST_TORRENTS,
        ST_ADV_SERP,
        ST_ADV_WEB,
        ST_PORTAL,

        ST_UNKNOWN,
    };

    // Note: SF_LOGINED is deprecated!
    enum ESearchFlags {
        SF_NO_FLAG =  0,
        SF_MOBILE = 1,
        SF_LOGINED = 2,
        SF_SOCIAL = 4,
        SF_LOCAL = 8,
        SF_PEOPLE = 16,
        SF_REDIRECT = 32,
        SF_MAIL = 64,
        SF_SEARCH = 128,
        SF_FAKE_SEARCH = 256,
        SF_PORTAL = 512,

        SF_TOTAL_FLAGS_COUNT = 9 // WARNING: Init carefully!
    };

    enum EPlatform {
        P_ANDROID,
        P_IPHONE,
        P_IPAD,
        P_WINDOWS_PHONE,
        P_WINDOWS_RT,

        P_UNKNOWN,
    };
};
