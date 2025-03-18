#include "options.h"
#include "path_handlers.h"

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/http/misc/parsed_request.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/http/server/response.h>

#include <util/generic/hash.h>

class TReplier: public TRequestReplier {
public:
    TReplier(const THashMap<TStringBuf, TPathHandler>& handlers, TDuration responseWaitTime)
        : Handlers(handlers)
        , ResponseWaitTime(responseWaitTime)
    {
    }

    bool DoReply(const TRequestReplier::TReplyParams& params) override {
        TParsedHttpFull req(params.Input.FirstLine());

        if (ResponseWaitTime > TDuration::Zero()) {
            Sleep(ResponseWaitTime);
        }

        if (const TPathHandler* h = Handlers.FindPtr(req.Path)) {
            try {
                params.Output << (*h)(params);
            } catch (...) {
                auto msg = CurrentExceptionMessage();
                Cerr << msg << Endl;
                params.Output << THttpResponse(HTTP_INTERNAL_SERVER_ERROR).SetContent(msg);
            }
        } else {
            params.Output << THttpResponse(HTTP_NOT_FOUND);
        }

        return true;
    }

private:
    const THashMap<TStringBuf, TPathHandler>& Handlers;
    TDuration ResponseWaitTime;
};

struct TCallback: public THttpServer::ICallBack {
    TCallback(const THashMap<TStringBuf, TPathHandler>& handlers, TDuration responseWaitTime)
        : Handlers(handlers)
        , ResponseWaitTime(responseWaitTime)
    {
    }

    TClientRequest* CreateClient() override {
        return new TReplier(Handlers, ResponseWaitTime);
    }

private:
    const THashMap<TStringBuf, TPathHandler>& Handlers;
    TDuration ResponseWaitTime;
};

int main(int argc, char* argv[]) {
    NLastGetopt::TOpts opts = CreateOptions();
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    ui16 port;
    THashMap<TStringBuf, TPathHandler> handlers;
    try {
        port = ParsePort(res);
        handlers = ParseHandlers(res);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        opts.PrintUsage(argv[0], Cerr);
        return 1;
    }

    TDuration responseWaitTime = ParseResponseWaitTime(res);
    TCallback callback(handlers, responseWaitTime);
    THttpServer server(&callback, THttpServerOptions(port).SetThreads(32));
    if (!server.Start()) {
        Cerr << "Failed to start server: " << server.GetError() << Endl;
        return 2;
    }
    server.Wait();

    return 0;
}
