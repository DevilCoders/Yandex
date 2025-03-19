#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NQueryData {
    constexpr TStringBuf CGI_REQ_ID = "reqid";

    constexpr TStringBuf CGI_TEXT = "text";
    constexpr TStringBuf CGI_IGNORE_TEXT = "qd_ignore_text";
    constexpr TStringBuf CGI_USER_REQUEST = "user_request";
    constexpr TStringBuf CGI_NOQTREE = "noqtree";

    constexpr TStringBuf CGI_YANDEX_TLD = "tld";

    constexpr TStringBuf CGI_QSREQ_JSON_1 = "qsreq:json:1";
    constexpr TStringBuf CGI_QSREQ_JSON_ZLIB_1 = "qsreq:json/zlib:1";

    // rearr
    constexpr TStringBuf CGI_USER_REGIONS = "all_regions";
    constexpr TStringBuf CGI_USER_REGIONS_IPREG = "urcntr";
    constexpr TStringBuf CGI_USER_REGIONS_IP_COUNTRY = "ip_country";
    constexpr TStringBuf CGI_USER_REGIONS_IPREG_FULLPATH = "ipregions";
    constexpr TStringBuf CGI_USER_REGIONS_IPREG_DEBUG = "urcntr_debug";
    constexpr TStringBuf CGI_USER_IP_MOBILE_OP = "gsmop";
    constexpr TStringBuf CGI_USER_IP_MOBILE_OP_DEBUG = "gsmop_debug";
    constexpr TStringBuf CGI_USER_ID = "yandexuid";
    constexpr TStringBuf CGI_USER_LOGIN = "login";
    constexpr TStringBuf CGI_USER_LOGIN_HASH = "loginhash";
    constexpr TStringBuf CGI_BEGEMOT_FAILED = "begemot_failed";

    constexpr TStringBuf CGI_IS_DESKTOP = "is_desktop";
    constexpr TStringBuf CGI_IS_TABLET = "is_tablet";
    constexpr TStringBuf CGI_IS_TOUCH = "is_touch";
    constexpr TStringBuf CGI_IS_SMART = "is_smart";
    constexpr TStringBuf CGI_IS_MOBILE = "is_mobile";
    constexpr TStringBuf CGI_DEVICE = "device";
    constexpr TStringBuf CGI_STRUCT_KEYS = "qd_struct_keys";
    constexpr TStringBuf CGI_IS_RECOMMENDATIONS_REQUEST = "itditp_dynamic";

    constexpr TStringBuf CGI_DOC_IDS = "docids";
    constexpr TStringBuf CGI_URL_DEPRECATED = "qd_url"; // deprecated
    constexpr TStringBuf CGI_CATEGS = "qd_categs";

    constexpr TStringBuf CGI_SNIP_DOC_IDS = "snipdocids";
    constexpr TStringBuf CGI_SNIP_CATEGS = "qd_snipcategs";

    constexpr TStringBuf CGI_URLS = "qd_urls";

    constexpr TStringBuf CGI_SNIP_URLS = "qd_snipurls";

    // TODO: delete after deleting yweb/serpapi
    constexpr TStringBuf CGI_COMPRESSION_ZLIB = "zlib";

    enum ECgiCompression {
        CC_INVALID = -1 /*"INVALID"*/,
        CC_PLAIN = 0 /*"plain", "none"*/,
        CC_BASE64 /*"base64"*/,
        CC_ZLIB /*"zlib"*/,
        CC_LZ4 /*"lz4"*/
    };

    constexpr TStringBuf CGI_FILTER_NAMESPACES = "qd_filter_namespaces";
    constexpr TStringBuf CGI_FILTER_FILES = "qd_filter_files";

    constexpr TStringBuf CGI_SKIP_NAMESPACES = "qd_skip_namespaces";
    constexpr TStringBuf CGI_SKIP_FILES = "qd_skip_files";

    // relev
    constexpr TStringBuf CGI_DOPPEL_NORM = "dnorm";
    constexpr TStringBuf CGI_DOPPEL_NORM_W = "dnorm_w";
    constexpr TStringBuf CGI_STRONG_NORM = "norm";
    constexpr TStringBuf CGI_UI_LANG = "uil";

    ////////////////////////////////////////////////
    // scheme representation
    ////////////////////////////////////////////////

    TVector<std::pair<TString, TString>> GetCgiHelp();
}
