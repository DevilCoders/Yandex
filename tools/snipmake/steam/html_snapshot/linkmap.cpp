#include "linkmap.h"
#include <library/cpp/html/url/url.h>

namespace NSteam
{

bool MergeUrlWithBase(const TString& link, const THttpURL& base, THttpURL& merged, ECharset charset)
{
    THttpURL result;
    THttpURL::TLinkType ltype = NHtml::NormalizeLinkForCrawl(base, link.data(), &result, charset);
    if (ltype == THttpURL::LinkIsBad) {
        return false;
    }

    THttpURL tmp;
    tmp.Copy(result);
    tmp.Set(THttpURL::FieldFragment, nullptr);
    if (!tmp.IsValidAbs()) {
        return false;
    }
    merged = tmp;
    return true;
}

}
