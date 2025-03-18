#include "static_handler.h"

#include <antirobot/captcha/localized_data.h>

#include <library/cpp/archive/yarchive.h>

#include <util/string/cast.h>


namespace NAntiRobot {

namespace {
    const char* JS_MIME_TYPE = "application/x-javascript";
    const char* CSS_MIME_TYPE = "text/css";
    const char* HTML_MIME_TYPE = "text/html";
    const char* SVG_MIME_TYPE = "image/svg+xml";
    const char* PNG_MIME_TYPE = "image/png";

    const char* NameToMimeType(const TString& name) {
        if (name.EndsWith(".js")) {
            return JS_MIME_TYPE;
        }
        if (name.EndsWith(".css")) {
            return CSS_MIME_TYPE;
        }
        if (name.EndsWith(".html")) {
            return HTML_MIME_TYPE;
        }
        if (name.EndsWith(".svg")) {
            return SVG_MIME_TYPE;
        }
        if (name.EndsWith(".png")) {
            return PNG_MIME_TYPE;
        }
        throw yexception() << "Can't get mime type of " << name;
    }

    bool DisableCache(TStringBuf doc) {
        return EqualToOneOf(doc, "/captcha.js", "/tmgrdfrend.js", "/themer.html");
    }
}

class TStaticHandler {
public:
    TStaticHandler(TStaticData& staticData)
        : Data(staticData)
    {
    }

    NThreading::TFuture<TResponse> operator()(TRequestContext& rc);

private:
    TStaticData& Data;
};

void TStaticData::AddDoc(const TString& key, const char* webPath, const char* mimeType) {
    Docs.push_back(TLocalizedData::Instance().GetArchiveReader().ObjectByKey(key)->ReadAll());
    CHttpServerStatic::AddDoc(webPath, (ui8*)(Docs.back().data()), Docs.back().size(), mimeType);
}

void TStaticData::AddDocWithHandler(const TString& key, const char* webPath, const char* mimeType) {
    AddDoc(key, webPath, mimeType);
    Handler.Add(webPath, TStaticHandler(*this));
}

TStaticData::TStaticData() {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        int version = ANTIROBOT_DAEMON_CONFIG.StaticFilesVersion <= 0
            ? TLocalizedData::Instance().GetExternalVersionedFilesVersions().back()
            : ANTIROBOT_DAEMON_CONFIG.StaticFilesVersion;

        AddDocWithHandler("/" + ToString(version) + "-captcha.js", "/captcha.js", JS_MIME_TYPE);
        AddDocWithHandler("/demo.html", "/demo.html", HTML_MIME_TYPE);
        AddDocWithHandler("/themer.html", "/themer.html", HTML_MIME_TYPE);

        for (const TString& name : TLocalizedData::Instance().GetExternalVersionedFiles()) {
            if (name.EndsWith(".html")) {
                AddDoc(name, name.c_str(), NameToMimeType(name));
            } else {
                AddDocWithHandler(name, name.c_str(), NameToMimeType(name));
            }
        }
    } else {
        AddDocWithHandler("/captcha.min.css", "/captcha.min.css", CSS_MIME_TYPE); // TODO: удалить в следующей версии
        AddDocWithHandler("/captcha.ie.min.css", "/captcha.ie.min.css", CSS_MIME_TYPE); // TODO: удалить в следующей версии

        for (const TString& name : TLocalizedData::Instance().GetAntirobotVersionedFiles()) {
            if (!name.Contains(".html.")) {
                AddDocWithHandler(name, name.c_str(), NameToMimeType(name));
                AddDocWithHandler(name, ("/static/media" + name).c_str(), NameToMimeType(name));
            }
        }
    }
    AddDocWithHandler("/tmgrdfrend.js", "/tmgrdfrend.js", JS_MIME_TYPE);
};

NThreading::TFuture<TResponse> TStaticHandler::operator()(TRequestContext& rc) {
    TStringStream contentOutput;
    Data.PathHandle(ToString(rc.Req->Doc).c_str(), contentOutput);

    THttpInput httpInput(&contentOutput);
    auto resp = TResponse::ToUser(HTTP_OK);
    for (const auto& header : httpInput.Headers()) {
        if (header.Name() == "Content-Length") {
            // будет выставлен автоматически в конце
            continue;
        }
        if (!(header.Name() == "Cache-Control" && DisableCache(rc.Req->Doc))) {
            resp.AddHeader(header);
        }
    }
    if (DisableCache(rc.Req->Doc)) {
        resp.AddHeader("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate");
    }
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        resp.AddHeader("Access-Control-Allow-Origin", "*");
    }
    resp.SetContent(httpInput.ReadAll());
    return NThreading::MakeFuture(std::move(resp));
}

} // namespace NAntiRobot
