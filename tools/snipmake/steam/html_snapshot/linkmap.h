#pragma once

#include <util/generic/string.h>
#include <library/cpp/uri/http_url.h>

static const TString URL_ERROR_PREFIX("error:");

namespace NSteam
{

class IUrlMapper
{
public:
    enum ELinkType
    {
        LINK_IMAGE,
        LINK_CSS,
        LINK_FRAME,
        LINK_UNKNOWN
    };

    virtual TString RewriteUrl(const TString& url, ELinkType linkType) = 0;
};

bool MergeUrlWithBase(const TString& link, const THttpURL& base, THttpURL& merged, ECharset charset);


}
