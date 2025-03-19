#include "services.h"

EMainServiceSpec ServiceToMainService(const EYandexService service) {
    switch (service) {
        case YS_WEB_SEARCH:
            return MSS_WEB;
        case YS_IMAGES_SEARCH:
            return MSS_IMG;
        case YS_VIDEO_SEARCH:
            return MSS_VID;
        case YS_MAPS:
            return MSS_MAP;
        case YS_PORTAL:
            return MSS_PORTAL;
        case YS_CBIR:
            return MSS_IMG;
        case YS_TURBO:
            return MSS_TURBO;
        case YS_DIRECT:
            return MSS_DIRECT;
        case YS_IZNANKA:
            return MSS_IZNANKA;
        case YS_YDO:
            return MSS_YDO;
        case YS_TV_ONLINE:
            return MSS_TV_ONLINE;
        case YS_NEWS:
            return MSS_NEWS;
        case YS_MARKET:
            return MSS_MARKET;
        default:
            return MSS_UNDEF;
    }
}

EMainServiceSpec SearchTypeToMainService(const NSe::ESearchType searchType) {
    switch (searchType) {
        case NSe::ST_WEB:
            return MSS_WEB;
        case NSe::ST_IMAGES:
            return MSS_IMG;
        case NSe::ST_VIDEO:
            return MSS_VID;
        case NSe::ST_MAPS:
            return MSS_MAP;
        case NSe::ST_PORTAL:
            return MSS_PORTAL;
        default:
            return MSS_UNDEF;
    }
}
