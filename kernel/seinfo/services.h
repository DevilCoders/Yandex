#pragma once

#include <bitset>

#include "seinfo.h"

enum EYandexService {
   YS_WEB_SEARCH,
   YS_MAPS,
   YS_IMAGES_SEARCH,
   YS_NEWS,
   YS_YACA_SEARCH,
   YS_VIDEO_SEARCH,
   YS_MOB_SEARCH,
   YS_SITE_SEARCH,
   YS_MAIL,
   YS_METRIKA,
   YS_MUSIC,
   YS_MARKET,
   YS_PEOPLE,
   YS_WEATHER,
   YS_SHARE,
   YS_DIRECT,
   YS_UNDEF,
   YS_API,
   YS_HELP,
   YS_COMPANY,
   YS_FOTKI,
   YS_INTRASEARCH,
   YS_INTRASEARCH_AT,
   YS_INTRASEARCH_ST,
   YS_INTRASEARCH_DOC,
   YS_INTRASEARCH_PEOPLE,
   YS_INTRASEARCH_PLAN,
   YS_INTRASEARCH_STAT,
   YS_INTRASEARCH_STUDY,
   YS_INTRASEARCH_WIKI,
   YS_INTRASEARCH_CODE,
   YS_STARTREK,
   YS_RABOTA,
   YS_STORE,
   YS_TV,
   YS_SAAS,
   YS_PORTAL,
   YS_SAFEBROWSING_API,
   YS_SUPPORT,
   YS_CBIR,
   YS_COLLECTIONS,
   YS_TURBO,
   YS_TRANSLATE,
   YS_IZNANKA,
   YS_TV_ONLINE,
   YS_YDO,
   YS_HEALTH,
   YS_UGC,
   YS_TRIP,
   YS_PRODUCTS,
};

enum EMainServiceSpec {
    MSS_UNDEF = -1 /* "undefined" */,
    MSS_WEB = 0 /* "web" */,
    MSS_IMG = 1 /* "img" */,
    MSS_VID = 2 /* "vid" */,
    MSS_MAP = 3 /* "geo" */,
    MSS_PPL = 4 /* "ppl" */,
    MSS_PORTAL = 5 /* "portal" */,
    MSS_MAIL = 6 /* "mail" */,
    MSS_WEATHER = 7 /* "weather" */,
    MSS_MARKET = 8 /* "market" */,
    MSS_COLLECTIONS = 9 /* "collections" */,
    MSS_SAFE_BROWSING = 10 /* "safebrowsing" */,
    MSS_RECOMMENDATIONS = 11 /* "recommendations" */,
    MSS_DIRECT = 12 /* "direct" */,
    MSS_TURBO = 13 /* "turbo" */,
    MSS_UGC = 14 /* "ugc" */,
    MSS_IZNANKA = 15 /* "iznanka" */,
    MSS_TV_ONLINE = 16 /* "tv_online" */,
    MSS_TRANSLATE = 17 /* "translate" */,
    MSS_YDO = 18 /* "ydo" */,
    MSS_HEALTH = 19 /* "health" */,
    MSS_NEWS = 20 /* "news" */,
    MSS_PRODUCTS = 21 /* "products" */,
    MSS_NUM
};

using TMainServicesFlags = std::bitset<MSS_NUM>;

bool FromString(const TStringBuf& name, EMainServiceSpec& serv);

EMainServiceSpec ServiceToMainService(const EYandexService service);
EMainServiceSpec SearchTypeToMainService(const NSe::ESearchType searchType);
