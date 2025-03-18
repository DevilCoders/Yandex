#include "req_types.h"

#include "config_global.h"
#include "fullreq_info.h"
#include "xml_reqs_helpers.h"

#include <antirobot/lib/kmp_skip_search.h>

#include <library/cpp/regex/regexp_classifier/regexp_classifier.h>

#include <util/generic/yexception.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/hostname.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NAntiRobot {
    const size_t BLOCK_CATEGORY_COUNT = BC_NUM - 1;

    const  TVector<EBlockCategory>& AllBlockCategories() {
        static const EBlockCategory list[] = {
            BC_NON_SEARCH,
            BC_SEARCH,
            BC_SEARCH_WITH_SPRAVKA,
            BC_ANY_FROM_WHITELIST,
        };

        static_assert(Y_ARRAY_SIZE(list) == BLOCK_CATEGORY_COUNT, "Count of block categories must be equal to BC_NUM - 1");

        static const TVector<EBlockCategory> result(list, list + Y_ARRAY_SIZE(list));
        return result;
    }

    namespace {
    TStringBuf CutMobileHostPrefix(TStringBuf host) {
        static const TStringBuf MOBILE_PREFIXES[] = {"m.", "pda.", "wap."};

        for (size_t i = 0; i < Y_ARRAY_SIZE(MOBILE_PREFIXES); ++i) {
            if (StartsWith(host, MOBILE_PREFIXES[i])) {
                return host.Tail(MOBILE_PREFIXES[i].size());
            }
        }
        return host;
    }
    } // anonymous namespace

#ifdef TLD_REGEXP
#undef TLD_REGEXP
#endif
#define TLD_REGEXP "\\.(\\w{2,3}|(com|co)\\.\\w{2,3})"

    TRegexpClassifier<EHostType> GenDocClassifier() {
        TVector<TRegexpClassifier<EHostType>::TClassDescription> descriptions;
        descriptions.reserve(ANTIROBOT_DAEMON_CONFIG.JsonConfig.ServiceRegExp.size());
        for (const auto& x : ANTIROBOT_DAEMON_CONFIG.JsonConfig.ServiceRegExp) {
            if (x.Cgi.Defined()) {
                descriptions.emplace_back(TString::Join(x.Doc, "\\?", *x.Cgi), x.Host);
            } else {
                descriptions.emplace_back(TString::Join(x.Doc, "(\\?.*|)"), x.Host);
            }
        }
        return TRegexpClassifier<EHostType>(descriptions.begin(),
                                            descriptions.end(),
                                            HOST_OTHER);
    }

    EHostType HostToHostType(TStringBuf host, TStringBuf doc, TStringBuf cgi) {
        static const TRegexpClassifier<EHostType> hostDocClassifier = GenDocClassifier();

        host = CutMobileHostPrefix(CutWWWPrefix(host));

        TTempBufOutput buf(host.Size() + doc.Size() + cgi.Size() + 1);
        buf << host << doc << "?" << cgi;
        return hostDocClassifier[TStringBuf(buf.Data(), buf.Filled())];
    }

#undef TLD_REGEXP

    EHostType HostToHostType(TStringBuf url) {
        url = CutHttpPrefix(url, false);

        TStringBuf doc;
        TStringBuf cgi;
        SplitUri(url, doc, cgi);
        size_t slashPosForDoc = doc.find('/'), slashPosForUrl = url.find('/');
        return HostToHostType(url.Head(slashPosForUrl), doc.Tail(slashPosForDoc), cgi);
    }

    EHostType ParseHostType(ui32 x) {
        Y_ENSURE(x < HOST_NUMTYPES, "Bad host type");
        return static_cast<EHostType>(x);
    }
}
