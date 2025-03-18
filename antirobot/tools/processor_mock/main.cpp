#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>

class TReplier: public TRequestReplier {
public:
    TReplier(TDuration responseWaitTime)
        : ResponseWaitTime(responseWaitTime)
    {
    }

    bool DoReply(const TReplyParams& params) override {
        if (ResponseWaitTime > TDuration::Zero()) {
            Sleep(ResponseWaitTime);
        }
        params.Output << THttpResponse(HTTP_OK);
        return true;
    }

private:
    TDuration ResponseWaitTime;
};

struct TCallback: public THttpServer::ICallBack {
    TCallback(TDuration responseWaitTime)
        : ResponseWaitTime(responseWaitTime)
    {
    }

    TClientRequest* CreateClient() override {
        return new TReplier(ResponseWaitTime);
    }

private:
    TDuration ResponseWaitTime;
};

struct TParams {
    ui16 Port = 13513;
    TDuration ResponseWaitTime = TDuration::Zero();
};

TParams ParseParams(int argc, char** argv) {
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.SetFreeArgsNum(0);
    TParams params;
    if (TOpt* helpOpt = opts.FindLongOption("help")) {
        helpOpt->AddShortName('h');
    }

    opts.AddLongOption("port", "port to listen to")
        .StoreResult(&params.Port)
        .DefaultValue(params.Port)
        .OptionalArgument("<ui16>");

    size_t responseTimeMicroseconds;

    opts.AddLongOption("response-time-microseconds", "time to wait before response")
        .StoreResult(&responseTimeMicroseconds)
        .DefaultValue(params.ResponseWaitTime.MicroSeconds())
        .OptionalArgument("<size_t>");

    TOptsParseResult res(&opts, argc, argv);

    params.ResponseWaitTime = TDuration::MicroSeconds(responseTimeMicroseconds);

    return params;
}

int main(int argc, char** argv) {
    TParams params = ParseParams(argc, argv);

    ui16 port = params.Port;
    TCallback callback(params.ResponseWaitTime);
    THttpServer server(&callback, THttpServerOptions(port));
    if (!server.Start()) {
        Cerr << "Failed to start server: " << server.GetError() << Endl;
        return 2;
    }
    server.Wait();

    return 0;
}
