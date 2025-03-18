#include "return_path.h"

#include "host_ops.h"
#include "request_params.h"

#include <antirobot/lib/keyring.h>

#include <yweb/weblibs/signurl/clickhandle.h>

#include <library/cpp/uri/http_url.h>

#include <library/cpp/string_utils/quote/quote.h>

#include <library/cpp/string_utils/base64/base64.h>

namespace NAntiRobot {

namespace {

const char URL_SIGNATURE_SEPARATOR = '_';
const TStringBuf HEADER_RETPATH = "X-Retpath-Y";

TString MakeInitialRetPath(const TRequest& req) {
    TString result;
    TStringOutput output(result);

    output << req.Scheme << req.HostWithPort << req.Doc;

    if (req.Doc.back() != '?')
        output << '?';

    if (req.ClientType == CLIENT_AJAX && (req.HostType == HOST_WEB || req.HostType == HOST_IMAGES)) {
        // hack for ajax web search, remove ajax=1 from cgi to get human-readable page on retpath
        TCgiParameters cgi = req.CgiParams;
        cgi.EraseAll(TStringBuf("ajax"));
        cgi.EraseAll(TStringBuf("callback"));
        cgi.EraseAll(TStringBuf("format"));
        cgi.EraseAll(TStringBuf("yu"));
        cgi.EraseAll(TStringBuf("_"));

        if (req.HostType == HOST_IMAGES) {
            cgi.EraseAll(TStringBuf("layout"));
            cgi.EraseAll(TStringBuf("request"));
        }
        output << cgi.Print();
    } else {
        output << req.Cgi;
    }

    return result;
}

void CheckRetpathValid(const TStringBuf& retpath) {
    THttpURL up;

    if (!up.ParseUri(retpath, THttpURL::FeaturesDefault | THttpURL::FeatureSchemeFlexible) == THttpURL::ParsedOK) {
        ythrow TReturnPath::TInvalidRetPathException() << "Bad retpath: '" << retpath << "'";
    }

    TString host = up.Get(THttpURL::FieldHost);

    for (const auto& item: GetYandexUrls()) {
        TString suff = "." + item;
        if (host.EndsWith(suff) || (item == host)) {
            return;
        }
    }

    ythrow TReturnPath::TInvalidRetPathException() << "Redirect to '" << retpath << "' is not allowed";
}

TString MakeInitialRetPathUsingHeader(const TStringBuf& retPath) {
    CheckRetpathValid(retPath);
    return TString(retPath);
}

}

const TStringBuf TReturnPath::CGI_PARAM_NAME = TStringBuf("retpath");

TReturnPath::TReturnPath(const TStringBuf& url)
    : Url(url)
{
}

TReturnPath TReturnPath::FromRequest(const TRequest& req)
{
    if (req.Headers.Has(HEADER_RETPATH)) {
        return TReturnPath(MakeInitialRetPathUsingHeader(req.Headers.Get(HEADER_RETPATH)));
    }

    return TReturnPath(MakeInitialRetPath(req));
}

TReturnPath TReturnPath::FromCgi(const TCgiParameters& cgi)
{
    try {
        const TString cgiValue = cgi.Get(CGI_PARAM_NAME);

        size_t base64SepPos = cgiValue.rfind(URL_SIGNATURE_SEPARATOR);
        
        if (base64SepPos == TString::npos) {
            ythrow TInvalidRetPathException() << "Symbol '" << URL_SIGNATURE_SEPARATOR
                                          << "' wasn't found in " << cgiValue;
        }

        const TStringBuf encodeBuf = cgiValue;
        const TString decodedUrl = Base64Decode(encodeBuf.SubStr(0, base64SepPos));
        const TStringBuf decodedSignature = encodeBuf.SubStr(base64SepPos + 1);

        if (!TKeyRing::Instance()->IsSignedHex(decodedUrl, decodedSignature)) {
           ythrow TInvalidRetPathException() << "Wrong signature in retpath "
                                          << decodedUrl << "_" << decodedSignature;
        }
        
        return TReturnPath(decodedUrl);
    }
    catch (...) {
        ythrow TInvalidRetPathException() << "Can't decode base64";
    }
}

void TReturnPath::AddToCGI(TCgiParameters& cgi) const {
    cgi.InsertUnescaped(
        CGI_PARAM_NAME,
        Base64EncodeUrl(Url) + URL_SIGNATURE_SEPARATOR + TKeyRing::Instance()->SignHex(Url)
    );
}

} /* namespace NAntiRobot */
