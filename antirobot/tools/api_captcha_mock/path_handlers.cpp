#include "config.h"
#include "path_handlers.h"
#include "image.h"

#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/stream/str.h>
#include <util/string/printf.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/writer/json.h>


THttpResponse BadRequest() {
    return THttpResponse(HTTP_BAD_REQUEST)
        .SetContent("Tests aren't supposed to call this method directly");
}

THttpResponse IncorrectXmlHandler(const TParsedHttpFull&) {
    return THttpResponse(HTTP_OK).SetContent("<?xml version='1.0'?><tag>value</tag>",
                                             TStringBuf("text/xml"));
}

THttpResponse TimeoutHandler(const TParsedHttpFull&) {
    Sleep(TDuration::Seconds(1));
    return THttpResponse(HTTP_OK);
}

THttpResponse PingHandler(const TParsedHttpFull&) {
    return THttpResponse(HTTP_OK).SetContent("pong", TStringBuf("text/plain"));
}

THttpResponse ImageHandler(const TParsedHttpFull&) {
    return THttpResponse(HTTP_OK).SetContent(TString{imageData}, TStringBuf("image/gif"));
}

template<typename T>
void SetConfig(const TString& key, T& value, const T& newValue, const TVector<T> possibleValues) {
    if (!IsIn(possibleValues, newValue)) {
        throw yexception() << "Invalid '" << key << "' value '" << newValue << "'";
    }
    value = newValue;
}

THttpResponse SetStrategyHandler(const TParsedHttpFull& request) {
    const auto cgiParameters = TCgiParameters(request.Cgi);
    const auto key = cgiParameters.Get("key");
    const auto value = cgiParameters.Get("value");
    try {
        if (key == "CheckStrategy") {
            SetConfig(key, STRATEGY_CONFIG.CheckStrategy, value, {"timeout", "incorrect", "success", "fail", "as_rep", "error"});
        } else if (key == "GenerateStrategy") {
            SetConfig(key, STRATEGY_CONFIG.GenerateStrategy, value, {"timeout", "incorrect", "correct"});
        } else {
            throw yexception() << "Invalid key '" << key << "'";
        }
    } catch (...) {
        TStringStream str;
        str << "{\"result\":\"failed\",\"desc\":\"" << CurrentExceptionMessage() << "\"}";
        return THttpResponse(HTTP_OK).SetContent(str.Str(), TStringBuf("text/json"));
    }
    return THttpResponse(HTTP_OK).SetContent("{\"result\":\"ok\"}", TStringBuf("text/json"));
}


TGenerateHandler::TGenerateHandler(const TString& host, ui16 port)
    : Host(host)
    , Port(port)
{
}

THttpResponse TGenerateHandler::operator()(const TParsedHttpFull& request) {
    const auto cgiParameters = TCgiParameters(request.Cgi);
    const auto type = cgiParameters.Get("type");
    const auto lang = cgiParameters.Get("vtype");
    const bool json = cgiParameters.Has("json", "1");

    auto strategy = STRATEGY_CONFIG.GenerateStrategy;
    if (strategy == "timeout") {
        return TimeoutHandler(request);
    }
    if (strategy == "incorrect") {
        return IncorrectXmlHandler(request);
    }
    if (strategy != "correct") {
        ythrow yexception() << "Invalid generate strategy '" << strategy << "'";
    }

    TStringStream str;

    TString host("u.captcha.yandex.net");
    TString port;

    if (!Host.empty()) {
        host = Host;
        port = Sprintf(":%u", Port);
    }

    TStringBuf contentType;

    // For testing purposes
    TString langAndTypeSuffix = "&type=" + type + "&vtype=" + lang;

    if (type == "localization") {
        str << "{\"images\":["
            << "{\"grid\":\"4,4\",\"bbox\":\"0.136,0.189;0.936,0.989\",\"url\":\"https://" << host << port << "/image?num=1&key=18Ei5r8CWzY6hMSApKgX7inSgr1ZY6pZ" << langAndTypeSuffix << "\",\"category\":\"платья, комбинезоны, халаты\"},"
            << "{\"grid\":\"4,4\",\"bbox\":\"0.044,0.033;0.844,0.833\",\"url\":\"https://" << host << port << "/image?num=2&key=18Ei5r8CWzY6hMSApKgX7inSgr1ZY6pZ" << langAndTypeSuffix << "\",\"category\":\"брюки, штаны, юбки, шорты\"}],"
            << "\"https\":1,\"token\":\"18Ei5r8CWzY6hMSApKgX7inSgr1ZY6pZ\",\"json\":\"1\"}";
        contentType = TStringBuf("text/json");
    } else {
        if (json) {
            str << "{\"imageurl\":\"http://" << host << port << "/image?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF" << langAndTypeSuffix << "\","
                << "\"https\":1,"
                << "\"token\":\"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF\","
                << "\"json\":\"1\"}";
            contentType = TStringBuf("text/json");
        } else {
            str << "<?xml version='1.0'?>"
                << "<number url=\"http://" << host << port << "/image?key=10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF" << langAndTypeSuffix << "\">"
                << "10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF"
                << "</number>";
            contentType = TStringBuf("text/xml");
        }
    }

    return THttpResponse(HTTP_OK).SetContent(str.Str(), contentType);
}

TCheckHandler::TCheckHandler(const TString& host, ui16 port)
    : Host(host)
    , Port(port)
{
}

THttpResponse TCheckHandler::operator()(const TParsedHttpFull& request) {
    const auto cgiParameters = TCgiParameters(request.Cgi);
    const bool json = cgiParameters.Has("json", "1");
    const auto client = cgiParameters.Get("client");
    const auto rep = cgiParameters.Get("rep");
    auto strategy = STRATEGY_CONFIG.CheckStrategy;

    if (strategy == "timeout") {
        return TimeoutHandler(request);
    }
    if (strategy == "incorrect") {
        return IncorrectXmlHandler(request);
    }
    TString status;
    TString error, errorDesc;
    if (strategy == "success") {
        status = "ok";
    } else if (strategy == "fail") {
        status = "failed";
    } else if (strategy == "random") {
        status = RandomNumber(100U) < 50 ? "ok" : "failed";
    } else if (strategy == "as_rep") {
        status = rep;
    } else if (strategy == "error") {
        status = "failed";
        error = "test error";
        errorDesc = "test error description";
    } else {
        Cerr << "Invalid check strategy '" << strategy << "'" << Endl;
        ythrow yexception() << "Invalid check strategy '" << strategy << "'";
    }

    TStringStream str;

    if (json) {
        TString str;
        TStringOutput so(str);
        NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
        json.BeginObject();
        json.WriteKey("status").WriteString(status);
        json.WriteKey("json").WriteString("1");
        if (error) {
            json.WriteKey("error").WriteString(error);
        }
        if (errorDesc) {
            json.WriteKey("error_desc").WriteString(errorDesc);
        }
        if (client) {
            json.WriteKey("voice_answer").WriteString("1234");
            json.WriteKey("answer").WriteString("vvvvv");
            json.WriteKey("session_metadata").BeginObject();
            {
                json.WriteKey("voice_type").WriteString("ru");
                json.WriteKey("voice_key").BeginObject();
                {
                    json.WriteKey("version").WriteString("3a879be0f31b03abac31f2e2dbb34154");
                    json.WriteKey("type").WriteString("sound_ru");
                    json.WriteKey("id").WriteString("captcha/data/voice/ru/000/005/00000512.emp3");
                }
                json.EndObject();
                json.WriteKey("image_client_ip").WriteString("2a02:6b8:c08:228d:10b:2afa:0:1b58");
                json.WriteKey("image_key").BeginObject();
                {
                    json.WriteKey("version").WriteString("9b761947a632507fe96e5a3efc8b0e10");
                    json.WriteKey("type").WriteString("txt_v1");
                    json.WriteKey("id").WriteString("txt_v1/c2/08731256-23d7-492b-a437-40ce18a4be58");
                }
                json.EndObject();
                json.WriteKey("voice_metadata").BeginObject();
                {
                    json.WriteKey("content_type").WriteString("audio/mpeg; charset=utf-8");
                    json.WriteKey("answer").WriteString("1234");
                }
                json.EndObject();
                json.WriteKey("check_timestamp").WriteLongLong(1622716759233LL);
                json.WriteKey("image_timestamp").WriteLongLong(1622716732909LL);
                json.WriteKey("image_metadata").BeginObject();
                {
                    json.WriteKey("content_type").WriteString("image/png; charset=utf-8");
                    json.WriteKey("answer").WriteString("vvvvv");
                }
                json.EndObject();
                json.WriteKey("check_server_ip").WriteString("2a02:6b8:0:40c:b9aa:4beb:8d3c:7341");
            }
            json.EndObject();
        }
        json.EndObject();
        return THttpResponse(HTTP_OK).SetContent(str, TStringBuf("text/json"));
    } else {
        return THttpResponse(HTTP_OK).SetContent("<?xml version='1.0'?><image_check>" + status + "</image_check>", TStringBuf("text/xml"));
    }
}
