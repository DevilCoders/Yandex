#pragma once

#include "convertresult.h"
#include "rewritecss.h"
#include "linkmap.h"

#include <dict/recognize/docrec/recognizer.h>

#include <kernel/recshell/recshell.h>

#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/html/zoneconf/attrextractor.h>
#include <library/cpp/html/zoneconf/ht_conf.h>
#include <library/cpp/html/html5/parse.h>
#include <library/cpp/html/url/url.h>
#include <library/cpp/numerator/numerate.h>
#include <library/cpp/uri/http_url.h>

#include <util/stream/output.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <util/system/mutex.h>
#include <util/system/guard.h>

namespace NSteam
{

class TPreparsedHtml
{
    TString Url;
    TBuffer Html;
    ECharset Charset;
    NHtml::TStorage Storage;
    THolder<IParsedDocProperties> DocProps;

    bool Valid;

public:
    TPreparsedHtml(const TString& html, const TString& url)
        : Url(url)
        , Html(html.data(), html.size() + 1) // ???
        , Charset(CODES_UNKNOWN)
        , Valid(false)
    {
        DocProps.Reset(CreateParsedDocProperties().Release());
        if (0 != DocProps->SetProperty(PP_BASE, url.data())) {
            Cerr << "error parsing base URL: [" << url << "]" << Endl;
        }
        else {
            Storage.SetPeerMode(true);
            NHtml::TParserResult parserResult(Storage);
            NHtml5::ParseHtml(Html, &parserResult, ::GetUrl(DocProps.Get()));
            Valid = true;
        }
    }

    TPreparsedHtml& SetCharset(ECharset charset)
    {
        Charset = charset;
        DocProps->SetProperty(PP_CHARSET, NameByCharset(charset));
        return *this;
    }

    TPreparsedHtml& Recognize(TRecognizer& recognizer, ECharset hint = CODES_UNKNOWN)
    {
        if (!Valid)
            return *this;

        ECharset enc(CODES_UNKNOWN);
        ELanguage lang(LANG_UNK);
        TRecognizer::THints recHints;
        recHints.Url = Url.data();
        if (hint != CODES_UNKNOWN && hint != CODES_UNSUPPORTED) {
            recHints.HttpCodepage.SafeSet(hint);
        }
        TRecognizerShell(&recognizer).Recognize(Storage.Begin(), Storage.End(),
            &enc, &lang, nullptr, recHints);
        if (CODES_UNKNOWN != enc) {
            Charset = enc;
            TString charset = NameByCharset(enc);
            DocProps->SetProperty(PP_CHARSET, charset.data());
        }
        DocProps->SetProperty(PP_LANGUAGE, NameByLanguage(lang));
        return *this;
    }

    const TBuffer& GetHtml() const
    {
        return Html;
    }

    bool IsValid() const
    {
        return Valid;
    }

    TString GetUrl() const
    {
        return Url;
    }

    ECharset GetCharset() const
    {
        return Charset;
    }

    bool Numerate(Numerator& numerator, TAttributeExtractor& extractor) const
    {
        numerator.Numerate(Storage.Begin(), Storage.End(), DocProps.Get(), &extractor);

        if (!numerator.DocFormatOK()) {
            Cerr << "Numerator error parsing HTML document: " << numerator.GetParseError() << Endl;
            return false;
        }

        return true;
    }
};

struct THtmlRewriter
{
    const TString ConfigDir;
    THtConfigurator HtConfig;

    THtmlRewriter(const TString& configDir)
        : ConfigDir(configDir)
    {
        HtConfig.Configure((ConfigDir + "/htparser.ini").data());
        HtConfig.AddAttrConf("image", "URL,any/meta.og:image");
    }

    TConvertResult TryConvert(const TPreparsedHtml& html, IUrlMapper& mapper, ICssConverter& cssConverter);
};

}
