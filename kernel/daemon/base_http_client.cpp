#include "base_http_client.h"
#include "messages.h"

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/http/misc/httpcodes.h>

#include <util/string/strip.h>
#include <util/string/type.h>

bool TCommonHttpClientFeatures::ProcessSpecialRequest() {
    const TBaseServerRequestData& rd = GetBaseRequestData();
    const TCgiParameters& cgi = GetCgi();
    const TCgiParameters::const_iterator& iter =
        cgi.Find(TStringBuf("info_server"));
    if (iter != cgi.end() && IsTrue(iter->second) || TStringBuf("/info_server") == rd.ScriptName()) {
        ProcessInfoServer();
    } else if (TStringBuf("/ping") == rd.ScriptName()) {
        ProcessPing();
    } else if (TStringBuf("/status") == rd.ScriptName()) {
        ProcessServerStatus();
    } else if (TStringBuf("/tass") == rd.ScriptName()) {
        ProcessTass(Output());
    } else if (TStringBuf("/metric") == rd.ScriptName()) {
        GetMetrics(Output());
    } else if (TStringBuf("/supermind") == rd.ScriptName()) {
        ProcessSuperMind(Output());
    } else if (TStringBuf(rd.ScriptName()).StartsWith("/$")) {
        TSpecialServerInfoMessage messServerInfo(TString(TStringBuf(rd.ScriptName()).SubStr(2)));
        if (SendGlobalMessage(messServerInfo)) {
            Output() << "HTTP/1.1 " << HttpCodeStrEx(HTTP_OK) << "\r\n\r\n";
            Output() << messServerInfo.GetReport();
        } else {
            Output() << "HTTP/1.1 " << HttpCodeStrEx(HTTP_BAD_REQUEST) << "\r\n\r\n";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void TCommonHttpClientFeatures::ProcessInfoServer() {
    TServerInfo info = GetServerInfo();
    const TStringBuf origin = GetBaseRequestData().HeaderInOrEmpty("Origin");
    const TString originHeader(!!origin ? TString::Join("Access-Control-Allow-Origin:", origin, "\r\nAccess-Control-Allow-Credentials:true\r\n") : TString());
    Output() << "HTTP/1.1 200 Ok\r\n"sv
        << originHeader
        << TStringBuf("Content-Type: application/json\r\n\r\n");
    NJson::TJsonWriterConfig jsonFormat;
    jsonFormat.FormatOutput = true;
    jsonFormat.SortKeys = true;
    NJson::WriteJson(&Output(), &info, jsonFormat);
}

void TCommonHttpClientFeatures::ProcessPing() {
    const TStringBuf origin = GetBaseRequestData().HeaderInOrEmpty("Origin");
    const TString originHeader(!!origin ? TString::Join("Access-Control-Allow-Origin:", origin, "\r\nAccess-Control-Allow-Credentials:true\r\n") : TString());
    Output() << "HTTP/1.1 200 Ok\r\n"sv << originHeader << TStringBuf("\r\n1") << Endl;
}

TServerInfo TCommonHttpClientFeatures::GetServerInfo() const {
    return CollectServerInfo(new TCollectServerInfo);
}

void TCommonHttpClientFeatures::ProcessServerStatus() {
    const TStringBuf origin = GetBaseRequestData().HeaderInOrEmpty("Origin");
    const TString originHeader(!!origin ? TString::Join("Access-Control-Allow-Origin:", origin, "\r\nAccess-Control-Allow-Credentials:true\r\n") : TString());
    Output() << "HTTP/1.1 200 Ok\r\n"sv
        << originHeader
        << TStringBuf("Content-Type: text/plain\r\n\r\n")
        << "Started";
}

void TCommonHttpClientFeatures::GetMetrics(IOutputStream& out) const {
    out << "HTTP/1.1 404 Not Found\r\n\r\n"sv << Endl;
}

void TCommonHttpClientFeatures::ProcessSuperMind(IOutputStream& out) const {
    out << "HTTP/1.1 404 Not Found\r\n\r\n"sv << Endl;
}

void TCommonHttpClientFeatures::ProcessTass(IOutputStream& out) const {
    const TStringBuf origin = GetBaseRequestData().HeaderInOrEmpty("Origin");
    const TString originHeader(!!origin ? TString::Join("Access-Control-Allow-Origin:", origin, "\r\nAccess-Control-Allow-Credentials:true\r\n") : TString());
    out << "HTTP/1.1 200 Ok\r\n"sv
        << originHeader
        << TStringBuf("Content-Type: application/json\r\n\r\n");
    out << TUnistat::Instance().CreateJsonDump(0, false);
}
