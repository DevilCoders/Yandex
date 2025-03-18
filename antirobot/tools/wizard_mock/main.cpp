#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>

#include <util/string/cast.h>

class TReplier: public TRequestReplier {
public:
    TReplier(TDuration responseWaitTime)
        : ResponseWaitTime(responseWaitTime)
    {
    }

    bool DoReply(const TReplyParams& params) override {
        const char* wizardAnswer =
            "login="
            "&qtree=cHicTc6hC8JQEAbw7x4qj8fCcGWYZGmYhmmYxGQcBhGLIoZlk4hh0WgyGJW5zaBg38QyxPT8jzxMe3D8wn1399RMGRImbNmGKzw0lX7q4hvpXBctOOigi54h65wAJ-ChjyECTDBvhKW1JxwJJ6pMPQg5gd-boGnpwafBVpKJViXkwC0tj4Kz4EUIaTVWla79P-RjVNsdXp-puN65blORsimbsAl7YS9sxmZszMa3RYNvwbVCrGkjJEUE_uYPswBKCg%2C%2C"
            "&text=%D0%BF%D0%BE%D1%80%D0%BD%D0%BE%3A%3A1251";
        params.Output << THttpResponse(HTTP_OK).SetContent(wizardAnswer);
        return true;
    }

private:
    TDuration ResponseWaitTime;
};

class TCallback: public THttpServer::ICallBack {
public:
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
    ui16 Port = 8891;
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

int main(int argc, char* argv[]) {
    TParams params = ParseParams(argc, argv);

    TCallback callback(params.ResponseWaitTime);
    THttpServer server(&callback, THttpServerOptions(params.Port));
    if (!server.Start()) {
        Cerr << "Failed to start server: " << server.GetError() << Endl;
        return 2;
    }
    server.Wait();

    return 0;
}
