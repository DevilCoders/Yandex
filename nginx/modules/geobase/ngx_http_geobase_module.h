#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_array_t  *proxies;    /* array of ngx_cidr_t */
    ngx_flag_t    proxy_recursive;
    void         *lookUp;
} ngx_http_geobase_conf_t;


typedef enum {
    GEOBASE_COUNTRY = 0,       // "RU"
    GEOBASE_COUNTRY_NAME,      // "Russia"
    GEOBASE_COUNTRY_PART,      // "Central"
    GEOBASE_COUNTRY_PART_NAME, // "Central Federal District"
    GEOBASE_REGION,            // "RU-MOS"
    GEOBASE_REGION_NAME,       // "Moscow and Moscow Oblast"
    GEOBASE_CITY,              // "RU MOW"
    GEOBASE_CITY_NAME,         // "Moscow"
    GEOBASE_LATITUDE,
    GEOBASE_LONGITUDE,
    GEOBASE_PATH,              // "RU/Central/RU-MOS/RU MOW"
    GEOBASE_PATH_NAME,         // "Russia/Central Federal District/...."
    GEOBASE_REGION_ID,         // "213"
    GEOBASE_PARENTS_IDS,
    GEOBASE_CHILDREN_IDS
} EGeobaseField;


typedef enum {
    GEOBASE_TRAIT_IS_STUB = 0,
    GEOBASE_TRAIT_IS_RESERVED,
    GEOBASE_TRAIT_IS_YANDEX_NET,
    GEOBASE_TRAIT_IS_YANDEX_STAFF,
    GEOBASE_TRAIT_IS_YANDEX_TURBO,
    GEOBASE_TRAIT_IS_TOR,
    GEOBASE_TRAIT_IS_PROXY,
    GEOBASE_TRAIT_IS_VPN,
    GEOBASE_TRAIT_IS_HOSTING,
    GEOBASE_TRAIT_IS_MOBILE,
    GEOBASE_TRAIT_ISP_NAME,
    GEOBASE_TRAIT_ORG_NAME,
    GEOBASE_TRAIT_ASN_LIST
} EGeobaseTrait;


char * ngx_http_geobase_init(ngx_conf_t *cf, ngx_http_geobase_conf_t* conf, ngx_str_t *path);
void ngx_http_geobase_cleanup(void *);
ngx_int_t ngx_get_geobase_info(ngx_http_request_t *r,
    ngx_http_geobase_conf_t *conf, ngx_str_t ip,
    uintptr_t field, ngx_http_variable_value_t *val);
ngx_int_t ngx_get_geobase_trait(ngx_http_request_t *r,
    ngx_http_geobase_conf_t *conf, ngx_str_t ip,
    uintptr_t trait, ngx_http_variable_value_t *val);

#ifdef __cplusplus
}
#endif
