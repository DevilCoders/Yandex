#pragma once

#include "linkmap.h"


#include <library/cpp/html/sanitize/css/css_sanitize.h>
#include <library/cpp/html/sanitize/common/url_processor/url_processor.h>

#include <library/cpp/uri/http_url.h>

#include <util/generic/string.h>
#include <library/cpp/charset/doccodes.h>
#include <util/system/mutex.h>
#include <util/system/guard.h>

namespace NSteam
{

class ICssConverter
{
public:
    virtual TString ConvertCss(const TString& css, const THttpURL& baseUrl, ECharset encoding, bool isInlineStyle) = 0;
    virtual ~ICssConverter() {};
};

// Client to CSS sanitizer, wrapper for IUrlMapper
class TUrlMapperWrapper: public IUrlProcessor
{
    IUrlMapper& Client;
    ECharset Charset;
    const THttpURL& BaseUrl;

public:
    TUrlMapperWrapper(IUrlMapper& client, ECharset charset, const THttpURL& baseUrl)
        : Client(client)
        , Charset(charset)
        , BaseUrl(baseUrl)
    {
    }

    TString Process(const char* sUrl) override
    {
        TString url(sUrl);
        TString lowerUrl(sUrl);
        lowerUrl.to_lower();
        IUrlMapper::ELinkType linkType = lowerUrl.EndsWith(".css") ? IUrlMapper::LINK_CSS : IUrlMapper::LINK_IMAGE;
        THttpURL parsedUrl;
        if (!MergeUrlWithBase(url, BaseUrl, parsedUrl, Charset)) {
            return URL_ERROR_PREFIX + sUrl;
        }
        return Client.RewriteUrl(parsedUrl.PrintS(), linkType);
    }

    bool IsAcceptedAttr(const char* /*tagName*/, const char* /*attrName*/) override {
        return true;
    }
};

class TAncientCssConverter: public ICssConverter
{
    IUrlMapper& Mapper;
    TMutex ConverterMutex;
    Yandex::NCssSanitize::TCssSanitizer CssSanitizer;

public:
    TAncientCssConverter(const TString& configDir, IUrlMapper& mapper)
        : Mapper(mapper)
    {
        CssSanitizer.OpenConfig(configDir + "/css.cfg");
        CssSanitizer.SetProcessAllUrls(true);
    }

    TString ConvertCss(const TString& css, const THttpURL& baseUrl, ECharset encoding, bool isInlineStyle) override
    {
        TString result;

        TGuard<TMutex> guard(ConverterMutex);
        TUrlMapperWrapper wrapper(Mapper, encoding, baseUrl);
        CssSanitizer.SetUrlProcessor(&wrapper);
        result = isInlineStyle ? CssSanitizer.SanitizeInline(css) : CssSanitizer.Sanitize(css);
        CssSanitizer.SetUrlProcessor(nullptr);
        return result;
    }
};


}
