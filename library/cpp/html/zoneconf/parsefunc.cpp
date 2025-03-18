#include "parsefunc.h"

#include <library/cpp/html/face/parsface.h>

#include <library/cpp/charset/codepage.h>
#include <util/datetime/base.h>
#include <util/digest/fnv.h>
#include <util/generic/ptr.h>
#include <library/cpp/http/misc/httpdate.h> // parse_http_date
#include <library/cpp/containers/str_hash/str_hash.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/util.h>

static str_spn sspn_spaces("\x20\t\n\r"); // "\v\f"

static const char* func_names[] = {
    "parse_url_lastname",
    "parse_url_7bit",
    "parse_url_ajax_fragment",
    "parse_http_refresh",
    "parse_http_charset",
    "parse_meta_robots",
    "parse_http_expires",
    "parse_data_integer",
    "parse_meta_yandex",
    "parse_string_to_fnvui32",
    "parse_meta_fragment",
    nullptr,
};

static HTATTR_PARSE_FUNC func_codes[] = {
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

static const char* CharsetCanonicalName(const char* name) {
    ECharset code = CharsetByName(name);
    return (code == CODES_UNKNOWN ? nullptr : NameByCharset(code));
}

class TName2FuncMapper: public Hash<HTATTR_PARSE_FUNC> {
public:
    TName2FuncMapper()
        : Hash<HTATTR_PARSE_FUNC>(func_names, 5, func_codes)
    {
    }
};

HTATTR_PARSE_FUNC GetParseFuncByName(const char* name) {
    HTATTR_PARSE_FUNC func;
    if (!Singleton<TName2FuncMapper>()->Has(name, &func))
        return PARSE_FUNC_UNKNOWN;
    return func;
}

TString parse_url_lastname(const TString& s, IParsedDocProperties* /*props*/) {
    if (s.find('?') != TString::npos)
        return TString();
    size_t pos = s.rfind('/');
    if (pos != TString::npos)
        return TString(s).remove(0, pos + 1);
    return s;
}

TString parse_string_to_fnvui32(const TString& s, IParsedDocProperties* /*props*/) {
    return ToString(FnvHash<ui32>(s.data(), s.size()));
}

TString parse_url_7bit(const TString& s, IParsedDocProperties* /*props*/) {
    for (auto p : s) {
        unsigned char uchar = (unsigned char)p;
        if (uchar > 0x7F)
            return TString();
    }
    return s;
}

TString parse_url_ajax_fragment(const TString& s, IParsedDocProperties* props) {
    const char* base = nullptr;
    TString ret;
    if (props && props->GetProperty(PP_BASE, &base) == 0) {
        if (s == "!")
            ret = TString(base) + "#!"; // rely on further link processing to rewrite fragment
        // what else?
    }
    return ret;
}

static str_spn sspn_num("0123456789");

/// @todo support non-conformant separators (like space etc)
TString parse_http_refresh(const TString& s, IParsedDocProperties* props) {
    // HTML4.0: "0,<url>"
    // NETSCAPE: "0, URL=<url>"
    // "[0-9]+\s*[,;]\s*([uU][rR][lL]\s*=\s*)"
    ui32 n;
    char* p;
    n = strtoul(s.data(), &p, 10); // strtoul skips initial whitespace
    if (p == s.data())
        n = ui32(-1);
    if (*p == '.') {
        p++;
        p += sspn_num.spn(p);
    }
    p += sspn_spaces.spn(p);
    if (*p != ',' && *p != ';')
        return TString();
    p += 1;
    p += sspn_spaces.spn(p);
    if (strnicmp(p, "URL", 3) == 0) {
        p += 3;
        p += sspn_spaces.spn(p);
        if (*p != '=')
            return TString();
        p += 1;
        p += sspn_spaces.spn(p);
        if (*p == '"' || *p == '\'') {
            p += 1;
            size_t last = strlen(p) - 1;
            if (p[last] == '\'' || p[last] == '"')
                p[last] = '\0';
            else
                return TString();
        }
        p += sspn_spaces.spn(p);
    }
    props->SetProperty(PP_REFRESHTIME, ToString(n).data());
    return p;
}

TString parse_http_charset(const TString& s, IParsedDocProperties* /*props*/) {
    // "text/html; charset=windows-1251"
    // "text/html;charset=windows-1251"
    const char* p = s.data();
    p += sspn_spaces.spn(p);
    if (strnicmp(p, "text/html", 9) != 0)
        return TString();
    p += 9;
    p += sspn_spaces.spn(p);
    if (*p != ';')
        return TString();
    p += 1;
    p += sspn_spaces.spn(p);
    if (strnicmp(p, "charset", 7) != 0)
        return TString();
    p += 7;
    p += sspn_spaces.spn(p);
    if (*p != '=')
        return TString();
    p += 1;
    p += sspn_spaces.spn(p);

    TString charset = Strip(p);

    const char* canon = CharsetCanonicalName(charset.data());
    if (canon)
        return canon;

    return charset;
}

inline void parse_meta_robots_state(const TString& s, char currentState[]) {
    char& mayIndex = currentState[0];
    char& mayFollow = currentState[1];
    char& mayArchive = currentState[2];
    char& mayODP = currentState[3];
    char& mayYACA = currentState[4];

    static const str_spn DELIMS(";, \t\n\r"); // "\v\f"
    TString sTemp = s;
    char* aptr = sTemp.begin(); // fork() inside begin()
    while (aptr < sTemp.end()) {
        const size_t len = DELIMS.cspn(aptr);
        aptr[len] = 0;
        if (stricmp(aptr, "ALL") == 0) {
            mayIndex = '1';
            mayFollow = '1';
        } else if (stricmp(aptr, "NONE") == 0) {
            mayIndex = (mayIndex == '1' ? '1' : '0');
            mayFollow = (mayFollow == '1' ? '1' : '0');
        } else if (stricmp(aptr, "INDEX") == 0) {
            mayIndex = '1';
        } else if (stricmp(aptr, "NOINDEX") == 0) {
            mayIndex = (mayIndex == '1' ? '1' : '0');
            ;
        } else if (stricmp(aptr, "FOLLOW") == 0) {
            mayFollow = '1';
        } else if (stricmp(aptr, "NOFOLLOW") == 0) {
            mayFollow = (mayFollow == '1' ? '1' : '0');
        } else if (stricmp(aptr, "ARCHIVE") == 0) {
            mayArchive = '1';
        } else if (stricmp(aptr, "NOARCHIVE") == 0) {
            mayArchive = (mayArchive == '1' ? '1' : '0');
        } else if (stricmp(aptr, "NOODP") == 0) {
            mayODP = (mayODP == '1' ? '1' : '0');
        } else if (stricmp(aptr, "NOYACA") == 0) {
            mayYACA = (mayYACA == '1' ? '1' : '0');
        }
        aptr += len + 1;
    }
}

inline void change_robots_yandex(char CurrentYandexState[], char CurrentRobotsState[]) {
    for (int i = 0; i < 5; i++) {
        if ((CurrentYandexState[i] != 'x') && (CurrentYandexState[i] != CurrentRobotsState[i])) {
            char& mayYaSmth = CurrentYandexState[i];
            char& mayRobotSmth = CurrentRobotsState[i];
            mayRobotSmth = mayYaSmth;
        }
    }
}

/// parse <meta name="robots" ...> - supported values:
///     ALL, NONE, INDEX, NOINDEX, FOLLOW, NOFOLLOW, ARCHIVE, NOARCHIVE, ODP, NOODP
///     YACA, NOYACA
/// return value: string of 5 chars
///  char 0: index
///  char 1: follow
///  char 2: archive
///  char 3: odp
///  char 4: yaca
/// char interpretation:
///  'x': not set -- allow
///  '0': set -- disallow
///  '1': set -- allow
/// so it is better to test for '0' always
/// chars override each other in the order provided
TString parse_meta_robots(const TString& s, IParsedDocProperties* props) {
    char currentState[] = "xxxxx";
    const char* meta_robots = nullptr;
    if (props && props->GetProperty(PP_ROBOTS, &meta_robots) == 0) {
        Y_ASSERT(meta_robots && strlen(meta_robots) == 5);
        memcpy(currentState, meta_robots, 5);
    }

    parse_meta_robots_state(s, currentState);

    char currentYandexState[] = "xxxxx";
    const char* meta_yandex = nullptr;
    if (props && props->GetProperty(PP_YANDEX, &meta_yandex) == 0) {
        Y_ASSERT(meta_yandex && strlen(meta_yandex) == 5);
        memcpy(currentYandexState, meta_yandex, 5);
    }

    change_robots_yandex(currentYandexState, currentState);

    return TString(currentState);
}

TString parse_http_expires(const TString& s, IParsedDocProperties* /*props*/) {
    time_t date;

    if ((date = parse_http_date(s)) == BAD_DATE)
        return TString();
    return DateToString(date);
}

TString parse_data_integer(const TString& s, IParsedDocProperties* /*props*/) {
    return ToString(FromString<int>(s));
}

TString parse_meta_yandex(const TString& s, IParsedDocProperties* props) {
    char currentState[] = "xxxxx";
    const char* meta_yandex = nullptr;
    if (props && props->GetProperty(PP_YANDEX, &meta_yandex) == 0) {
        Y_ASSERT(meta_yandex && strlen(meta_yandex) == 5);
        memcpy(currentState, meta_yandex, 5);
    }

    parse_meta_robots_state(s, currentState);

    char currentRobotsState[] = "xxxxx";
    const char* meta_robots = nullptr;
    if (props && props->GetProperty(PP_ROBOTS, &meta_robots) == 0) {
        Y_ASSERT(meta_robots && strlen(meta_robots) == 5);
        memcpy(currentRobotsState, meta_robots, 5);
    }

    change_robots_yandex(currentState, currentRobotsState);

    //if there are <meta name="yandex"> we have to change also the PP_ROBOTS property
    props->SetProperty(PP_ROBOTS, currentRobotsState);

    return TString(currentState);
}

TString parse_meta_fragment(const TString& s, IParsedDocProperties* /*props*/) {
    return s == "!" ? "1" : "0";
}

TString CallToParseFunc(HTATTR_PARSE_FUNC func, const TString& s, IParsedDocProperties* props) {
    switch (func) {
        case PARSE_URL_LASTNAME:
            return parse_url_lastname(s, props);
        case PARSE_URL_7BIT:
            return parse_url_7bit(s, props);
        case PARSE_URL_AJAX_FRAGMENT:
            return parse_url_ajax_fragment(s, props);
        case PARSE_HTTP_REFRESH:
            return parse_http_refresh(s, props);
        case PARSE_HTTP_CHARSET:
            return parse_http_charset(s, props);
        case PARSE_META_ROBOTS:
            return parse_meta_robots(s, props);
        case PARSE_HTTP_EXPIRES:
            return parse_http_expires(s, props);
        case PARSE_DATA_INTEGER:
            return parse_data_integer(s, props);
        case PARSE_META_YANDEX:
            return parse_meta_yandex(s, props);
        case PARSE_STRING_TO_FNVUI32:
            return parse_string_to_fnvui32(s, props);
        case PARSE_META_FRAGMENT:
            return parse_meta_fragment(s, props);
        default:
            ythrow yexception() << "invalid parse function";
    }
}

//**********************************************************************

// zone_aliases
static const char* attrNames[] = {
    "LITERAL",
    "URL",
    "DATE",
    "INTEGER",
    "BOOLEAN",
    nullptr,
};

static ATTR_TYPE attrTypes[] = {
    ATTR_LITERAL,
    ATTR_URL,
    ATTR_DATE,
    ATTR_INTEGER,
    ATTR_BOOLEAN,
};

class TAttrType: public Hash<ATTR_TYPE> {
public:
    TAttrType()
        : Hash<ATTR_TYPE>(attrNames, 11, attrTypes)
    {
    }
};

bool GetAttrType(const char* token, ATTR_TYPE& atype) {
    return Singleton<TAttrType>()->Has(token, &atype);
}
