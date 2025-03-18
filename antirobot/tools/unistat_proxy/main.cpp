#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/system/mutex.h>

TMutex Mutex;
THashMap<TString, double> HashMap;

TMaybe<double> GetMetric(const THashMap<TString, double>& map, const TString& s);

class TReplier: public TRequestReplier {
public:
    bool DoReply(const TReplyParams& params) override {
        const TParsedHttpFull request(RequestString);
        const TCgiParameters cgi(request.Cgi);
        const TString& metricName = cgi.Get("metric");

        auto metric = GetMetric(HashMap, metricName);
        if (metric.Empty()) {
            params.Output << THttpResponse(HTTP_INTERNAL_SERVER_ERROR).SetContent("Empty metric");
            return true;
        }

        auto resp = THttpResponse(HTTP_OK).SetContent(ToString(metric.GetRef()));
        params.Output << resp;

        return true;
    }
};

struct TCallback: public THttpServer::ICallBack {
    TClientRequest* CreateClient() override {
        return new TReplier();
    }
};

struct TParams {
    TDuration RequestTimeout = TDuration::MilliSeconds(100);
    ui16 HttpPort = 9797;
    ui16 UnistatPort = 8899;
    TString UnistatLocation = "/admin?action=unistats";
    TString UnistatHost = "localhost";
    TDuration PollInterval = TDuration::Seconds(1);
};

TParams ParseParams(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    TParams params;
    if (TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddLongOption("http-port", "HTTP port to listen to")
        .StoreResult(&params.HttpPort)
        .DefaultValue(params.HttpPort)
        .OptionalArgument("<ui16>");
    opts.AddLongOption("unistat-port", "HTTP port to get signals from")
        .StoreResult(&params.UnistatPort)
        .DefaultValue(params.UnistatPort)
        .OptionalArgument("<ui16>");
    opts.AddLongOption("unistat-location", "HTTP location for get signals from")
        .StoreResult(&params.UnistatLocation)
        .DefaultValue(params.UnistatLocation)
        .OptionalArgument("<url>");
    opts.AddLongOption("unistat-host", "HTTP host name for get signals from")
        .StoreResult(&params.UnistatHost)
        .DefaultValue(params.UnistatHost)
        .OptionalArgument("<hostname>");

    size_t requestTimeoutMilliseconds = params.RequestTimeout.MilliSeconds();

    opts.AddLongOption("timeout-milliseconds", "Request timeout for signals request (milliseconds)")
        .StoreResult(&requestTimeoutMilliseconds)
        .DefaultValue(requestTimeoutMilliseconds)
        .OptionalArgument("<int>");

    double pollIntervalMilliseconds = params.PollInterval.MilliSeconds();

    opts.AddLongOption("poll-interval-milliseconds", "Request timeout for signals request (milliseconds)")
        .StoreResult(&pollIntervalMilliseconds)
        .DefaultValue(pollIntervalMilliseconds)
        .OptionalArgument("<int>");

    TOptsParseResult res(&opts, argc, argv);

    params.RequestTimeout = TDuration::MilliSeconds(requestTimeoutMilliseconds);
    params.PollInterval = TDuration::MilliSeconds(pollIntervalMilliseconds);
    return params;
}

TString FetchHttpResponseHeaders(const TParams& params) {
    NNeh::TMessage message = NNeh::TMessage::FromString("http://" + params.UnistatHost + ":" + ToString(params.UnistatPort) + params.UnistatLocation);

    NNeh::NHttp::MakeFullRequest(message, "", "");
    const auto resp = NNeh::Request(message)->Wait(params.RequestTimeout);

    if (resp && !resp->IsError()) {
        return resp->Data;
    } else {
        return TString{};
    }
}

TMaybe<double> GetMetric(const THashMap<TString, double>& map, const TString& s) {
    TGuard<TMutex> guard{Mutex};
    auto it = map.find(s);
    if (it == map.end()) {
        return Nothing();
    }
    return it->second;
}

int main(int argc, char** argv) {
    TParams params = ParseParams(argc, argv);
    NJson::TJsonValue json;

    TCallback callback;
    THttpServer server(&callback, THttpServerOptions(params.HttpPort));
    if (!server.Start()) {
        Cerr << "Failed to start server: " << server.GetError() << Endl;
        return 2;
    }

    while (true) {
        TInstant nextFetch = TInstant::Now() + params.PollInterval;
        TString unistat = FetchHttpResponseHeaders(params);
        if (unistat.Empty()) {
            Sleep(TDuration::MilliSeconds(100));
            continue;
        }
        ReadJsonTree(unistat, &json, true);

        auto array = json.GetArraySafe();

        {
            TGuard<TMutex> guard{Mutex};
            for (auto x : array) {
                auto parts = x.GetArraySafe();
                auto name = parts[0].GetStringSafe();
                auto value = parts[1].GetDoubleSafe();
                HashMap[name] = value;
            }
        }

        auto now = TInstant::Now();
        if (now < nextFetch) {
            Sleep(nextFetch - now);
        }
    }
}
