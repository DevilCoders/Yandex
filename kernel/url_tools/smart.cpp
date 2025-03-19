#include "smart.h"

#include "utf8.h"

#include <util/charset/utf8.h>
#include <util/string/subst.h>
#include <util/string/strip.h>

TIsUrlSmartResult::TIsUrlSmartResult()
    : IsStraight(false)
{
}

TIsUrlSmartResult::TIsUrlSmartResult(const TIsUrlResult& isUrlResult, bool isStraight)
    : TIsUrlResult(isUrlResult)
    , IsStraight(isStraight)
{
}

static TIsUrlSmartResult IsUrlSmartInt(const TString& req, int flags) {
    TIsUrlResult isUrl = IsUrl(req, flags);
    if (isUrl.IsUrl)
        return TIsUrlSmartResult(isUrl, true);

    if (req.find('.') == TString::npos) {
        TString reqSpaceDot = req;
        if (SubstGlobal(reqSpaceDot, ' ', '.')) {
            isUrl = IsUrl(reqSpaceDot, flags);
            if (isUrl.IsUrl)
                return TIsUrlSmartResult(isUrl, false);
        }
    }

    TString reqDotSubst = req;
    if (CaseInsensitiveSubstUTF8(reqDotSubst, "точка", ".")) {
        isUrl = IsUrl(reqDotSubst, flags);
        if (isUrl.IsUrl)
            return TIsUrlSmartResult(isUrl, false);
        if (SubstGlobal(reqDotSubst, " ", "")) {
            isUrl = IsUrl(reqDotSubst, flags);
            if (isUrl.IsUrl)
                return TIsUrlSmartResult(isUrl, false);
        }
    }

    //Check that the request is ASCII string
    //Otherwise requests of form [russian word] [url] are recognized as IDNs
    bool isAscii = (UTF8Detect(req.data(), req.size()) == ASCII);
    if (isAscii) {
        TString reqNoSpace = req;
        if (SubstGlobal(reqNoSpace, ' ', '.')) {
            SubstGlobal(reqNoSpace, "..", ".");
            isUrl = IsUrl(reqNoSpace, flags);
            if (isUrl.IsUrl)
                return TIsUrlSmartResult(isUrl, false);
        }
    }

    return TIsUrlSmartResult();
}

TIsUrlSmartResult IsUrlSmart(TString req, int flags) {
    TIsUrlResult isUrl = IsUrl(req, flags);
    if (isUrl.IsUrl)
        return TIsUrlSmartResult(isUrl, true);

    CollapseInPlace(req);
    StripInPlace(req);

    if (req.StartsWith("сайт") || req.EndsWith("сайт")) {
        CaseInsensitiveSubstUTF8(req, "сайт", "");
        StripInPlace(req);
    }
    return IsUrlSmartInt(req, flags);
}
