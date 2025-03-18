#pragma once

#include <library/cpp/html/face/parstypes.h>

#include <util/system/defaults.h>
#include <util/generic/string.h>

class IParsedDocProperties;

enum HTATTR_PARSE_FUNC {
    PARSE_FUNC_UNKNOWN = 0,
    PARSE_URL_LASTNAME,
    PARSE_URL_7BIT,
    PARSE_URL_AJAX_FRAGMENT,
    PARSE_HTTP_REFRESH,
    PARSE_HTTP_CHARSET,
    PARSE_META_ROBOTS,
    PARSE_HTTP_EXPIRES,
    PARSE_DATA_INTEGER,
    PARSE_META_YANDEX,
    PARSE_STRING_TO_FNVUI32,
    PARSE_META_FRAGMENT
};

TString parse_url_lastname(const TString& s, IParsedDocProperties* props);
TString parse_url_7bit(const TString& s, IParsedDocProperties* props);
TString parse_url_ajax_fragment(const TString& s, IParsedDocProperties* props);
TString parse_http_refresh(const TString& s, IParsedDocProperties* props);
TString parse_http_charset(const TString& s, IParsedDocProperties* props);
TString parse_meta_robots(const TString& s, IParsedDocProperties* props);
TString parse_http_expires(const TString& s, IParsedDocProperties* props);
TString parse_data_integer(const TString& s, IParsedDocProperties* props);
TString parse_meta_yandex(const TString& s, IParsedDocProperties* props);
TString parse_string_to_fnvui32(const TString& s, IParsedDocProperties* props);
TString parse_meta_fragment(const TString& s, IParsedDocProperties* props);

HTATTR_PARSE_FUNC GetParseFuncByName(const char* name);

TString CallToParseFunc(HTATTR_PARSE_FUNC func, const TString& s, IParsedDocProperties* props);

bool GetAttrType(const char* token, ATTR_TYPE& atype);
