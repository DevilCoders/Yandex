#include <library/cpp/deprecated/split/delim_string_iter.h>

#include "non_common.h"

bool IsNonCommonQuery(TStringBuf cgiParamsStr, EMainServiceSpec serv,
                      bool allowRstr, bool isSiteSearch, bool isMailRu)
{
    if (isSiteSearch)
        return false; // TODO

    if (isMailRu)
        return false; // TODO

    const char* ACCEPTABLE_PREFIXES_WEB[] =
        { "suggest_reqid=", "csg=", "reqid=", "myreqid=", "app_req_id=",
          "l10n=", "st=", "factors=", "snip=mobile=", "device=",
          "service=", "exp_flags=", "mob_exp_config_id=", "cookies=1", "rearr=",
          "btype=", "ui=", "app_platform=", "bpage=", "slot-id=", "app_id=", "mob_exp_id=",
          "query_source=", "test-id=", "location_source=", "bregionid=",
          "session_info=", "uuid=", "banner_ua=", "location_accuracy=",
          "staticVersion=", "lr=", "text=", "stpar", "clid=", "p=", "rf=", "msid=", "oprnd=", "win=", "numdoc=", "ncrnd=",
          "tld=", "yasoft=", "lang=", "from=",
          "msp=1", "rdrnd=", "rpt=rad", "stype=", "noreask=", "ex=", "ua=", "ua_manually=", "corrected_from=",
          "callback=", "yu=", "ajax=", "_=", "ento=",
          "no-tests=", "i-m-a-hacker=" // see USERFEAT-500
        };

    const char* ACCEPTABLE_PREFIXES_WEB_RSTR[] =
        { "suggest_reqid=", "csg=", "reqid=", "myreqid=", "app_req_id=",
          "l10n=", "st=", "factors=", "snip=mobile=", "device=",
          "service=", "exp_flags=", "mob_exp_config_id=", "cookies=1", "rearr=",
          "btype=", "ui=", "app_platform=", "bpage=", "slot-id=", "app_id=", "mob_exp_id=",
          "query_source=", "test-id=", "location_source=", "bregionid=",
          "session_info=", "uuid=", "banner_ua=", "location_accuracy=",
          "staticVersion=", "lr=", "text=", "stpar", "clid=", "p=", "rf=", "msid=", "oprnd=", "win=", "numdoc=", "ncrnd=",
          "tld=", "rstr=-", "yasoft=", "lang=", "from=",
          "msp=1", "rdrnd=", "rpt=rad", "stype=", "noreask=", "ex=", "ua=", "ua_manually=", "corrected_from=",
          "callback=", "yu=", "ajax=", "_=", "ento=",
          "no-tests=", "i-m-a-hacker=" // see USERFEAT-500
        };

    const char* ACCEPTABLE_PREFIXES_IMG[] =
        { "text=", "myreqid=", "app_req_id=", "suggest_reqid=", "reqid=", "csg=", "uuid=", "serpid=",
          "lr=", "rpt=", "ed=1", "p=", "rf=", "msid=", "oprnd=", "requestid=", "left=", "right=", "clid=", "spsite=", "img_url=",
          "stype=image", "stype=simage", "nl=0", "tld=", "stpar", "ncrnd=", "yasoft=", "sp=", "rdrnd=", "from=",
          "type=pictures", "fyandex=", "g=", "msp=1", "s=", "bf=1",
          "callback=", "yu=", "ajax=", "_="};

    const char* ACCEPTABLE_PREFIXES_IMG_RSTR[] =
        { "text=", "myreqid=", "app_req_id=", "suggest_reqid=", "reqid=", "csg=", "uuid=", "serpid=",
          "lr=", "rpt=", "ed=1", "p=", "rf=", "msid=", "oprnd=", "requestid=", "left=", "right=", "clid=", "spsite=", "img_url=",
          "stype=image", "stype=simage", "nl=0", "tld=", "stpar", "ncrnd=", "yasoft=", "sp=", "rdrnd=", "isize=",
          "from=", "type=", "fyandex=", "g=", "msp=1", "s=", "icolor=",
          "h=" , "w=" , "wp=any", "bf=1", "itype=", "iorient=", "picsize=",
          "callback=", "yu=", "ajax=", "_="};

#define SELECT_ACCEPTABLE_PREFIXES(TYPE) \
    case MSS_##TYPE: \
        if (allowRstr) { \
            acceptablePrefixes = ACCEPTABLE_PREFIXES_##TYPE##_RSTR; \
            nPrefixes = Y_ARRAY_SIZE(ACCEPTABLE_PREFIXES_##TYPE##_RSTR); \
        } else { \
            acceptablePrefixes = ACCEPTABLE_PREFIXES_##TYPE; \
            nPrefixes = Y_ARRAY_SIZE(ACCEPTABLE_PREFIXES_##TYPE); \
        } \
        break;

    const char** acceptablePrefixes;
    size_t nPrefixes;
    switch (serv) {
        SELECT_ACCEPTABLE_PREFIXES(WEB)
        SELECT_ACCEPTABLE_PREFIXES(IMG)
        case MSS_VID:
        default:
            return false; // TODO
    }

#undef SELECT_ACCEPTABLE_PREFIXES

    for (TDelimStringIter it = begin_delim(cgiParamsStr,"&"); it.Valid(); ++it) {
        bool hasPrefix = false;

        for (size_t i = 0; i < nPrefixes; ++i) {
            if ((*it).StartsWith(acceptablePrefixes[i])) {
                hasPrefix = true;
                break;
            }
        }
        if (!hasPrefix) {
            return true;
        }
    }
    return false;
}

bool IsNonCommonSearchUrl(TStringBuf searchUrl, EMainServiceSpec serv, bool allowRstr,
                          bool isSiteSearch /*= false*/, bool isMailRu /*= false*/)
{
    if (searchUrl.empty())
        return false; // can't tell

    size_t qPos = searchUrl.find('?');
    if (qPos == TStringBuf::npos)
        ythrow yexception() << "Bad search url: " << searchUrl;
    return IsNonCommonQuery(searchUrl.SubStr(qPos + 1)
        , serv, allowRstr, isSiteSearch, isMailRu);
}



