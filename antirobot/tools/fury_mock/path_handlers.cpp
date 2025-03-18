#include "path_handlers.h"
#include "config.h"

#include <util/datetime/base.h>
#include <util/random/random.h>
#include <util/stream/str.h>
#include <util/string/printf.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/json/writer/json.h>
#include <library/cpp/json/json_reader.h>


THttpResponse BadRequest() {
    return THttpResponse(HTTP_BAD_REQUEST)
        .SetContent("Tests aren't supposed to call this method directly");
}

TCheckHandler::TCheckHandler(const TString& host, ui16 port)
    : Host(host)
    , Port(port)
{
}

THttpResponse IncorrectJsonHandler(const TRequestReplier::TReplyParams&) {
    return THttpResponse(HTTP_OK).SetContent("{\"qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq", TStringBuf("text/xml"));
}

THttpResponse TimeoutHandler(const TRequestReplier::TReplyParams&) {
    Sleep(TDuration::Seconds(2));
    return THttpResponse(HTTP_OK);
}

THttpResponse TCheckHandler::operator()(const TRequestReplier::TReplyParams& params) {
    TParsedHttpFull request(params.Input.FirstLine());

    auto requestContent = params.Input.ReadAll();

    NJson::TJsonValue jsonValue;
    if (!NJson::ReadJsonTree(requestContent, &jsonValue)) {
        return THttpResponse(HTTP_BAD_REQUEST);
    }
    Y_ENSURE(jsonValue["jsonrpc"].GetStringSafe() == "2.0");
    Y_ENSURE(jsonValue["method"].GetStringSafe() == "process");
    const auto imageResult = jsonValue["params"]["body"]["captcha_result"].GetBooleanSafe();
    const auto key = jsonValue["params"]["key"].GetStringSafe();

    const auto type = jsonValue["params"]["body"]["captcha_type"];
    const auto strategy = type == "button"
        ? STRATEGY_CONFIG.ButtonStrategy
        : STRATEGY_CONFIG.Strategy;
    Cerr << "Check strat (" << type << ") " << strategy << Endl;

    if (strategy == "timeout") {
        return TimeoutHandler(params);
    }
    if (strategy == "incorrect") {
        return IncorrectJsonHandler(params);
    }

    if (type != "button") {
        const auto sessionMetadata = jsonValue["params"]["body"]["session_metadata"];
        Y_ENSURE(sessionMetadata.IsMap());
        Y_ENSURE(sessionMetadata["voice_type"].GetString() == "ru");
        Y_ENSURE(sessionMetadata["voice_key"]["type"].GetString() == "sound_ru");
        Y_ENSURE(sessionMetadata["image_key"]["type"].GetString() == "txt_v1");
        Y_ENSURE(sessionMetadata["voice_metadata"]["answer"].GetString().length() > 0);
        Y_ENSURE(sessionMetadata["image_metadata"]["answer"].GetString().length() > 0);
    }

    TVector<TString> verdicts;
    if (strategy == "random") {
        if (RandomNumber(100U) < 50) {
            verdicts.push_back("captcha_robot");
        }
    } else if (strategy == "success") {
        // no verdict
    } else if (strategy == "fail") {
        verdicts.push_back("captcha_robot");
    } else if (strategy == "as_image") {
        if (imageResult) {
            // no verdict
        } else {
            verdicts.push_back("captcha_robot");
        }
    } else {
        ythrow yexception() << "Invalid check strategy '" << strategy << "'";
    }

    // Response examples:
    // {"jsonrpc":"2.0","id":1,"result":[{"source":"antifraud","subsource":"captcha","entity":"key","key":"10pIGk1YA_uMYERwCg9Zzltn_cQ3bBOF","name":"captcha_robot","value":true}]}
    // {"jsonrpc":"2.0","id":1,"result":[]}

    TString str;
    TStringOutput so(str);
    NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
    json.BeginObject();
    json.WriteKey("jsonrpc").WriteString("2.0");
    json.WriteKey("id").WriteInt(1);
    json.WriteKey("result").BeginList();
    for (const auto& verdict : verdicts) {
        json.BeginObject();
        json.WriteKey("source").WriteString("antifraud");
        json.WriteKey("subsource").WriteString("captcha");
        json.WriteKey("entity").WriteString("key");
        json.WriteKey("key").WriteString(key);
        json.WriteKey("name").WriteString(verdict);
        json.WriteKey("value").WriteBool(true);
        json.EndObject();
    }
    json.EndList();
    json.EndObject();

    return THttpResponse(HTTP_OK).SetContent(str, TStringBuf("text/json"));
}

template<typename T>
void SetConfig(const TString& key, T& value, const T& newValue, const TVector<T> possibleValues) {
    if (!IsIn(possibleValues, newValue)) {
        throw yexception() << "Invalid '" << key << "' value '" << newValue << "'";
    }
    value = newValue;
}

THttpResponse SetStrategyHandler(const TRequestReplier::TReplyParams& params) {
    TParsedHttpFull request(params.Input.FirstLine());
    const auto cgiParameters = TCgiParameters(request.Cgi);
    const auto key = cgiParameters.Get("key");
    const auto value = cgiParameters.Get("value");
    try {
        const static TVector<TString> validStrategies = {"timeout", "incorrect", "success", "fail", "as_image", "random"};
        if (key == "Strategy") {
            SetConfig(key, STRATEGY_CONFIG.Strategy, value, validStrategies);
        } else if (key == "ButtonStrategy") {
            SetConfig(key, STRATEGY_CONFIG.ButtonStrategy, value, validStrategies);
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

