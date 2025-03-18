#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

#include <array>

namespace NAntiRobot {
    enum EReqType {
        REQ_OTHER = 0                /* "other" */,
        REQ_MAIN                     /* "main" */,
        REQ_YANDSEARCH               /* "ys" */,
        REQ_XMLSEARCH                /* "xml" */,
        REQ_CYCOUNTER                /* "cyc" */,
        REQ_OPENSEARCH               /* "opensearch" */,
        REQ_NEWS_CLICK               /* "newsclick" */,
        REQ_IMAGEPAGER               /* "imgp" */,
        REQ_MSEARCH                  /* "ms" */,
        REQ_FAMILYSEARCH             /* "family" */,
        REQ_SCHOOLSEARCH             /* "school" */,
        REQ_LARGESEARCH              /* "large" */,
        REQ_REDIR                    /* "redir" */,
        REQ_SITESEARCH               /* "sitesearch" */,
        REQ_IMAGE_LIKE               /* "imglike" */,
        REQ_FAVICON                  /* "favicon" */,
        REQ_YANDSEARCH_WITHOUT_LR    /* "nolr_ys" */,
        REQ_SERP_REQID_COUNTER       /* "serp_reqid_cnt" */,
        REQ_SUGGEST                  /* "suggest" */,
        REQ_STATIC                   /* "static" */,
        REQ_CATALOG_OFFERS           /* "catalogoffers" */,
        REQ_CATALOG_MODELS           /* "catalogmodels" */,
        REQ_CATALOG                  /* "catalog" */,
        REQ_COLLECTIONS              /* "collections" */,
        REQ_MODEL_PRICES             /* "modelprices" */,
        REQ_MODEL                    /* "model" */,
        REQ_GURU                     /* "guru" */,
        REQ_OFFERS                   /* "offers" */,
        REQ_MODEL_OPINIONS           /* "modelopinions" */,
        REQ_BRANDS                   /* "brands" */,
        REQ_COMPARE                  /* "compare" */,
        REQ_SHOP                     /* "shop" */,
        REQ_THEME                    /* "theme" */,
        REQ_CHECKOUT                 /* "checkout" */,
        REQ_ROUTE                    /* "route" */,
        REQ_ORG                      /* "org" */,
        REQ_TURBO                    /* "turbo" */,
        REQ_IMAGES                   /* "images" */,
        REQ_API                      /* "api" */,
        REQ_WEB                      /* "web" */,
        REQ_ANALYTICAL               /* "analytical" */,
        REQ_DZENSEARCH               /* "dzensearch" */,
        REQ_NUMTYPES
    };

    enum class EReqGroup : ui32 {
        Generic,
        Custom
    };

    enum class EVerticalReqGroup : ui32 {
        Generic                      /* "generic" */,
        BlackSearch                  /* "black-search" */,
        BlackOffer                   /* "black-offer" */,
        BlackPhone                   /* "black-phone" */,
        EventsLog                    /* "events-logs" */,
    };

    enum EHostType {
        HOST_OTHER = 0               /* "other" */,
        HOST_WEB                     /* "web" */,
        HOST_XMLSEARCH_COMMON        /* "xml_c" */,
        HOST_CLICK                   /* "click" */,
        HOST_IMAGES                  /* "img" */,
        HOST_NEWS                    /* "news" */,
        HOST_YACA                    /* "yaca" */,
        HOST_BLOGS                   /* "blogs" */,
        HOST_HILIGHTER               /* "hiliter" */,
        HOST_MARKET                  /* "market" */,
        HOST_RCA                     /* "rca" */,
        HOST_VIDEO                   /* "video" */,
        HOST_AVIA                    /* "avia" */,
        HOST_SLOVARI                 /* "slovari" */,
        HOST_TECH                    /* "tech" */,
        HOST_AUTO                    /* "auto" */,
        HOST_RABOTA                  /* "rabota" */,
        HOST_REALTY                  /* "realty" */,
        HOST_TRAVEL                  /* "travel" */,
        HOST_MUSIC                   /* "music" */,
        HOST_KINOPOISK               /* "kinopoisk" */,
        HOST_AUTORU                  /* "autoru" */,
        HOST_BUS                     /* "bus" */,
        HOST_WEBMASTER               /* "webmaster" */,
        HOST_WORDSTAT                /* "wordstat" */,
        HOST_MAPS                    /* "maps" */,
        HOST_POGODA                  /* "pogoda" */,
        HOST_SPRAVKA                 /* "sprav" */,
        HOST_ZEN                     /* "zen" */,
        HOST_AFISHA                  /* "afisha" */,
        HOST_COLLECTIONS             /* "collections" */,
        HOST_IZNANKA                 /* "iznanka" */,
        HOST_TAXI                    /* "taxi" */,
        HOST_KPAPI                   /* "kpapi" */,
        HOST_SUGGEST                 /* "suggest" */,
        HOST_MARKETPARTNER           /* "marketpartner" */,
        HOST_MARKETAPI               /* "marketapi" */,
        HOST_MARKETBLUE              /* "marketblue" */,
        HOST_PUBLICUGC               /* "publicugc" */,
        HOST_ZNATOKI                 /* "znatoki" */,
        HOST_LOCAL                   /* "local" */,
        HOST_TRANSLATE               /* "translate" */,
        HOST_MARKETRED               /* "marketred" */,
        HOST_USLUGI                  /* "uslugi" */,
        HOST_SEARCHAPP               /* "searchapp" */,
        HOST_TUTOR                   /* "tutor" */,
        HOST_MORDA                   /* "morda" */,
        HOST_YARU                    /* "yaru" */,
        HOST_MESSENGER               /* "messenger" */,
        HOST_AVATARS                 /* "avatars" */,
        HOST_VOICE                   /* "voice" */,
        HOST_EDA                     /* "eda" */,
        HOST_TURBO                   /* "turbo" */,
        HOST_TURBOSITE               /* "turbosite" */,
        HOST_MARKETAPI_WHITE         /* "marketapi_white" */,
        HOST_MARKETAPI_BLUE          /* "marketapi_blue" */,
        HOST_MARKETFAPI_WHITE        /* "marketfapi_white" */,
        HOST_MARKETFAPI_BLUE         /* "marketfapi_blue" */,
        HOST_HEALTH                  /* "health" */,
        HOST_PLUS                    /* "plus" */,
        HOST_CLASSIFIED              /* "classified" */,
        HOST_CLOUD                   /* "cloud" */,
        HOST_ADVERT                  /* "advert" */,
        HOST_LAVKA                   /* "lavka" */,
        HOST_DIRECT                  /* "direct" */,
        HOST_MAPS_CORE               /* "maps_core" */,
        HOST_IOT                     /* "iot" */,
        HOST_UAC                     /* "uac" */,
        HOST_CAPTCHA_GEN             /* "captcha_gen" */,
        HOST_KINOPOISKHD             /* "kinopoisk_hd" */,
        HOST_MARKET_OTHER            /* "market_other" */,
        HOST_CONTEST                 /* "contest" */,
        HOST_APIAUTO                 /* "apiauto" */,
        HOST_DISK                    /* "disk" */,
        HOST_LPC                     /* "lpc" */,
        HOST_NMAPS                   /* "nmaps" */,
        HOST_MAIL                    /* "mail" */,
        HOST_SCHOOLBOOK              /* "schoolbook" */,
        HOST_PRACTICUM               /* "practicum" */,
        HOST_DIALOGS                 /* "dialogs" */,
        HOST_ANY                     /* "any" */,
        HOST_APIEDA                  /* "apieda" */,
        HOST_TIPS                    /* "tips" */,
        HOST_PASSPORT                /* "passport" */,
        HOST_TRUST                   /* "trust" */,
        HOST_BILLING                 /* "billing" */,
        HOST_MESSENGER_WEB_Q         /* "messenger_web_q" */,
        HOST_APIMAPS                 /* "apimaps" */,
        HOST_MESSENGER_WEB_DISTRIBUTION  /* "messenger_web_distribution" */,
        HOST_GAMES                  /* "games" */,
        HOST_BROWSER                /* "browser" */,
        HOST_INVEST                 /* "invest" */,
        HOST_PROMO                  /* "promo" */,
        HOST_MAPS_TILES             /* "maps_tiles" */,
        HOST_MARKETMAPI             /* "marketmapi" */,
        HOST_MAPS_MOBPROXY          /* "maps_mobproxy" */,
        HOST_MARKET_4BUSINESS       /* "market_4business" */,
        HOST_GEOADV_EXTERNAL        /* "geoadv_external" */,
        HOST_RASP                   /* "rasp" */,
        HOST_MEDIABILLING           /* "mediabilling" */,
        HOST_TV                     /* "tv" */,
        HOST_TOLOKA                 /* "toloka" */,
        HOST_RASP_SUBURBAN          /* "rasp_suburban" */,
        HOST_ADFOX                  /* "adfox" */,
        HOST_BANK                   /* "bank" */,
        HOST_SMARTTV                /* "smarttv" */,
        HOST_DIEHARD                /* "diehard" */,
        HOST_RETRIEVER              /* "retriever" */,
        HOST_MAPS_GEOCODE           /* "maps_geocode_search_api" */,
        HOST_BAND_LINK              /* "band_link" */,
        HOST_K50                    /* "k50" */,
        HOST_WIKI                   /* "wiki" */,
        HOST_TRAVEL_APP             /* "travel_app" */,
        HOST_FORMS                  /* "forms" */,
        HOST_JOBS                   /* "jobs" */,
        HOST_SUPPORT                /* "support" */,
        HOST_EDADEAL_CHERCHER       /* "edadeal_chercher" */,
        HOST_EXTERNAL_CLOUD         /* "external_cloud" */,
        HOST_DRM                    /* "drm" */,
        HOST_EXTERNAL_ABT           /* "external_abt" */,
        HOST_YANDEXPAY              /* "yandexpay" */,
        HOST_NUMTYPES,
        Count = HOST_NUMTYPES,

        HOST_TYPE_FIRST = HOST_OTHER,
        HOST_TYPE_LAST = HOST_NUMTYPES,
    };

    constexpr std::array<EHostType, 9> MARKET_TYPES = {
        HOST_MARKET,
        HOST_MARKETAPI,
        HOST_MARKETAPI_BLUE,
        HOST_MARKETAPI_WHITE,
        HOST_MARKETBLUE,
        HOST_MARKETFAPI_BLUE,
        HOST_MARKETFAPI_WHITE,
        HOST_MARKETPARTNER,
        HOST_MARKETRED,
    };

    constexpr std::array<EHostType, 11> MARKET_TYPES_EXT = {
        HOST_MARKET_OTHER,
        HOST_MARKET,
        HOST_MARKETAPI,
        HOST_MARKETAPI_BLUE,
        HOST_MARKETAPI_WHITE,
        HOST_MARKETBLUE,
        HOST_MARKETFAPI_BLUE,
        HOST_MARKETFAPI_WHITE,
        HOST_MARKETPARTNER,
        HOST_MARKETRED,
        HOST_MARKETMAPI,
    };

    constexpr std::array<EHostType, 3> VERTICAL_TYPES = {
        HOST_AUTORU,
        HOST_APIAUTO,
        HOST_REALTY,
    };

    constexpr std::array<EHostType, 8> DEPRECATED_AJAX_HOST_TYPES = {
        HOST_MARKET,
        HOST_ZEN,
        HOST_WEB,
        HOST_WEBMASTER,
        HOST_TUTOR,
        HOST_MUSIC,
        HOST_IMAGES,
        HOST_USLUGI,
    };

    /// Not used anymore.
    /// Not removed because there are still factors in the formula which correspond
    /// to every value of the enum.
    enum EReportType {
        REP_OTHER = 0                /* "other" */,
        REP_MARKET_OTHER             /* "market_other" */,
        REP_MARKET_MAIN              /* "market_main" */,
        REP_MARKET_STATIC            /* "market_static" */,
        REP_MARKET_SEARCH            /* "market_search" */,
        REP_MARKET_CATALOG_OFFERS    /* "market_catalogoffers" */,
        REP_MARKET_CATALOG_MODELS    /* "market_catalogmodels" */,
        REP_MARKET_CATALOG           /* "market_catalog" */,
        REP_MARKET_MODEL_PRICES      /* "market_modelprices" */,
        REP_MARKET_MODEL             /* "market_model" */,
        REP_MARKET_GURU              /* "market_guru" */,
        REP_MARKET_OFFERS            /* "market_offers" */,
        REP_MARKET_MODEL_OPINIONS    /* "market_modelopinions" */,
        REP_MARKET_BRANDS            /* "market_brands" */,
        REP_MARKET_COMPARE           /* "market_compare" */,
        REP_MARKET_SHOP              /* "market_shop" */,
        REP_BLOGS_OTHER              /* "blogs_other" */,
        REP_BLOGS_MAIN               /* "blogs_main" */,
        REP_BLOGS_THEME              /* "blogs_theme" */,
        REP_BLOGS_SEARCH             /* "blogs_search" */,
        REP_NUMTYPES
    };

    enum ECaptchaReqType {
        CAPTCHAREQ_NONE,
        CAPTCHAREQ_SHOW,
        CAPTCHAREQ_IMAGE,
        CAPTCHAREQ_CHECK,
    };

    enum EBlockCategory {
        BC_UNDEFINED = 0,
        BC_NON_SEARCH,
        BC_SEARCH,
        BC_SEARCH_WITH_SPRAVKA,
        BC_ANY_FROM_WHITELIST,
        BC_NUM,

        BLOCK_CATEGORY_FIRST = BC_UNDEFINED,
        BLOCK_CATEGORY_LAST = BC_NUM,
    };

    const TVector<EBlockCategory>& AllBlockCategories();

    /**
     * The function expects the entire URL, for example:
     *   - http://yandex.ru/yandsearch?text=...
     *   - yandex.com.tr
     *   - images.yandex.ru/yandsearch?text=...
     */
    EHostType HostToHostType(TStringBuf url);
    EHostType HostToHostType(TStringBuf host, TStringBuf doc, TStringBuf cgi);
    EHostType ParseHostType(ui32 x);
} // namespace NAntiRobot
