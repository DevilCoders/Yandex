#include "reqtext_utils.h"

#include "log_utils.h"

#include <util/memory/tempbuf.h>
#include <util/string/strip.h>

namespace NAntiRobot {
    TString ExtractReqTextFromCgi(const TCgiParameters& cgiParams) {
        // TODO use the first occurency of text or query for msearch, only text for yandsearch, both for xmlsearch
        const TString& textText = cgiParams.Get(TStringBuf("text"));
        const TString& textQuery = cgiParams.Get(TStringBuf("query"));
        TString textRaw(textText.size() > textQuery.size() ? textText : textQuery);
        TTempBuf textUnesc(textRaw.size() + 1);

        AdvancedUrlUnescape(textUnesc.Data(), textRaw);

        for (char* c = textUnesc.Data(); *c; ++c) {
            if (*c == '+')
                *c = ' ';
        }

        TString res(textUnesc.Data());
        CollapseInPlace(res);
        return res;
    }
}
