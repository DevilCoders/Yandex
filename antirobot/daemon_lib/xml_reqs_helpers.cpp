#include "xml_reqs_helpers.h"

#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/kmp_skip_search.h>

namespace NAntiRobot {

namespace {

static const TKmpSkipSearch XML_SEARCH_MATCHER("xmlsearch.");

}

TStringBuf NoXmlSearch(const TStringBuf& host) {
    TStringBuf res = XML_SEARCH_MATCHER.SearchInText(host);
    return res.empty() ? host : res.SubStr(XML_SEARCH_MATCHER.Length());
}

}
